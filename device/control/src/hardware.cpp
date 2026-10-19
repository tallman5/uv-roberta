#include "roberta/control/hardware.hpp"

#include <utility>

namespace roberta::control {

ControlLoop::ControlLoop(Controller controller, RcReceiver& receiver, MotorDriver& motor)
    : controller_(std::move(controller)), receiver_(receiver), motor_(motor) {}

ControlResult ControlLoop::tick(
    MonotonicTime now,
    const std::optional<DashboardCommand>& dashboard) noexcept {
    if (!motor_.healthy()) {
        motor_.stop();
        return {{}, false, ControlFault::MotorUnavailable};
    }

    const auto rc = receiver_.read(now);
    const auto output = controller_.update(now, rc, dashboard);
    const bool moving = output.left != 0.0 || output.right != 0.0;
    if (!moving) {
        motor_.stop();
        return {output, false, ControlFault::None};
    }

    if (!motor_.apply(output)) {
        motor_.stop();
        return {{}, false, ControlFault::MotorWriteFailed};
    }

    return {output, true, ControlFault::None};
}

}  // namespace roberta::control
