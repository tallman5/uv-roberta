#pragma once

#include "roberta/control/roboteq_telemetry.hpp"

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
    [[nodiscard]] bool send_query(std::string_view query);
    [[nodiscard]] std::optional<std::string> read_line();
    [[nodiscard]] std::optional<std::string> query(
        std::string_view command,
        std::string_view response_prefix);

    int fd_{-1};
    std::string input_buffer_;
};

}  // namespace roberta::control
