#pragma once

#include "roberta/control/roboteq_telemetry.hpp"

#include <chrono>
#include <optional>
#include <string>
#include <string_view>

namespace roberta::control {

class LinuxRoboteqTelemetry final {
public:
    LinuxRoboteqTelemetry(std::string device, int baud);
    ~LinuxRoboteqTelemetry();

    LinuxRoboteqTelemetry(const LinuxRoboteqTelemetry&) = delete;
    LinuxRoboteqTelemetry& operator=(const LinuxRoboteqTelemetry&) = delete;

    [[nodiscard]] RoboteqTelemetrySnapshot refresh();

private:
    [[nodiscard]] bool send_command(std::string_view command);
    [[nodiscard]] std::optional<std::string> read_line(
        std::chrono::steady_clock::time_point deadline);

    int fd_{-1};
    std::string input_buffer_;
    RoboteqTelemetrySnapshot latest_snapshot_;
};

}  // namespace roberta::control
