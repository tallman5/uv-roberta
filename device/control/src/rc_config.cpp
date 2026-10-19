#include "roberta/control/rc_config.hpp"

#include <charconv>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include <string_view>

namespace roberta::control {
namespace {

std::string trim(std::string value) {
    const auto first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return {};
    }
    const auto last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

int parse_int(const std::string& value, const std::string& key) {
    int result{};
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::runtime_error("Invalid integer value for " + key);
    }
    return result;
}

double parse_double(const std::string& value, const std::string& key) {
    std::size_t used{};
    double result{};
    try {
        result = std::stod(value, &used);
    } catch (const std::exception&) {
        throw std::runtime_error("Invalid number for " + key);
    }
    if (used != value.size() || !std::isfinite(result)) {
        throw std::runtime_error("Invalid number for " + key);
    }
    return result;
}

bool parse_bool(const std::string& value, const std::string& key) {
    if (value == "true") return true;
    if (value == "false") return false;
    throw std::runtime_error("Expected true or false for " + key);
}

std::uint16_t parse_channel_value(const std::string& value, const std::string& key) {
    const int parsed = parse_int(value, key);
    if (parsed < 0 || parsed > 2047) {
        throw std::runtime_error(key + " must be an SBUS channel value from 0 through 2047.");
    }
    return static_cast<std::uint16_t>(parsed);
}

}  // namespace

RcDriveConfig load_rc_drive_config(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Cannot read config file: " + path);
    }

    RcDriveConfig config;
    bool has_throttle_channel = false;
    bool has_steering_channel = false;
    bool has_throttle_calibration = false;
    bool has_steering_calibration = false;
    std::string line;
    std::size_t line_number = 0;
    while (std::getline(input, line)) {
        ++line_number;
        auto content = trim(line);
        if (content.empty() || content.front() == '#' || content.front() == ';') {
            continue;
        }
        const auto separator = content.find('=');
        if (separator == std::string::npos) {
            throw std::runtime_error("Expected key=value on config line " + std::to_string(line_number));
        }

        const auto key = trim(content.substr(0, separator));
        const auto value = trim(content.substr(separator + 1));
        if (key == "receiver_gpio") {
            config.receiver_gpio = parse_int(value, key);
        } else if (key == "throttle_channel") {
            const int channel = parse_int(value, key);
            config.mapping.throttle_channel = channel > 0 ? static_cast<unsigned>(channel - 1) : 16;
            has_throttle_channel = true;
        } else if (key == "steering_channel") {
            const int channel = parse_int(value, key);
            config.mapping.steering_channel = channel > 0 ? static_cast<unsigned>(channel - 1) : 16;
            has_steering_channel = true;
        } else if (key == "throttle_min") {
            config.mapping.throttle.minimum = parse_channel_value(value, key);
            has_throttle_calibration = true;
        } else if (key == "throttle_center") {
            config.mapping.throttle.center = parse_channel_value(value, key);
            has_throttle_calibration = true;
        } else if (key == "throttle_max") {
            config.mapping.throttle.maximum = parse_channel_value(value, key);
            has_throttle_calibration = true;
        } else if (key == "throttle_inverted") {
            config.mapping.throttle.inverted = parse_bool(value, key);
        } else if (key == "steering_min") {
            config.mapping.steering.minimum = parse_channel_value(value, key);
            has_steering_calibration = true;
        } else if (key == "steering_center") {
            config.mapping.steering.center = parse_channel_value(value, key);
            has_steering_calibration = true;
        } else if (key == "steering_max") {
            config.mapping.steering.maximum = parse_channel_value(value, key);
            has_steering_calibration = true;
        } else if (key == "steering_inverted") {
            config.mapping.steering.inverted = parse_bool(value, key);
        } else if (key == "deadband") {
            config.mapping.deadband = parse_double(value, key);
        } else if (key == "receiver_timeout_ms") {
            config.receiver_timeout_ms = parse_int(value, key);
        } else if (key == "roboteq_device") {
            config.roboteq_device = value;
        } else if (key == "roboteq_baud") {
            config.roboteq_baud = parse_int(value, key);
        } else if (key == "max_motor_command") {
            config.max_motor_command = parse_int(value, key);
        } else if (key == "left_motor_channel") {
            config.left_motor_channel = parse_int(value, key);
        } else if (key == "right_motor_channel") {
            config.right_motor_channel = parse_int(value, key);
        } else if (key == "left_motor_inverted") {
            config.left_motor_inverted = parse_bool(value, key);
        } else if (key == "right_motor_inverted") {
            config.right_motor_inverted = parse_bool(value, key);
        } else if (key == "arm_neutral_ms") {
            config.arm_neutral_ms = parse_int(value, key);
        } else {
            throw std::runtime_error("Unknown config key on line " + std::to_string(line_number) + ": " + key);
        }
    }

    if (!has_throttle_channel || !has_steering_channel ||
        !has_throttle_calibration || !has_steering_calibration) {
        throw std::runtime_error(
            "Config must include throttle_channel, steering_channel, and min/center/max calibration for both axes.");
    }
    validate_rc_drive_config(config, false);
    return config;
}

