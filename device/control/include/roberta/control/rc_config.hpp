#pragma once

#include "roberta/control/rc_mapping.hpp"

#include <string>

namespace roberta::control {

struct RcDriveConfig {
    int receiver_gpio{17};
    RcMapping mapping{
        1,
        0,
        {172, 992, 1811, false},
        {172, 992, 1811, false},
        0.04
    };
    int receiver_timeout_ms{100};
    std::string roboteq_device{
        "/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00"};
    int roboteq_baud{9600};
    double battery_nominal_volts{24.0};
    int max_motor_command{200};
    int left_motor_channel{2};
    int right_motor_channel{1};
    bool left_motor_inverted{};
    bool right_motor_inverted{};
    int arm_neutral_ms{2000};
};

[[nodiscard]] RcDriveConfig load_rc_drive_config(const std::string& path);
void validate_rc_drive_config(const RcDriveConfig& config, bool require_motor_device);
void save_rc_drive_config(const std::string& path, const RcDriveConfig& config);

}  // namespace roberta::control
