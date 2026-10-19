#pragma once

#include "roberta/control/controller.hpp"

#include <optional>
#include <string_view>

namespace roberta::control {

class RcReceiver {
public:
    virtual ~RcReceiver() = default;
    [[nodiscard]] virtual std::optional<RcFrame> read(MonotonicTime now) noexcept = 0;
};

class MotorDriver {
public:
    virtual ~MotorDriver() = default;
    [[nodiscard]] virtual bool healthy() const noexcept = 0;
    [[nodiscard]] virtual bool apply(const MotorOutput& output) noexcept = 0;
    virtual void stop() noexcept = 0;
};

enum class ControlFault {
    None,
    MotorUnavailable,
    MotorWriteFailed
};

struct ControlResult {
    MotorOutput output;
    bool motor_enabled{};
    ControlFault fault{ControlFault::None};
};

class ControlLoop final {
public:
    ControlLoop(Controller controller, RcReceiver& receiver, MotorDriver& motor);

    [[nodiscard]] ControlResult tick(
        MonotonicTime now,
        const std::optional<DashboardCommand>& dashboard) noexcept;

private:
    Controller controller_;
    RcReceiver& receiver_;
    MotorDriver& motor_;
};

}  // namespace roberta::control