void validate_rc_drive_config(const RcDriveConfig& config, bool require_motor_device) {
    if (config.receiver_gpio < 0 || config.receiver_gpio > 27) {
        throw std::runtime_error("receiver_gpio must be a BCM GPIO number from 0 through 27.");
    }
    if (!valid_mapping(config.mapping)) {
        throw std::runtime_error(
            "Stick channels must be distinct channels 1-16; each calibration needs valid SBUS min/center/max; deadband must be below 0.25.");
    }
    if (config.receiver_timeout_ms < 30 || config.receiver_timeout_ms > 500) {
        throw std::runtime_error("receiver_timeout_ms must be between 30 and 500.");
    }
    if (require_motor_device && config.roboteq_device.empty()) {
        throw std::runtime_error("roboteq_device must name the MDC2250 serial device.");
    }
    if (config.roboteq_baud != 9600 && config.roboteq_baud != 19200 &&
        config.roboteq_baud != 38400 && config.roboteq_baud != 57600 &&
        config.roboteq_baud != 115200) {
        throw std::runtime_error("roboteq_baud must be 9600, 19200, 38400, 57600, or 115200.");
    }
    if (config.max_motor_command < 1 || config.max_motor_command > 1000) {
        throw std::runtime_error("max_motor_command must be between 1 and 1000.");
    }
    if ((config.left_motor_channel != 1 && config.left_motor_channel != 2) ||
        (config.right_motor_channel != 1 && config.right_motor_channel != 2) ||
        config.left_motor_channel == config.right_motor_channel) {
        throw std::runtime_error("left_motor_channel and right_motor_channel must be different channels 1 and 2.");
    }
    if (config.arm_neutral_ms < 1000 || config.arm_neutral_ms > 10000) {
        throw std::runtime_error("arm_neutral_ms must be between 1000 and 10000.");
    }
}

void save_rc_drive_config(const std::string& path, const RcDriveConfig& config) {
    validate_rc_drive_config(config, false);
    const std::filesystem::path destination(path);
    auto temporary = destination;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output) {
            throw std::runtime_error("Cannot write temporary config file: " + temporary.string());
        }
        output << "# RC-only Phase 1 drive configuration. Calibrate the sticks before arming.\n"
               << "receiver_gpio=" << config.receiver_gpio << '\n'
               << "throttle_channel=" << config.mapping.throttle_channel + 1 << '\n'
               << "throttle_min=" << config.mapping.throttle.minimum << '\n'
               << "throttle_center=" << config.mapping.throttle.center << '\n'
               << "throttle_max=" << config.mapping.throttle.maximum << '\n'
               << "throttle_inverted=" << (config.mapping.throttle.inverted ? "true" : "false") << '\n'
               << "steering_channel=" << config.mapping.steering_channel + 1 << '\n'
               << "steering_min=" << config.mapping.steering.minimum << '\n'
               << "steering_center=" << config.mapping.steering.center << '\n'
               << "steering_max=" << config.mapping.steering.maximum << '\n'
               << "steering_inverted=" << (config.mapping.steering.inverted ? "true" : "false") << '\n'
               << std::fixed << std::setprecision(3)
               << "deadband=" << config.mapping.deadband << '\n'
               << "receiver_timeout_ms=" << config.receiver_timeout_ms << '\n'
               << "roboteq_device=" << config.roboteq_device << '\n'
               << "roboteq_baud=" << config.roboteq_baud << '\n'
               << "max_motor_command=" << config.max_motor_command << '\n'
               << "left_motor_channel=" << config.left_motor_channel << '\n'
               << "right_motor_channel=" << config.right_motor_channel << '\n'
               << "left_motor_inverted=" << (config.left_motor_inverted ? "true" : "false") << '\n'
               << "right_motor_inverted=" << (config.right_motor_inverted ? "true" : "false") << '\n'
               << "arm_neutral_ms=" << config.arm_neutral_ms << '\n';
        if (!output) {
            throw std::runtime_error("Failed while writing config file: " + temporary.string());
        }
    }

    std::error_code error;
    std::filesystem::rename(temporary, destination, error);
    if (error) {
        std::filesystem::remove(temporary);
        throw std::runtime_error("Cannot atomically replace config file: " + error.message());
    }
}

}  // namespace roberta::control
