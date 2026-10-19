#pragma once

#include "roberta/control/controller.hpp"
#include "roberta/control/sbus.hpp"

#include <cstdint>

namespace roberta::control {

struct AxisCalibration {
    std::uint16_t minimum{};
    std::uint16_t center{};
    std::uint16_t maximum{};
    bool inverted{};
};

struct RcMapping {
    unsigned throttle_channel{};
    unsigned steering_channel{};
    AxisCalibration throttle;
    AxisCalibration steering;
    double deadband{0.04};
};

[[nodiscard]] bool valid_calibration(const AxisCalibration& calibration) noexcept;
[[nodiscard]] double normalize_axis(
    std::uint16_t value,
    const AxisCalibration& calibration,
    double deadband) noexcept;
[[nodiscard]] bool valid_mapping(const RcMapping& mapping) noexcept;
[[nodiscard]] bool valid_sbus_drive_channels(const SbusFrame& frame, const RcMapping& mapping) noexcept;
[[nodiscard]] DriveInput map_sbus_drive(const SbusFrame& frame, const RcMapping& mapping) noexcept;

}  // namespace roberta::control
