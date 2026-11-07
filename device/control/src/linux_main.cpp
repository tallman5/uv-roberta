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
#include <unistd.h>

namespace {

volatile std::sig_atomic_t stopping = 0;

void on_signal(int) {
    stopping = 1;
}

struct Options {
    std::string config_path{"/etc/uv-roberta/rc-drive.conf"};
    std::string controller{"roboteq"};
    bool controller_specified{};
    bool arm{};
    bool monitor{};
    bool calibrate{};
    bool test_controller{};
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
        } else if (argument == "--test-controller") {
            options.test_controller = true;
        } else if (argument == "--config" && index + 1 < argc) {
            options.config_path = argv[++index];
        } else if (argument == "--controller" && index + 1 < argc) {
            options.controller = argv[++index];
            options.controller_specified = true;
        } else {
            throw std::runtime_error(
                "Usage: roberta_rc_drive [--config PATH] [--controller roboteq|zs] "
                "[--monitor | --calibrate | --arm | --test-controller]");
        }
    }
    if (options.controller != "roboteq" && options.controller != "zs") {
        throw std::runtime_error("--controller must be either roboteq or zs.");
    }
    if (static_cast<int>(options.arm) + static_cast<int>(options.monitor) +
            static_cast<int>(options.calibrate) + static_cast<int>(options.test_controller) != 1) {
        throw std::runtime_error(
            "Select exactly one mode: --monitor, --calibrate, --arm, or --test-controller.");
    }
    if (options.test_controller && !options.controller_specified) {
        throw std::runtime_error("--test-controller requires an explicit --controller selection.");
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

void print_drive_table(
    const roberta::control::RcDriveConfig& config,
    bool zs_controller,
    const std::optional<roberta::control::SbusFrame>& raw_frame,
    const std::optional<roberta::control::RcFrame>& rc,
    const roberta::control::ControlResult& result,
    bool armed,
    bool interactive) {
    using namespace roberta::control;

    const char* state = result.fault != ControlFault::None
        ? "FAULT"
        : !rc ? "NO RC" : armed ? "ARMED" : "ARMING";
    std::cout << "RC channels and drive outputs | " << state
              << (interactive ? "\033[K" : "") << '\n';

    const auto print_row = [interactive](
                               const std::string& channel,
                               const std::string& channel_value,
                               const std::string& parameter = {},
                               const std::string& parameter_value = {}) {
        std::cout << std::left << std::setw(14) << channel
                  << std::setw(10) << channel_value
                  << std::setw(20) << parameter << parameter_value
                  << (interactive ? "\033[K" : "") << '\n';
    };

    std::cout << std::left << std::setw(14) << "RC / drive" << std::setw(10) << "Value"
              << std::setw(20) << (zs_controller ? "ZS parameter" : "Drive output")
              << "Value" << (interactive ? "\033[K" : "") << '\n';
    for (std::size_t channel = 0; channel < 18; ++channel) {
        std::string label = "CH";
        if (channel + 1 < 10) {
            label += '0';
        }
        label += std::to_string(channel + 1);
        std::string value = "--";
        if (raw_frame) {
            if (channel < raw_frame->channels.size()) {
                value = std::to_string(raw_frame->channels[channel]);
            } else if (channel == 16) {
                value = raw_frame->channel_17 ? "ON" : "OFF";
            } else {
                value = raw_frame->channel_18 ? "ON" : "OFF";
            }
        }

        std::string parameter;
        std::string parameter_value;
        if (zs_controller && channel == 0) {
            parameter = "ZS LEFT";
        } else if (zs_controller && channel == 1) {
            parameter = "PWM (GPIO " + std::to_string(config.zs_left_speed_gpio) + ")";
            const int duty = result.motor_enabled
                ? static_cast<int>(std::lround(std::abs(result.output.left) * config.zs_max_pwm))
                : 0;
            parameter_value = std::to_string(duty) + '/' + std::to_string(config.zs_max_pwm);
        } else if (zs_controller && channel == 2) {
            parameter = "DIR (GPIO " + std::to_string(config.zs_left_direction_gpio) + ")";
            const int direction =
                (result.output.left >= 0.0) != config.zs_left_direction_inverted ? 1 : 0;
            parameter_value = result.motor_enabled ? std::to_string(direction) : "--";
        } else if (zs_controller && channel == 3) {
            parameter = "STOPn (GPIO " + std::to_string(config.zs_left_stop_gpio) + ")";
            parameter_value = result.motor_enabled ? "1" : "0";
        } else if (zs_controller && channel == 4) {
            parameter = "ZS RIGHT";
        } else if (zs_controller && channel == 5) {
            parameter = "PWM (GPIO " + std::to_string(config.zs_right_speed_gpio) + ")";
            const int duty = result.motor_enabled
                ? static_cast<int>(std::lround(std::abs(result.output.right) * config.zs_max_pwm))
                : 0;
            parameter_value = std::to_string(duty) + '/' + std::to_string(config.zs_max_pwm);
        } else if (zs_controller && channel == 6) {
            parameter = "DIR (GPIO " + std::to_string(config.zs_right_direction_gpio) + ")";
            const int direction =
                (result.output.right >= 0.0) != config.zs_right_direction_inverted ? 1 : 0;
            parameter_value = result.motor_enabled ? std::to_string(direction) : "--";
        } else if (zs_controller && channel == 7) {
            parameter = "STOPn (GPIO " + std::to_string(config.zs_right_stop_gpio) + ")";
            parameter_value = result.motor_enabled ? "1" : "0";
        }
        print_row(label, value, parameter, parameter_value);
    }

    print_row("Throttle norm", std::to_string(rc ? rc->drive.throttle : 0.0));
    print_row("Steering norm", std::to_string(rc ? rc->drive.steering : 0.0));
    print_row("Left command", std::to_string(result.output.left));
    print_row("Right command", std::to_string(result.output.right));

    std::cout << (interactive ? "\033[J" : "") << std::flush;
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
    const bool interactive = isatty(STDOUT_FILENO) != 0;
    if (interactive) {
        std::cout << "\033[2J\033[H";
    } else {
        std::cout << "Waiting for both sticks to remain neutral for "
                  << config.arm_neutral_ms << " ms before arming." << std::endl;
    }

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
                    std::cerr << "RC link valid and neutral. Drive control armed." << std::endl;
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
            if (interactive) {
                std::cout << "\033[H";
            }
            print_drive_table(
                config,
                zs_controller,
                receiver.latest_frame(),
                frame,
                result,
                armed,
                interactive);
            next_report = now + (interactive ? milliseconds(200) : seconds(1));
        }

        next_tick += period;
        std::this_thread::sleep_until(next_tick);
        if (steady_clock::now() > next_tick + period) {
            next_tick = steady_clock::now();
        }
    }
}

