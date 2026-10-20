#include "roberta/control/roboteq_telemetry.hpp"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

namespace roberta::control {
namespace {

std::optional<std::vector<double>> parse_values(
    std::string_view response,
    std::string_view prefix,
    double scale) {
    if (!std::isfinite(scale) || response.size() <= prefix.size() ||
        response.substr(0, prefix.size()) != prefix) {
        return std::nullopt;
    }

    std::string values(response.substr(prefix.size()));
    std::istringstream input(values);
    input.imbue(std::locale::classic());
    std::vector<double> result;
    std::string value;
    while (std::getline(input, value, ':')) {
        std::istringstream number(value);
        number.imbue(std::locale::classic());
        double parsed{};
        if (!(number >> parsed) || !std::isfinite(parsed)) {
            return std::nullopt;
        }
        number >> std::ws;
        if (!number.eof()) {
            return std::nullopt;
        }
        result.push_back(parsed * scale);
    }
    return result;
}

}  // namespace

std::optional<std::array<double, 2>> parse_roboteq_pair(
    std::string_view response,
    std::string_view prefix,
    double scale) {
    const auto values = parse_roboteq_values(response, prefix, scale);
    if (!values || values->size() != 2) {
        return std::nullopt;
    }
    return std::array<double, 2>{(*values)[0], (*values)[1]};
}

std::optional<std::vector<double>> parse_roboteq_values(
    std::string_view response,
    std::string_view prefix,
    double scale) {
    return parse_values(response, prefix, scale);
}

std::optional<std::array<double, 3>> parse_roboteq_triple(
    std::string_view response,
    std::string_view prefix,
    double scale) {
    const auto values = parse_roboteq_values(response, prefix, scale);
    if (!values || values->size() != 3) {
        return std::nullopt;
    }
    return std::array<double, 3>{(*values)[0], (*values)[1], (*values)[2]};
}

std::optional<int> estimate_battery_percent(
    const std::optional<double>& voltage,
    double nominal_volts) {
    if (!std::isfinite(nominal_volts) || nominal_volts <= 0) {
        return std::nullopt;
    }

    if (!voltage || !std::isfinite(*voltage) || *voltage < 0) {
        return std::nullopt;
    }

    constexpr double kVoltageScale = 1.125;
    const double minimum = nominal_volts / kVoltageScale;
    const double maximum = nominal_volts * kVoltageScale;
    const double ratio = (*voltage - minimum) / (maximum - minimum);
    return static_cast<int>(std::lround(std::clamp(ratio, 0.0, 1.0) * 100.0));
}

}  // namespace roberta::control
