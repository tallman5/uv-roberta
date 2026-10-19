#include "roberta/control/controller.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace roberta::control {
namespace {

bool valid_drive(const DriveInput& drive) {
    return std::isfinite(drive.throttle) &&
           std::isfinite(drive.steering) &&
           drive.throttle >= -1.0 && drive.throttle <= 1.0 &&
           drive.steering >= -1.0 && drive.steering <= 1.0;
}

bool fresh(MonotonicTime now, MonotonicTime received_at, std::chrono::milliseconds timeout) {
    return received_at <= now && now - received_at <= timeout;
}

MotorOutput mix(const DriveInput& drive, double scale) {
    return {
        std::clamp((drive.throttle + drive.steering) * scale, -1.0, 1.0),
        std::clamp((drive.throttle - drive.steering) * scale, -1.0, 1.0)
    };
}

}  // namespace

Controller::Controller(ControllerConfig config) : config_(config) {
    if (config_.rc_timeout <= std::chrono::milliseconds::zero() ||
        config_.dashboard_timeout <= std::chrono::milliseconds::zero() ||
        !std::isfinite(config_.slow_mode_scale) ||
        config_.slow_mode_scale < 0.0 || config_.slow_mode_scale > 1.0) {
        throw std::invalid_argument("controller timeouts and slow-mode scale must be within safe bounds");
    }
}

MotorOutput Controller::update(
    MonotonicTime now,
    const std::optional<RcFrame>& rc,
    const std::optional<DashboardCommand>& dashboard) const {
    if (!rc || !fresh(now, rc->received_at, config_.rc_timeout) || !valid_drive(rc->drive)) {
        return {};
    }
    if (!std::isfinite(rc->motor_command_scale) ||
        rc->motor_command_scale < 0.0 || rc->motor_command_scale > 1.0) {
        return {};
    }

    DriveInput requested{};
    switch (rc->selected_source) {
    case ControlSource::Rc:
        requested = rc->drive;
        break;
    case ControlSource::Dashboard:
        if (!dashboard ||
            !fresh(now, dashboard->received_at, config_.dashboard_timeout) ||
            !valid_drive(dashboard->drive)) {
            return {};
        }
        requested = dashboard->drive;
        break;
    default:
        return {};
    }

    auto output = mix(requested, rc->slow_mode ? config_.slow_mode_scale : 1.0);
    output.left *= rc->motor_command_scale;
    output.right *= rc->motor_command_scale;
    return output;
}

}  // namespace roberta::control
