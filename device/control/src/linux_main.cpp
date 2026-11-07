#include "roberta/control/controller.hpp"
#include "roberta/control/hardware.hpp"
#include "roberta/control/linux_roboteq_driver.hpp"
#include "roberta/control/linux_sbus_receiver.hpp"
#include "roberta/control/linux_zs_x11_driver.hpp"
#include "roberta/control/rc_config.hpp"
#include "roberta/control/sbus.hpp"

#include <pigpiod_if2.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <exception>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace {

volatile std::sig_atomic_t stopping = 0;

void on_signal(int) {
    stopping = 1;
}

struct Options {
    std::string config_path{"/etc/uv-roberta/rc-drive.conf"};
    std::string controller{"roboteq"};
    bool arm{};
    bool monitor{};
    bool calibrate{};
};

Options parse_options(int argc, char** argv) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        if (argument == "--arm") {
            options.arm = true;
        } else if (argument == "--monitor") {
            options.monitor = true;
        } else if (argument == "--calibrate") {
            options.calibrate = true;
        } else if (argument == "--config" && index + 1 < argc) {
            options.config_path = argv[++index];
        } else if (argument == "--controller" && index + 1 < argc) {
            options.controller = argv[++index];
        } else {
            throw std::runtime_error(
                "Usage: roberta_rc_drive [--config PATH] [--controller roboteq|zs] "
                "[--monitor | --calibrate | --arm]");
        }
    }
    if (options.controller != "roboteq" && options.controller != "zs") {
        throw std::runtime_error("--controller must be either roboteq or zs.");
    }
    if (static_cast<int>(options.arm) + static_cast<int>(options.monitor) +
            static_cast<int>(options.calibrate) != 1) {
        throw std::runtime_error("Select exactly one mode: --monitor, --calibrate, or --arm.");
    }
    return options;
}

bool neutral(const roberta::control::RcFrame& frame) {
    constexpr double kNeutralThreshold = 0.05;
    return std::abs(frame.drive.throttle) <= kNeutralThreshold &&
           std::abs(frame.drive.steering) <= kNeutralThreshold;
}

std::uint16_t capture_axis_position(
    roberta::control::LinuxSbusReceiver& receiver,
    unsigned channel,
    const std::string& prompt,
    bool use_maximum) {
    using namespace std::chrono;
    std::cout << prompt << " Then press Enter; hold it there for 2 seconds." << std::endl;
    std::string line;
    if (!std::getline(std::cin, line)) {
        throw std::runtime_error("Calibration input ended unexpectedly.");
    }

    const auto deadline = steady_clock::now() + seconds(2);
    std::vector<std::uint16_t> samples;
    while (steady_clock::now() < deadline && !stopping) {
        const auto now = steady_clock::now();
        const auto frame = receiver.latest_frame();
        if (receiver.read(now) && frame) {
            samples.push_back(frame->channels[channel]);
        }
        std::this_thread::sleep_for(milliseconds(10));
    }
    if (samples.size() < 10) {
        throw std::runtime_error("Too few valid receiver samples. Check the receiver signal and retry.");
    }

    if (use_maximum) {
        return *std::max_element(samples.begin(), samples.end());
    }
    if (prompt.find("center") != std::string::npos) {
        std::sort(samples.begin(), samples.end());
        return samples[samples.size() / 2];
    }
    return *std::min_element(samples.begin(), samples.end());
}

void calibrate_axis(
    roberta::control::LinuxSbusReceiver& receiver,
    unsigned channel,
    const std::string& name,
    roberta::control::AxisCalibration& calibration) {
    std::cout << "\nCalibrating " << name << " on SBUS channel " << channel + 1 << '.' << std::endl;
    calibration.minimum = capture_axis_position(
        receiver, channel, "Move the " + name + " stick to its minimum endpoint.", false);
    calibration.center = capture_axis_position(
        receiver, channel, "Release the " + name + " stick to neutral center.", false);
    calibration.maximum = capture_axis_position(
        receiver, channel, "Move the " + name + " stick to its maximum endpoint.", true);
    if (!roberta::control::valid_calibration(calibration)) {
        throw std::runtime_error("Captured " + name + " endpoints are too close or out of SBUS range.");
    }
    std::cout << name << " calibration: min=" << calibration.minimum
              << " center=" << calibration.center
              << " max=" << calibration.maximum << std::endl;
}

void run_calibration(
    const std::string& config_path,
    roberta::control::RcDriveConfig& config,
    roberta::control::LinuxSbusReceiver& receiver) {
    calibrate_axis(
        receiver,
        config.mapping.throttle_channel,
        "throttle",
        config.mapping.throttle);
    calibrate_axis(
        receiver,
        config.mapping.steering_channel,
        "steering",
        config.mapping.steering);
    roberta::control::save_rc_drive_config(config_path, config);
    std::cout << "\nSaved calibrated ranges to " << config_path << '.' << std::endl;
}

