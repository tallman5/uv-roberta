#include "roberta/control/rc_config.hpp"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <stdexcept>

namespace roberta::control {
void validate_rc_drive_config(
    const RcDriveConfig& config,
    bool require_motor_device,
    bool require_zs_controller) {
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
    if (!std::isfinite(config.battery_nominal_volts) ||
        config.battery_nominal_volts < 1.0 || config.battery_nominal_volts > 1000.0) {
        throw std::runtime_error("battery_nominal_volts must be between 1 and 1000.");
    }
    if (config.max_motor_command < 1 || config.max_motor_command > 1000) {
        throw std::runtime_error("max_motor_command must be between 1 and 1000.");
    }
    const int zs_pins[] = {
        config.zs_left_speed_gpio,
        config.zs_left_direction_gpio,
        config.zs_left_stop_gpio,
        config.zs_right_speed_gpio,
        config.zs_right_direction_gpio,
        config.zs_right_stop_gpio
    };
    for (const int pin : zs_pins) {
        if (require_zs_controller && (pin < 0 || pin > 27)) {
            throw std::runtime_error("ZS GPIO pins must be BCM GPIO numbers from 0 through 27.");
        }
    }
    if (require_zs_controller) {
        for (std::size_t first = 0; first < std::size(zs_pins); ++first) {
            for (std::size_t second = first + 1; second < std::size(zs_pins); ++second) {
                if (zs_pins[first] == zs_pins[second]) {
                    throw std::runtime_error("ZS output GPIO pins must all be distinct.");
                }
            }
        }
        if (std::find(std::begin(zs_pins), std::end(zs_pins), config.receiver_gpio) !=
            std::end(zs_pins)) {
            throw std::runtime_error("The SBUS input GPIO must not be used by a ZS output.");
        }
        if (config.zs_max_pwm < 1 || config.zs_max_pwm > 255) {
            throw std::runtime_error("zs_max_pwm must be between 1 and 255.");
        }
        if (config.zs_pwm_frequency_hz < 50 || config.zs_pwm_frequency_hz > 20000) {
            throw std::runtime_error("zs_pwm_frequency_hz must be between 50 and 20000.");
        }
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

}  // namespace roberta::control
