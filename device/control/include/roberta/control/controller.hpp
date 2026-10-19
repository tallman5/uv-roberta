#pragma once

#include <chrono>
#include <optional>

namespace roberta::control {

using MonotonicTime = std::chrono::steady_clock::time_point;

enum class ControlSource {
    Rc,
    Dashboard
};

struct DriveInput {
    double throttle{};
    double steering{};
};

struct RcFrame {
    DriveInput drive;
    ControlSource selected_source{ControlSource::Rc};
    bool slow_mode{};
    MonotonicTime received_at;
    double motor_command_scale{1.0};
};

struct DashboardCommand {
    DriveInput drive;
    MonotonicTime received_at;
};

struct MotorOutput {
    double left{};
    double right{};
};

struct ControllerConfig {
    std::chrono::milliseconds rc_timeout{250};
    std::chrono::milliseconds dashboard_timeout{500};
    double slow_mode_scale{0.25};
};

class Controller final {
public:
    explicit Controller(ControllerConfig config = {});

    [[nodiscard]] MotorOutput update(
        MonotonicTime now,
        const std::optional<RcFrame>& rc,
        const std::optional<DashboardCommand>& dashboard) const;

private:
    ControllerConfig config_;
};

}  // namespace roberta::control