void run_controller_test(
    const roberta::control::RcDriveConfig& config,
    roberta::control::MotorDriver& motor,
    bool zs_controller) {
    using namespace std::chrono_literals;
    using namespace roberta::control;

    if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
        throw std::runtime_error(
            "--test-controller requires an interactive terminal for its safety confirmation.");
    }

    constexpr int kMaxTestPwm = 51;
    constexpr int kMaxTestRoboteqCommand = 100;
    const double test_level = zs_controller
        ? static_cast<double>(std::min(config.zs_max_pwm, kMaxTestPwm)) / config.zs_max_pwm
        : static_cast<double>(std::min(config.max_motor_command, kMaxTestRoboteqCommand)) /
              config.max_motor_command;
    const int test_pwm = zs_controller
        ? std::min(config.zs_max_pwm, kMaxTestPwm)
        : 0;
    const int test_command = zs_controller
        ? 0
        : static_cast<int>(std::lround(test_level * config.max_motor_command));

    std::cout << "CONTROLLER TEST: wheels must be securely raised, people clear, and the "
                 "physical emergency stop within reach.\n"
              << "Each wheel will pulse forward and reverse separately for 1 second, "
              << "capped at " << (zs_controller
                                      ? std::to_string(test_pwm) + "/255 PWM"
                                      : std::to_string(test_command) + " command units")
              << ". Type TEST to continue: " << std::flush;
    std::string confirmation;
    if (!std::getline(std::cin, confirmation) || confirmation != "TEST") {
        motor.stop();
        std::cout << "\nController test cancelled; outputs stopped." << std::endl;
        return;
    }

    struct TestStep {
        const char* name;
        MotorOutput output;
    };
    const TestStep steps[] = {
        {"left forward", {test_level, 0.0}},
        {"left reverse", {-test_level, 0.0}},
        {"right forward", {0.0, test_level}},
        {"right reverse", {0.0, -test_level}}
    };

    for (const auto& step : steps) {
        if (stopping) {
            break;
        }
        std::cout << "Testing " << step.name << "..." << std::endl;
        if (!motor.healthy() || !motor.apply(step.output)) {
            motor.stop();
            throw std::runtime_error("Motor controller rejected the " + std::string(step.name) +
                                     " test command.");
        }
        std::this_thread::sleep_for(1s);
        motor.stop();
        if (!motor.healthy()) {
            throw std::runtime_error("Motor controller failed while stopping after the " +
                                     std::string(step.name) + " test.");
        }
        std::this_thread::sleep_for(300ms);
    }

    motor.stop();
    if (!motor.healthy()) {
        throw std::runtime_error("Motor controller failed to stop after the controller test.");
    }
    std::cout << (stopping
                      ? "Controller test interrupted; outputs stopped."
                      : "Controller test commands complete; physical wheel movement is not detected. "
                        "Outputs stopped.")
              << std::endl;
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
            (options.arm || options.test_controller) && options.controller == "roboteq",
            (options.arm || options.test_controller) && options.controller == "zs");

        std::signal(SIGINT, on_signal);
        std::signal(SIGTERM, on_signal);

        if (options.controller == "zs" || !options.test_controller) {
            pigpio_handle = pigpio_start(nullptr, nullptr);
            if (pigpio_handle < 0) {
                throw std::runtime_error(
                    "Cannot connect to pigpiod. Install/enable pigpio and check local GPIO permissions.");
            }
        }

        std::unique_ptr<LinuxSbusReceiver> receiver;
        if (!options.test_controller) {
            receiver = std::make_unique<LinuxSbusReceiver>(
                static_cast<unsigned>(pigpio_handle),
                config.receiver_gpio,
                config.mapping,
                std::chrono::milliseconds(config.receiver_timeout_ms));
        }

        if (options.monitor) {
            std::cout << "Receiver monitor only; motors are not opened or commanded." << std::endl;
            run_monitor(*receiver);
        } else if (options.calibrate) {
            std::cout << "Calibration only; motors are not opened or commanded." << std::endl;
            run_calibration(options.config_path, config, *receiver);
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
            if (options.test_controller) {
                run_controller_test(config, *motor, options.controller == "zs");
            } else {
                std::cout << "Starting RC control for " << options.controller
                          << " only; the other controller is not opened." << std::endl;
                run_armed(config, *receiver, *motor, options.controller == "zs");
            }
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
