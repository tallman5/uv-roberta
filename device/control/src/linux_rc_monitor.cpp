#include "roberta/control/linux_roboteq_telemetry.hpp"
#include "roberta/control/rc_config.hpp"
#include "roberta/control/sbus.hpp"

#include <pigpiod_if2.h>

#include <chrono>
#include <csignal>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sstream>
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

class TerminalScreen final {
public:
    TerminalScreen() {
        std::cout << "\033[2J\033[?25l" << std::flush;
    }

    ~TerminalScreen() {
        std::cout << "\033[?25h" << std::endl;
    }
};

struct Options {
    unsigned gpio{17};
    bool gpio_specified{};
    bool telemetry{};
    std::string config_path{"/etc/uv-roberta/rc-drive.conf"};
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
            options.gpio_specified = true;
        } else if (argument == "--telemetry") {
            options.telemetry = true;
        } else if (argument == "--config" && index + 1 < argc) {
            options.config_path = argv[++index];
        } else {
            throw std::invalid_argument(
                "Usage: roberta_rc_monitor [--gpio BCM_PIN] [--telemetry [--config PATH]]");
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

    std::cout << "\033[H"
              << "RC receiver monitor (Ctrl-C to exit)\n"
              << "Input: BCM GPIO " << gpio << " | Status: ";
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
    }
    std::cout << "\033[K\n\n";

    for (std::size_t channel = 0; channel < 16; ++channel) {
        std::cout << "CH" << std::setw(2) << channel + 1 << " : "
                  << std::setw(4) << std::right;
        if (frame) {
            std::cout << frame->channels[channel];
        } else {
            std::cout << "--";
        }
        std::cout << "\033[K\n";
    }
    std::cout << "CH17 : " << std::setw(4) << std::right
              << (frame ? (frame->channel_17 ? "ON" : "OFF") : "--") << "\033[K\n"
              << "CH18 : " << std::setw(4) << std::right
              << (frame ? (frame->channel_18 ? "ON" : "OFF") : "--") << "\033[K\n"
              << "\033[J" << std::flush;
}

std::string format_metric(
    const std::optional<double>& value,
    const std::string& unit,
    int precision = 1) {
    if (!value) {
        return "--";
    }
    std::ostringstream output;
    output << std::fixed << std::setprecision(precision) << *value << unit;
    return output.str();
}

std::string format_channel(
    const std::optional<roberta::control::SbusFrame>& frame,
    std::size_t channel) {
    std::ostringstream output;
    output << "CH" << std::setw(2) << std::setfill('0') << channel + 1
           << " : " << std::setfill(' ') << std::setw(4);
    if (!frame) {
        output << "--";
    } else if (channel < frame->channels.size()) {
        output << frame->channels[channel];
    } else if (channel == 16) {
        output << (frame->channel_17 ? "ON" : "OFF");
    } else {
        output << (frame->channel_18 ? "ON" : "OFF");
    }
    return output.str();
}

