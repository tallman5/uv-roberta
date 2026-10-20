#include "roberta/control/controller.hpp"
#include "roberta/control/hardware.hpp"
#include "roberta/control/rc_config.hpp"
#include "roberta/control/rc_mapping.hpp"
#include "roberta/control/sbus.hpp"
#include "roberta/control/roboteq_telemetry.hpp"

#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std::chrono_literals;
using roberta::control::ControlSource;
using roberta::control::Controller;
using roberta::control::ControllerConfig;
using roberta::control::ControlFault;
using roberta::control::ControlLoop;
using roberta::control::DashboardCommand;
using roberta::control::DriveInput;
using roberta::control::MotorDriver;
using roberta::control::MotorOutput;
using roberta::control::MonotonicTime;
using roberta::control::RcReceiver;
using roberta::control::RcFrame;
using roberta::control::RcMapping;
using roberta::control::SbusDecoder;
using roberta::control::SbusEdgeDecoder;
using roberta::control::SbusFrame;

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void require_output(MotorOutput actual, MotorOutput expected, const std::string& name) {
    require(std::abs(actual.left - expected.left) < 1e-9, name + ": left output mismatch");
    require(std::abs(actual.right - expected.right) < 1e-9, name + ": right output mismatch");
}

void run(const std::string& name, const auto& test) {
    test();
    std::cout << "PASS " << name << '\n';
}

class FakeReceiver final : public RcReceiver {
public:
    std::optional<RcFrame> frame;

    std::optional<RcFrame> read(MonotonicTime) noexcept override {
        return frame;
    }
};

class FakeMotor final : public MotorDriver {
public:
    bool is_healthy{true};
    bool accept_output{true};
    bool stopped{};
    MotorOutput last_output{};

    bool healthy() const noexcept override {
        return is_healthy;
    }

    bool apply(const MotorOutput& output) noexcept override {
        last_output = output;
        return accept_output;
    }

    void stop() noexcept override {
        stopped = true;
        last_output = {};
    }
};

std::array<std::uint8_t, 25> encode_sbus(const std::array<std::uint16_t, 16>& channels, std::uint8_t flags = 0) {
    std::array<std::uint8_t, 25> bytes{};
    bytes[0] = 0x0F;
    for (std::size_t channel = 0; channel < channels.size(); ++channel) {
        for (std::size_t bit = 0; bit < 11; ++bit) {
            if ((channels[channel] & (1U << bit)) != 0) {
                const std::size_t target_bit = channel * 11 + bit;
                bytes[1 + target_bit / 8] |= static_cast<std::uint8_t>(1U << (target_bit % 8));
            }
        }
    }
    bytes[23] = flags;
    bytes[24] = 0;
    return bytes;
}

std::array<bool, 300> encode_gpio_bits(const std::array<std::uint8_t, 25>& bytes) {
    std::array<bool, 300> bits{};
    for (std::size_t byte = 0; byte < bytes.size(); ++byte) {
        const auto offset = byte * 12;
        bits[offset] = false;
        unsigned ones = 0;
        for (std::size_t bit = 0; bit < 8; ++bit) {
            bits[offset + bit + 1] = (bytes[byte] & (1U << bit)) != 0;
            ones += bits[offset + bit + 1] ? 1U : 0U;
        }
        bits[offset + 9] = ones % 2U != 0;
        bits[offset + 10] = true;
        bits[offset + 11] = true;
    }
    return bits;
}

}  // namespace

