#pragma once

#include <array>
#include <chrono>
#include <optional>
#include <string_view>
#include <vector>

namespace roberta::control {

struct RoboteqTelemetrySnapshot {
    std::optional<double> battery_volts;
    std::array<std::optional<double>, 2> speed_rpm;
    std::array<std::optional<double>, 2> motor_amps;
    std::array<std::optional<double>, 2> battery_amps;
    std::array<std::optional<double>, 2> power_percent;
    std::array<std::optional<double>, 2> motor_temperature;
    std::optional<double> controller_temperature;
    std::optional<std::chrono::steady_clock::time_point> last_response_at;
    int responses{};
};

[[nodiscard]] std::optional<std::array<double, 2>> parse_roboteq_pair(
    std::string_view response,
    std::string_view prefix,
    double scale = 1.0);
[[nodiscard]] std::optional<std::vector<double>> parse_roboteq_values(
    std::string_view response,
    std::string_view prefix,
    double scale = 1.0);
[[nodiscard]] std::optional<std::array<double, 3>> parse_roboteq_triple(
    std::string_view response,
    std::string_view prefix,
    double scale = 1.0);
[[nodiscard]] std::optional<int> estimate_battery_percent(
    const std::optional<double>& voltage,
    double nominal_volts);

}  // namespace roberta::control