void print_telemetry_status(
    ReceiverState& state,
    unsigned gpio,
    const roberta::control::RoboteqTelemetrySnapshot& telemetry,
    const roberta::control::RcDriveConfig& config) {
    using namespace roberta::control;
    std::optional<SbusFrame> frame;
    std::chrono::steady_clock::time_point received_at;
    {
        std::lock_guard lock(state.mutex);
        frame = state.latest_frame;
        received_at = state.received_at;
    }

    std::cout << "\033[H"
              << "RC + Roboteq telemetry monitor (read-only, Ctrl-C to exit)\n"
              << "Input: BCM GPIO " << gpio << " | Status: ";
    if (!frame) {
        std::cout << "WAITING FOR SBUS";
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
    }
    std::cout << "\033[K\n\n"
              << "SBUS channels (raw)                 | Roboteq (read-only)\n"
              << "------------------------------------+--------------------------------------\n";

    const auto left = static_cast<std::size_t>(config.left_motor_channel - 1);
    const auto right = static_cast<std::size_t>(config.right_motor_channel - 1);
    const auto battery_percent = estimate_battery_percent(
        telemetry.battery_volts, config.battery_nominal_volts);
    for (std::size_t row = 0; row < 9; ++row) {
        std::cout << std::left << std::setw(18) << format_channel(frame, row)
                  << " | " << format_channel(frame, row + 9) << "\033[K\n";
    }

    const auto print_side = [&](const std::string& label, std::size_t channel) {
        std::cout << " | " << label << " (CH" << channel + 1 << ")"
                  << "  Batt V(shared) " << format_metric(telemetry.battery_volts, " V")
                  << "  Speed " << format_metric(telemetry.speed_rpm[channel], " RPM", 0)
                  << "\033[K\n"
                  << " | Motor " << format_metric(telemetry.motor_amps[channel], " A")
                  << "  Battery " << format_metric(telemetry.battery_amps[channel], " A")
                  << "  Power " << format_metric(telemetry.power_percent[channel], " %")
                  << "  Temp " << format_metric(telemetry.motor_temperature[channel], " C")
                  << "\033[K\n";
    };
    print_side("Left", left);
    print_side("Right", right);
    std::cout << " | Controller temp: "
              << format_metric(telemetry.controller_temperature, " C")
              << "  Roboteq: " << (telemetry.responses > 0 ? "responding" : "no response")
              << "\033[K\n"
              << " | Battery estimate: ";
    if (battery_percent) {
        std::cout << *battery_percent << '%';
    } else {
        std::cout << "--";
    }
    std::cout << " (nominal " << config.battery_nominal_volts << " V; voltage estimate)"
              << "\033[K\n\033[J" << std::flush;
}

void run_telemetry_monitor(
    ReceiverState& state,
    unsigned gpio,
    roberta::control::LinuxRoboteqTelemetry& roboteq,
    const roberta::control::RcDriveConfig& config) {
    using namespace std::chrono;
    auto next_refresh = steady_clock::now();
    while (!stopping) {
        const auto telemetry = roboteq.refresh();
        print_telemetry_status(state, gpio, telemetry, config);
        next_refresh += seconds(1);
        std::this_thread::sleep_until(next_refresh);
        if (steady_clock::now() > next_refresh + seconds(1)) {
            next_refresh = steady_clock::now();
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    int pigpio_handle = -1;
    int callback_id = -1;
    std::optional<roberta::control::LinuxRoboteqTelemetry> telemetry;
    try {
        const auto options = parse_options(argc, argv);
        std::optional<roberta::control::RcDriveConfig> config;
        if (options.telemetry) {
            config = roberta::control::load_rc_drive_config(options.config_path);
            roberta::control::validate_rc_drive_config(*config, true);
        } else if (options.config_path != "/etc/uv-roberta/rc-drive.conf") {
            throw std::invalid_argument("--config requires --telemetry.");
        }
        const auto gpio = options.gpio_specified || !config
            ? options.gpio
            : static_cast<unsigned>(config->receiver_gpio);
        std::signal(SIGINT, on_signal);
        std::signal(SIGTERM, on_signal);

        pigpio_handle = pigpio_start(nullptr, nullptr);
        if (pigpio_handle < 0) {
            throw std::runtime_error("Cannot connect to pigpiod. Check that the daemon is running.");
        }

        ReceiverState state;
        if (set_mode(
                static_cast<unsigned>(pigpio_handle),
                gpio,
                PI_INPUT) < 0 ||
            (callback_id = callback_ex(
                static_cast<unsigned>(pigpio_handle),
                gpio,
                EITHER_EDGE,
                alert_callback,
                &state)) < 0) {
            throw std::runtime_error(
                "Unable to configure the SBUS GPIO. Check BCM pin numbering and pigpiod.");
        }

        TerminalScreen screen;
        if (options.telemetry) {
            telemetry.emplace(config->roboteq_device, config->roboteq_baud);
            run_telemetry_monitor(state, gpio, *telemetry, *config);
        } else {
            std::cout << "RC receiver monitor only; this program cannot command motors." << std::endl;
            while (!stopping) {
                print_status(state, gpio);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
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
