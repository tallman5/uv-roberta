#pragma once

#include "roberta/control/hardware.hpp"
#include "roberta/control/rc_config.hpp"

namespace roberta::control {

class LinuxZsX11Driver final : public MotorDriver {
public:
    LinuxZsX11Driver(unsigned pigpio_handle, const RcDriveConfig& config);
    ~LinuxZsX11Driver() override;

    LinuxZsX11Driver(const LinuxZsX11Driver&) = delete;
    LinuxZsX11Driver& operator=(const LinuxZsX11Driver&) = delete;

    [[nodiscard]] bool healthy() const noexcept override;
    [[nodiscard]] bool apply(const MotorOutput& output) noexcept override;
    void stop() noexcept override;

private:
    [[nodiscard]] bool write_speed(int gpio, int duty) noexcept;
    [[nodiscard]] bool write_level(int gpio, int level) noexcept;
    [[nodiscard]] bool release_outputs() noexcept;
    void fail_safe_stop() noexcept;

    unsigned pigpio_handle_{};
    int left_speed_gpio_{};
    int left_direction_gpio_{};
    int left_stop_gpio_{};
    int right_speed_gpio_{};
    int right_direction_gpio_{};
    int right_stop_gpio_{};
    int max_pwm_{};
    bool left_direction_inverted_{};
    bool right_direction_inverted_{};
    bool left_forward_{};
    bool right_forward_{};
    bool left_direction_known_{};
    bool right_direction_known_{};
    bool enabled_{};
    bool faulted_{};
};

}  // namespace roberta::control
