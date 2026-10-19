#include "roberta/control/rc_mapping.hpp"

#include <algorithm>
#include <cmath>

namespace roberta::control {

bool valid_calibration(const AxisCalibration& calibration) noexcept {
    return calibration.minimum < calibration.center &&
           calibration.center < calibration.maximum &&
           calibration.minimum >= 172 &&
           calibration.maximum <= 1811 &&
           calibration.center - calibration.minimum >= 200 &&
           calibration.maximum - calibration.center >= 200;
}

double normalize_axis(
    std::uint16_t value,
    const AxisCalibration& calibration,
    double deadband) noexcept {
    const auto bounded = std::clamp(value, calibration.minimum, calibration.maximum);
    double normalized = bounded >= calibration.center
        ? static_cast<double>(bounded - calibration.center) /
              static_cast<double>(calibration.maximum - calibration.center)
        : -static_cast<double>(calibration.center - bounded) /
              static_cast<double>(calibration.center - calibration.minimum);
    if (calibration.inverted) {
        normalized = -normalized;
    }

    if (std::abs(normalized) <= deadband) {
        return 0.0;
    }
    const double adjusted = (std::abs(normalized) - deadband) / (1.0 - deadband);
    return std::copysign(adjusted, normalized);
}

bool valid_mapping(const RcMapping& mapping) noexcept {
    return mapping.throttle_channel < 16 &&
           mapping.steering_channel < 16 &&
           mapping.throttle_channel != mapping.steering_channel &&
           valid_calibration(mapping.throttle) &&
           valid_calibration(mapping.steering) &&
           std::isfinite(mapping.deadband) &&
           mapping.deadband >= 0.0 &&
           mapping.deadband < 0.25;
}

bool valid_sbus_drive_channels(const SbusFrame& frame, const RcMapping& mapping) noexcept {
    return valid_mapping(mapping) &&
           frame.channels[mapping.throttle_channel] >= 172 &&
           frame.channels[mapping.throttle_channel] <= 1811 &&
           frame.channels[mapping.steering_channel] >= 172 &&
           frame.channels[mapping.steering_channel] <= 1811 &&
           frame.channels[9] >= 172 &&
           frame.channels[9] <= 1811;
}

DriveInput map_sbus_drive(const SbusFrame& frame, const RcMapping& mapping) noexcept {
    if (frame.frame_lost || frame.failsafe || !valid_sbus_drive_channels(frame, mapping)) {
        return {};
    }

    return {
        normalize_axis(frame.channels[mapping.throttle_channel], mapping.throttle, mapping.deadband),
        normalize_axis(frame.channels[mapping.steering_channel], mapping.steering, mapping.deadband)
    };
}

double motor_command_scale(std::uint16_t channel_value) noexcept {
    constexpr std::uint16_t kSbusMinimum = 172;
    constexpr std::uint16_t kLowMediumBoundary =
        kSbusMinimum + (1811 - kSbusMinimum) / 3;
    constexpr std::uint16_t kMediumHighBoundary =
        kSbusMinimum + 2 * (1811 - kSbusMinimum) / 3;

    if (channel_value < kSbusMinimum || channel_value > 1811) {
        return 0.0;
    }
    if (channel_value < kLowMediumBoundary) {
        return 1.0 / 3.0;
    }
    if (channel_value < kMediumHighBoundary) {
        return 2.0 / 3.0;
    }
    return 1.0;
}

}  // namespace roberta::control