void run_monitor(roberta::control::LinuxSbusReceiver& receiver) {
    std::optional<std::array<std::uint16_t, 16>> last_channels;
    bool previously_valid = false;
    auto next_report = std::chrono::steady_clock::now();

    while (!stopping) {
        const auto now = std::chrono::steady_clock::now();
        const auto frame = receiver.latest_frame();
        const bool fresh = receiver.read(now).has_value();
        if (fresh && frame &&
            (!previously_valid || !last_channels || frame->channels != *last_channels ||
             now >= next_report)) {
            std::cout << "SBUS";
            for (std::size_t index = 0; index < frame->channels.size(); ++index) {
                std::cout << " CH" << (index + 1) << '=' << frame->channels[index];
            }
            std::cout << std::endl;
            last_channels = frame->channels;
            next_report = now + std::chrono::seconds(1);
        } else if (!fresh && previously_valid) {
            std::cout << "No fresh valid receiver frame (signal lost or failsafe)." << std::endl;
        }
        previously_valid = fresh;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void print_drive_table_header(
    const roberta::control::RcDriveConfig& config,
    bool zs_controller) {
    std::cout << "State      | RC CH" << config.mapping.throttle_channel + 1
              << " raw | RC CH" << config.mapping.steering_channel + 1
              << " raw | RC CH10 | Throttle | Steering | Left cmd | Right cmd";
    if (zs_controller) {
        std::cout << " | L PWM (GPIO" << config.zs_left_speed_gpio << ")"
                  << " | L DIR (GPIO" << config.zs_left_direction_gpio << ")"
                  << " | L STOPn (GPIO" << config.zs_left_stop_gpio << ")"
                  << " | R PWM (GPIO" << config.zs_right_speed_gpio << ")"
                  << " | R DIR (GPIO" << config.zs_right_direction_gpio << ")"
                  << " | R STOPn (GPIO" << config.zs_right_stop_gpio << ')';
    }
    std::cout << '\n';
}

void print_drive_table_row(
    const roberta::control::RcDriveConfig& config,
    bool zs_controller,
    const std::optional<roberta::control::SbusFrame>& raw_frame,
    const std::optional<roberta::control::RcFrame>& rc,
    const roberta::control::ControlResult& result,
    bool armed) {
    using namespace roberta::control;

    const char* state = result.fault != ControlFault::None
        ? "FAULT"
        : !rc ? "NO RC" : armed ? "ARMED" : "ARMING";
    std::cout << std::left << std::setw(10) << state << " | " << std::right;
    const auto print_channel = [&raw_frame](unsigned channel) {
        if (raw_frame) {
            std::cout << std::setw(7) << raw_frame->channels[channel];
        } else {
            std::cout << std::setw(7) << "--";
        }
    };
    print_channel(config.mapping.throttle_channel);
    std::cout << " | ";
    print_channel(config.mapping.steering_channel);
    std::cout << " | ";
    print_channel(9);
    std::cout << std::fixed << std::setprecision(2)
              << " | " << std::setw(8) << (rc ? rc->drive.throttle : 0.0)
              << " | " << std::setw(8) << (rc ? rc->drive.steering : 0.0)
              << " | " << std::setw(8) << result.output.left
              << " | " << std::setw(9) << result.output.right;

    if (zs_controller) {
        const auto print_zs_output = [&config, &result](double command, bool left) {
            const bool direction_inverted =
                left ? config.zs_left_direction_inverted : config.zs_right_direction_inverted;
            const int duty = result.motor_enabled
                ? static_cast<int>(std::lround(std::abs(command) * config.zs_max_pwm))
                : 0;
            const int direction = (command >= 0.0) != direction_inverted ? 1 : 0;

            std::cout << " | " << std::setw(3) << duty << '/' << config.zs_max_pwm
                      << " | " << (result.motor_enabled ? std::to_string(direction) : "--")
                      << " | " << (result.motor_enabled ? "1" : "0");
        };
        print_zs_output(result.output.left, true);
        print_zs_output(result.output.right, false);
    }
    std::cout << '\n' << std::flush;
}

void run_armed(
    const roberta::control::RcDriveConfig& config,
    roberta::control::LinuxSbusReceiver& receiver,
    roberta::control::MotorDriver& motor,
    bool zs_controller) {
    using namespace std::chrono;
    using namespace roberta::control;

    Controller controller(ControllerConfig{
        milliseconds(config.receiver_timeout_ms),
        milliseconds(500),
        0.25
    });
    ControlLoop control_loop(std::move(controller), receiver, motor);
    const auto period = milliseconds(20);
    auto next_tick = steady_clock::now();
    steady_clock::time_point neutral_since{};
    bool neutral_timer_active = false;
    std::optional<bool> last_receiver_valid;
    bool armed = false;
    std::optional<ControlFault> last_fault;
    auto next_report = next_tick;
    std::cout << "Waiting for both sticks to remain neutral for "
              << config.arm_neutral_ms << " ms before arming." << std::endl;
    std::cout << "Live RC and motor output values (ZS PWM shown as duty/max; STOPn=1 means enabled):"
              << std::endl;
    print_drive_table_header(config, zs_controller);

    while (!stopping) {
        const auto now = steady_clock::now();
        const auto frame = receiver.read(now);
        ControlResult result{};
        const bool receiver_valid = frame.has_value();
        if (last_receiver_valid && *last_receiver_valid != receiver_valid) {
            if (receiver_valid) {
                std::cerr << "RC receiver frames restored; drive remains disarmed until both sticks are neutral."
                          << std::endl;
            } else {
                std::cerr << "RC receiver lost/failsafe; stopping both motors and disarming."
                          << std::endl;
            }
        }
        if (armed && !receiver_valid) {
            armed = false;
            neutral_timer_active = false;
        }
        last_receiver_valid = receiver_valid;
        if (!armed) {
            if (frame && neutral(*frame)) {
                if (!neutral_timer_active) {
                    neutral_since = now;
                    neutral_timer_active = true;
                } else if (now - neutral_since >= milliseconds(config.arm_neutral_ms)) {
                    armed = true;
                    std::cout << "RC link valid and neutral. Drive control armed." << std::endl;
                }
            } else {
                neutral_timer_active = false;
            }
            motor.stop();
            if (!motor.healthy()) {
                throw std::runtime_error("Motor controller did not accept the startup stop command.");
            }
        } else {
            result = control_loop.tick(now, std::nullopt);
            if (result.fault != last_fault) {
                if (result.fault == ControlFault::MotorUnavailable) {
                    std::cerr << "Motor controller unavailable; output stopped." << std::endl;
                } else if (result.fault == ControlFault::MotorWriteFailed) {
                    std::cerr << "Motor command write failed; output stopped. Restart to re-arm."
                              << std::endl;
                }
                last_fault = result.fault;
            }
            if (!motor.healthy()) {
                throw std::runtime_error("Motor controller output failed. Motor output was stopped.");
            }
        }

        if (now >= next_report) {
            print_drive_table_row(
                config,
                zs_controller,
                receiver.latest_frame(),
                frame,
                result,
                armed);
            next_report = now + milliseconds(200);
        }

        next_tick += period;
        std::this_thread::sleep_until(next_tick);
        if (steady_clock::now() > next_tick + period) {
            next_tick = steady_clock::now();
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    using namespace roberta::control;

    std::unique_ptr<MotorDriver> motor;
    int pigpio_handle = -1;
    try {
        const auto options = parse_options(argc, argv);
        auto config = load_rc_drive_config(options.config_path);
        validate_rc_drive_config(
            config,
            options.arm && options.controller == "roboteq",
            options.arm && options.controller == "zs");

        std::signal(SIGINT, on_signal);
        std::signal(SIGTERM, on_signal);

        pigpio_handle = pigpio_start(nullptr, nullptr);
        if (pigpio_handle < 0) {
            throw std::runtime_error(
                "Cannot connect to pigpiod. Install/enable pigpio and check local GPIO permissions.");
        }

        LinuxSbusReceiver receiver(
            static_cast<unsigned>(pigpio_handle),
            config.receiver_gpio,
            config.mapping,
            std::chrono::milliseconds(config.receiver_timeout_ms));

        if (options.monitor) {
            std::cout << "Receiver monitor only; motors are not opened or commanded." << std::endl;
            run_monitor(receiver);
        } else if (options.calibrate) {
            std::cout << "Calibration only; motors are not opened or commanded." << std::endl;
            run_calibration(options.config_path, config, receiver);
        } else {
            if (options.controller == "roboteq") {
                motor = std::make_unique<LinuxRoboteqDriver>(
                    config.roboteq_device,
                    config.roboteq_baud,
                    config.max_motor_command,
                    config.left_motor_channel,
                    config.right_motor_channel,
                    config.left_motor_inverted,
                    config.right_motor_inverted);
            } else {
                motor = std::make_unique<LinuxZsX11Driver>(
                    static_cast<unsigned>(pigpio_handle), config);
            }
            motor->stop();
            if (!motor->healthy()) {
                throw std::runtime_error("Motor controller did not accept the startup stop command.");
            }
            std::cout << "Starting RC control for " << options.controller
                      << " only; the other controller is not opened." << std::endl;
            run_armed(config, receiver, *motor, options.controller == "zs");
        }
    } catch (const std::exception& error) {
        if (motor) {
            motor->stop();
            motor.reset();
        }
        std::cerr << "roberta_rc_drive: " << error.what() << std::endl;
        if (pigpio_handle >= 0) {
            pigpio_stop(static_cast<unsigned>(pigpio_handle));
        }
        return 1;
    }

    if (motor) {
        motor->stop();
        motor.reset();
    }
    if (pigpio_handle >= 0) {
        pigpio_stop(static_cast<unsigned>(pigpio_handle));
    }
    return 0;
}
