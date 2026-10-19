#pragma once

#include "roberta/control/hardware.hpp"

#include <string>

namespace roberta::control {

class LinuxRoboteqDriver final : public MotorDriver {
public:
    LinuxRoboteqDriver(
        std::string device,
        int baud,
        int max_command,
        int left_channel,
        int right_channel,
        bool invert_left,
        bool invert_right);
    ~LinuxRoboteqDriver() override;

    LinuxRoboteqDriver(const LinuxRoboteqDriver&) = delete;
    LinuxRoboteqDriver& operator=(const LinuxRoboteqDriver&) = delete;

    [[nodiscard]] bool healthy() const noexcept override;
    [[nodiscard]] bool apply(const MotorOutput& output) noexcept override;
    void stop() noexcept override;

private:
    [[nodiscard]] bool send_text(const std::string& text) noexcept;
    [[nodiscard]] bool send_command(int channel, int command) noexcept;
    [[nodiscard]] bool drain_input() noexcept;

    int fd_{-1};
    int max_command_{};
    int left_channel_{};
    int right_channel_{};
    bool invert_left_{};
    bool invert_right_{};
    bool faulted_{};
    std::string input_line_;
};

}  // namespace roberta::control