int main() {
    try {
        const Controller controller;
        const MonotonicTime now{};

        run("RC source drives without blending", [&] {
            const RcFrame rc{{0.5, 0.25}, ControlSource::Rc, false, now};
            const DashboardCommand dashboard{{-1.0, 0.0}, now};
            require_output(controller.update(now, rc, dashboard), {0.75, 0.25}, "RC priority");
        });

        run("dashboard source is selected by the RC switch", [&] {
            const RcFrame rc{{1.0, 0.0}, ControlSource::Dashboard, false, now};
            const DashboardCommand dashboard{{0.5, -0.25}, now};
            require_output(controller.update(now, rc, dashboard), {0.25, 0.75}, "dashboard priority");
        });

        run("slow mode limits both sources to 25 percent", [&] {
            const RcFrame rc{{1.0, 0.0}, ControlSource::Rc, true, now};
            require_output(controller.update(now, rc, std::nullopt), {0.25, 0.25}, "RC slow mode");
            const RcFrame dashboard_selected{{0.0, 0.0}, ControlSource::Dashboard, true, now};
            const DashboardCommand dashboard{{1.0, 0.0}, now};
            require_output(controller.update(now, dashboard_selected, dashboard), {0.25, 0.25}, "dashboard slow mode");
        });

        run("motor command scale caps mixed output", [&] {
            const RcFrame low{{1.0, 1.0}, ControlSource::Rc, false, now, 1.0 / 3.0};
            const RcFrame medium{{1.0, 0.0}, ControlSource::Rc, false, now, 2.0 / 3.0};
            const RcFrame high{{1.0, 0.0}, ControlSource::Rc, false, now, 1.0};
            require_output(controller.update(now, low, std::nullopt), {1.0 / 3.0, 0.0}, "low switch position");
            require_output(controller.update(now, medium, std::nullopt), {2.0 / 3.0, 2.0 / 3.0}, "medium switch position");
            require_output(controller.update(now, high, std::nullopt), {1.0, 1.0}, "high switch position");

            const RcFrame invalid_scale{{1.0, 0.0}, ControlSource::Rc, false, now, 1.1};
            require_output(controller.update(now, invalid_scale, std::nullopt), {}, "invalid motor command scale");
        });

        run("stale RC input stops regardless of selected source", [&] {
            const RcFrame stale{{1.0, 0.0}, ControlSource::Dashboard, false, now - 251ms};
            const DashboardCommand dashboard{{1.0, 0.0}, now};
            require_output(controller.update(now, stale, dashboard), {}, "stale RC");
            require_output(controller.update(now, std::nullopt, dashboard), {}, "missing RC");
        });

        run("missing or stale dashboard command stops", [&] {
            const RcFrame rc{{0.0, 0.0}, ControlSource::Dashboard, false, now};
            require_output(controller.update(now, rc, std::nullopt), {}, "missing dashboard command");
            const DashboardCommand stale{{1.0, 0.0}, now - 501ms};
            require_output(controller.update(now, rc, stale), {}, "stale dashboard command");
        });

        run("future input and out-of-range input stop", [&] {
            const RcFrame future{{1.0, 0.0}, ControlSource::Rc, false, now + 1ms};
            require_output(controller.update(now, future, std::nullopt), {}, "future RC frame");
            const RcFrame invalid{{1.01, 0.0}, ControlSource::Rc, false, now};
            require_output(controller.update(now, invalid, std::nullopt), {}, "invalid RC input");
            const RcFrame dashboard_selected{{0.0, 0.0}, ControlSource::Dashboard, false, now};
            const DashboardCommand nan_command{{std::numeric_limits<double>::quiet_NaN(), 0.0}, now};
            require_output(controller.update(now, dashboard_selected, nan_command), {}, "NaN dashboard input");
        });

        run("motor mixing saturates at the output bounds", [&] {
            const RcFrame rc{{1.0, 1.0}, ControlSource::Rc, false, now};
            require_output(controller.update(now, rc, std::nullopt), {1.0, 0.0}, "mixed output");
            const RcFrame unknown_source{{1.0, 0.0}, static_cast<ControlSource>(99), false, now};
            require_output(controller.update(now, unknown_source, std::nullopt), {}, "unknown source");
        });

        run("invalid controller configuration is rejected", [&] {
            bool threw = false;
            try {
                static_cast<void>(Controller(ControllerConfig{250ms, 500ms, 1.5}));
            } catch (const std::invalid_argument&) {
                threw = true;
            }
            require(threw, "out-of-range slow-mode scale was accepted");
        });

        run("control loop applies selected input and stops on lost RC", [&] {
            FakeReceiver receiver;
            FakeMotor motor;
            receiver.frame = RcFrame{{0.5, 0.0}, ControlSource::Rc, false, now};
            ControlLoop loop(Controller{}, receiver, motor);

            const auto moving = loop.tick(now, std::nullopt);
            require(moving.motor_enabled, "valid RC drive was not applied");
            require_output(motor.last_output, {0.5, 0.5}, "control loop output");

            receiver.frame.reset();
            const auto stopped = loop.tick(now, std::nullopt);
            require(!stopped.motor_enabled, "missing RC input left motors enabled");
            require(motor.stopped, "missing RC input did not call motor stop");
        });

        run("control loop stops and reports motor driver faults", [&] {
            FakeReceiver receiver;
            FakeMotor motor;
            receiver.frame = RcFrame{{0.5, 0.0}, ControlSource::Rc, false, now};
            ControlLoop loop(Controller{}, receiver, motor);

            motor.is_healthy = false;
            require(loop.tick(now, std::nullopt).fault == ControlFault::MotorUnavailable,
                "unhealthy motor driver was not reported");

            motor.is_healthy = true;
            motor.accept_output = false;
            require(loop.tick(now, std::nullopt).fault == ControlFault::MotorWriteFailed,
                "motor output failure was not reported");
            require(motor.stopped, "motor output failure did not call stop");
        });

        run("SBUS decoder unpacks channels and receiver failsafe flags", [&] {
            std::array<std::uint16_t, 16> channels{};
            channels[0] = 172;
            channels[1] = 1811;
            channels[15] = 1001;
            const auto bytes = encode_sbus(channels);
            SbusDecoder decoder;
            std::optional<SbusFrame> decoded;
            for (const auto byte : bytes) {
                if (auto frame = decoder.feed(byte)) {
                    decoded = frame;
                }
            }
            require(decoded.has_value(), "valid SBUS frame was not decoded");
            require(decoded->channels == channels, "SBUS channel values were decoded incorrectly");

            const auto digital = encode_sbus(channels, 0x03);
            decoded.reset();
            for (const auto byte : digital) {
                if (auto frame = decoder.feed(byte)) {
                    decoded = frame;
                }
            }
            require(
                decoded && decoded->channel_17 && decoded->channel_18,
                "SBUS digital channels 17 and 18 were not decoded");

            const auto lost = encode_sbus(channels, 0x04);
            decoded.reset();
            for (const auto byte : lost) {
                if (auto frame = decoder.feed(byte)) {
                    decoded = frame;
                }
            }
            require(decoded && decoded->frame_lost, "SBUS frame-lost flag was ignored");
        });

        run("SBUS GPIO decoder validates inverted UART framing and parity", [&] {
            std::array<std::uint16_t, 16> channels{};
            channels[0] = 990;
            channels[1] = 1010;
            const auto bytes = encode_sbus(channels);
            auto bits = encode_gpio_bits(bytes);
            const auto decoded = roberta::control::decode_sbus_gpio_bits(bits);
            require(decoded && decoded->channels == channels, "GPIO SBUS bits failed to decode");

            bits[20] = !bits[20];
            require(!roberta::control::decode_sbus_gpio_bits(bits), "corrupt SBUS parity was accepted");
        });

        run("SBUS edge decoder reconstructs frames from inverted GPIO transitions", [&] {
            std::array<std::uint16_t, 16> channels{};
            channels[0] = 700;
            channels[1] = 1300;
            const auto bits = encode_gpio_bits(encode_sbus(channels));
            SbusEdgeDecoder decoder;
            int physical_level = 0;
            std::optional<SbusFrame> decoded;
            constexpr std::uint32_t start_tick = 1000;

            for (std::size_t bit = 0; bit < bits.size(); ++bit) {
                const int next_level = bits[bit] ? 0 : 1;
                if (next_level != physical_level) {
                    decoded = decoder.feed(
                        next_level,
                        start_tick + static_cast<std::uint32_t>(bit * 10));
                    physical_level = next_level;
                }
            }

            decoded = decoder.feed(
                1,
                start_tick + static_cast<std::uint32_t>(bits.size() * 10 + 4000));
            require(decoded && decoded->channels == channels, "GPIO transitions did not reconstruct SBUS frame");
        });

        run("RC stick calibration maps centers and endpoints safely", [&] {
            const RcMapping mapping{
                1,
                0,
                {172, 992, 1811, false},
                {172, 992, 1811, true},
                0.04
            };
            SbusFrame frame;
            frame.channels[1] = 992;
            frame.channels[0] = 992;
            frame.channels[9] = 992;
            auto drive = roberta::control::map_sbus_drive(frame, mapping);
            require_output({drive.throttle, drive.steering}, {}, "neutral sticks");

            frame.channels[1] = 1811;
            frame.channels[0] = 172;
            drive = roberta::control::map_sbus_drive(frame, mapping);
            require(std::abs(drive.throttle - 1.0) < 1e-9, "forward endpoint did not map to full throttle");
            require(std::abs(drive.steering - 1.0) < 1e-9, "inverted steering endpoint did not map correctly");

            frame.failsafe = true;
            drive = roberta::control::map_sbus_drive(frame, mapping);
            require_output({drive.throttle, drive.steering}, {}, "failsafe frame");
        });

        run("channel 10 selects low medium and high motor limits", [&] {
            using roberta::control::motor_command_scale;
            require(std::abs(motor_command_scale(172) - 1.0 / 3.0) < 1e-9, "low switch limit incorrect");
            require(std::abs(motor_command_scale(992) - 2.0 / 3.0) < 1e-9, "medium switch limit incorrect");
            require(std::abs(motor_command_scale(1811) - 1.0) < 1e-9, "high switch limit incorrect");
            require(motor_command_scale(171) == 0.0, "out-of-range switch value was accepted");

            const RcMapping mapping{
                1,
                0,
                {172, 992, 1811, false},
                {172, 992, 1811, false},
                0.04
            };
            SbusFrame frame;
            frame.channels[1] = 992;
            frame.channels[0] = 992;
            frame.channels[9] = 0;
            require(
                !roberta::control::valid_sbus_drive_channels(frame, mapping),
                "invalid channel 10 value was accepted");
        });

        run("Roboteq telemetry values are parsed and battery level is estimated", [&] {
            using namespace roberta::control;
            const auto volts = parse_roboteq_pair("V=240:238", "V=", 0.1);
            require(volts && (*volts)[0] == 24.0 && (*volts)[1] == 23.8,
                "battery-voltage response was not scaled correctly");
            const auto voltage_values = parse_roboteq_values("V=0:238:240", "V=", 0.1);
            require(voltage_values && voltage_values->size() == 3 &&
                    (*voltage_values)[1] == 23.8,
                "battery-voltage field was not parsed from an extended response");
            const auto temperatures = parse_roboteq_triple("T=31:32:33", "T=");
            require(temperatures && (*temperatures)[2] == 33.0,
                "temperature response was not parsed correctly");
            require(!parse_roboteq_pair("V=240", "V="),
                "incomplete channel response was accepted");
            const std::optional<double> full_battery = 27.0;
            const std::optional<double> empty_battery = 21.0;
            require(estimate_battery_percent(full_battery, 24.0) == 100,
                "full battery voltage did not clamp to 100 percent");
            require(estimate_battery_percent(empty_battery, 24.0) == 0,
                "low battery voltage did not clamp to 0 percent");
            require(!estimate_battery_percent(std::nullopt, 24.0),
                "battery percentage was reported without voltage readings");
        });

        run("RC configuration is parsed and validated before arming", [&] {
            const roberta::control::RcDriveConfig defaults;
            require(
                defaults.roboteq_device ==
                    "/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00",
                "default Roboteq device path was incorrect");

            const auto path = std::filesystem::temp_directory_path() / "roberta-rc-config-test.conf";
            const auto write_config = [&](const std::string& roboteq_device) {
                std::ofstream output(path);
                output << "receiver_gpio=17\n"
                       << "throttle_channel=2\n"
                       << "throttle_min=172\n"
                       << "throttle_center=992\n"
                       << "throttle_max=1811\n"
                       << "throttle_inverted=false\n"
                       << "steering_channel=1\n"
                       << "steering_min=180\n"
                       << "steering_center=1000\n"
                       << "steering_max=1800\n"
                       << "steering_inverted=true\n"
                       << "deadband=0.05\n"
                       << "receiver_timeout_ms=100\n"
                       << "roboteq_device=" << roboteq_device << '\n'
                       << "roboteq_baud=9600\n"
                       << "battery_nominal_volts=24.5\n"
                       << "max_motor_command=200\n"
                       << "left_motor_channel=2\n"
                       << "right_motor_channel=1\n"
                       << "left_motor_inverted=false\n"
                       << "right_motor_inverted=true\n"
                       << "arm_neutral_ms=2000\n";
            };

            write_config("/dev/serial/by-id/controller");
            const auto config = roberta::control::load_rc_drive_config(path.string());
            require(config.mapping.throttle_channel == 1, "one-based throttle channel was parsed incorrectly");
            require(config.mapping.steering_channel == 0, "one-based steering channel was parsed incorrectly");
            require(config.mapping.steering.inverted, "steering inversion was parsed incorrectly");
            require(
                config.roboteq_device == "/dev/serial/by-id/controller",
                "Roboteq device path override was not parsed");
            require(config.battery_nominal_volts == 24.5,
                "nominal battery voltage was not parsed");
            require(config.left_motor_channel == 2 && config.right_motor_channel == 1,
                "Roboteq motor-channel mapping was parsed incorrectly");
            roberta::control::validate_rc_drive_config(config, true);

            write_config("");
            const auto default_path_config = roberta::control::load_rc_drive_config(path.string());
            require(
                default_path_config.roboteq_device == defaults.roboteq_device,
                "empty Roboteq device setting did not use the default");
            std::filesystem::remove(path);
        });

        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
