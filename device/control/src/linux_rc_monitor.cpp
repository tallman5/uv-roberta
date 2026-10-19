#include "roberta/control/sbus.hpp"

#include <pigpiod_if2.h>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

volatile std::sig_atomic_t stopping = 0;

void on_signal(int) {
    stopping = 1;
}

struct ReceiverState {
    std::mutex mutex;
    roberta::control::SbusEdgeDecoder decoder;
    std::optional<roberta::control::SbusFrame> latest_frame;
    std::chrono::steady_clock::time_point received_at{};
};

struct Options {
    unsigned gpio{17};
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--gpio" && index + 1 < argc) {
            const auto value = std::stoul(argv[++index]);
            if (value > 27) {
                throw std::invalid_argument("BCM GPIO must be between 0 and 27.");
            }
            options.gpio = static_cast<unsigned>(value);
        } else {
            throw std::invalid_argument("Usage: roberta_rc_monitor [--gpio BCM_PIN]");
        }
    }
    return options;
}

void alert_callback(
    int,
    unsigned,
    unsigned level,
    std::uint32_t tick,
    void* userdata) {
    auto& state = *static_cast<ReceiverState*>(userdata);
    const auto frame = state.decoder.feed(level, tick);
    if (frame) {
        std::lock_guard lock(state.mutex);
        state.latest_frame = frame;
        state.received_at = std::chrono::steady_clock::now();
    }
}

void print_status(ReceiverState& state, unsigned gpio) {
    std::optional<roberta::control::SbusFrame> frame;
    std::chrono::steady_clock::time_point received_at;
    {
        std::lock_guard lock(state.mutex);
        frame = state.latest_frame;
        received_at = state.received_at;
    }

    std::cout << "\rBCM GPIO " << gpio << " | ";
    if (!frame) {
        std::cout << "Waiting for a valid SBUS frame...";
    } else {
        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - received_at).count();
        if (age > 100) {
            std::cout << "STALE (" << age << " ms)";
        } else if (frame->failsafe) {
            std::cout << "FAILSAFE";
        } else if (frame->frame_lost) {
            std::cout << "FRAME LOST";
        } else {
            std::cout << "OK";
        }
        std::cout << " age=" << age << "ms";
        for (std::size_t channel = 0; channel < frame->channels.size(); ++channel) {
            std::cout << " CH" << channel + 1 << '=' << frame->channels[channel];
        }
        std::cout << " CH17=" << (frame->channel_17 ? "ON" : "OFF")
                  << " CH18=" << (frame->channel_18 ? "ON" : "OFF");
    }
    std::cout << " (Ctrl-C to exit)\033[K" << std::flush;
}

}  // namespace

int main(int argc, char** argv) {
    int pigpio_handle = -1;
    int callback_id = -1;
    try {
        const auto options = parse_options(argc, argv);
        std::signal(SIGINT, on_signal);
        std::signal(SIGTERM, on_signal);

        pigpio_handle = pigpio_start(nullptr, nullptr);
        if (pigpio_handle < 0) {
            throw std::runtime_error("Cannot connect to pigpiod. Check that the daemon is running.");
        }

        ReceiverState state;
        if (set_mode(
                static_cast<unsigned>(pigpio_handle),
                options.gpio,
                PI_INPUT) < 0 ||
            (callback_id = callback_ex(
                static_cast<unsigned>(pigpio_handle),
                options.gpio,
                EITHER_EDGE,
                alert_callback,
                &state)) < 0) {
            throw std::runtime_error(
                "Unable to configure the SBUS GPIO. Check BCM pin numbering and pigpiod.");
        }

        std::cout << "RC receiver monitor only; this program cannot command motors." << std::endl;
        while (!stopping) {
            print_status(state, options.gpio);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        std::cout << std::endl;

        callback_cancel(static_cast<unsigned>(callback_id));
        callback_id = -1;
        pigpio_stop(static_cast<unsigned>(pigpio_handle));
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "roberta_rc_monitor: " << error.what() << std::endl;
        if (pigpio_handle >= 0) {
            if (callback_id >= 0) {
                callback_cancel(static_cast<unsigned>(callback_id));
            }
            pigpio_stop(static_cast<unsigned>(pigpio_handle));
        }
        return 1;
    }
}
