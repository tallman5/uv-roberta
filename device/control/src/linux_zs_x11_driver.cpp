#include "roberta/control/linux_zs_x11_driver.hpp"

#include <pigpiod_if2.h>

#include <cmath>
#include <iostream>
#include <stdexcept>

namespace roberta::control {
namespace {

bool valid_output(const MotorOutput& output) noexcept {
    return std::isfinite(output.left) && std::isfinite(output.right) &&
           std::abs(output.left) <= 1.0 && std::abs(output.right) <= 1.0;
}

int pwm_duty(double command, int maximum) noexcept {
    return static_cast<int>(std::lround(std::abs(command) * maximum));
}

}  // namespace

LinuxZsX11Driver::LinuxZsX11Driver(unsigned pigpio_handle, const RcDriveConfig& config)
    : pigpio_handle_(pigpio_handle),
      left_speed_gpio_(config.zs_left_speed_gpio),
      left_direction_gpio_(config.zs_left_direction_gpio),
      left_stop_gpio_(config.zs_left_stop_gpio),
      right_speed_gpio_(config.zs_right_speed_gpio),
      right_direction_gpio_(config.zs_right_direction_gpio),
      right_stop_gpio_(config.zs_right_stop_gpio),
      max_pwm_(config.zs_max_pwm),
      left_direction_inverted_(config.zs_left_direction_inverted),
      right_direction_inverted_(config.zs_right_direction_inverted) {
    if (max_pwm_ < 1 || max_pwm_ > 255 ||
        config.zs_pwm_frequency_hz < 50 || config.zs_pwm_frequency_hz > 20000) {
        throw std::invalid_argument("Invalid ZS-X11-H drive configuration.");
    }

    const int stop_pins[] = {left_stop_gpio_, right_stop_gpio_};
    const int output_pins[] = {
        left_speed_gpio_, left_direction_gpio_,
        right_speed_gpio_, right_direction_gpio_
    };
    for (const int gpio : stop_pins) {
        if (set_mode(pigpio_handle_, static_cast<unsigned>(gpio), PI_OUTPUT) < 0 ||
            !write_level(gpio, 0)) {
            fail_safe_stop();
            const bool released = release_outputs();
            throw std::runtime_error(
                released
                    ? "Unable to assert the active-low ZS controller stop inputs."
                    : "Unable to assert the active-low ZS controller stop inputs or release its GPIOs.");
        }
    }
    for (const int gpio : output_pins) {
        if (set_mode(pigpio_handle_, static_cast<unsigned>(gpio), PI_OUTPUT) < 0) {
            fail_safe_stop();
            const bool released = release_outputs();
            throw std::runtime_error(
                released
                    ? "Unable to configure ZS controller output GPIO."
                    : "Unable to configure ZS controller output GPIO or release its GPIOs.");
        }
    }
    if (set_PWM_frequency(
            pigpio_handle_,
            static_cast<unsigned>(left_speed_gpio_),
            config.zs_pwm_frequency_hz) < 0 ||
        set_PWM_frequency(
            pigpio_handle_,
            static_cast<unsigned>(right_speed_gpio_),
            config.zs_pwm_frequency_hz) < 0 ||
        !write_speed(left_speed_gpio_, 0) ||
        !write_speed(right_speed_gpio_, 0) ||
        !write_level(left_direction_gpio_, 0) ||
        !write_level(right_direction_gpio_, 0)) {
        fail_safe_stop();
        const bool released = release_outputs();
        throw std::runtime_error(
            released
                ? "Unable to initialize ZS controller outputs in the stopped state."
                : "Unable to initialize ZS controller outputs or release its GPIOs.");
    }
}

LinuxZsX11Driver::~LinuxZsX11Driver() {
    stop();
    if (!release_outputs()) {
        std::cerr << "roberta_rc_drive: Unable to release ZS GPIOs to inputs; "
                     "the active-low stop outputs may remain asserted."
                  << std::endl;
    }
}

bool LinuxZsX11Driver::healthy() const noexcept {
    return !faulted_;
}

bool LinuxZsX11Driver::write_speed(int gpio, int duty) noexcept {
    return set_PWM_dutycycle(
        pigpio_handle_,
        static_cast<unsigned>(gpio),
        static_cast<unsigned>(duty)) >= 0;
}

bool LinuxZsX11Driver::write_level(int gpio, int level) noexcept {
    return gpio_write(pigpio_handle_, static_cast<unsigned>(gpio), static_cast<unsigned>(level)) >= 0;
}

bool LinuxZsX11Driver::release_outputs() noexcept {
    bool ok = true;
    const int pins[] = {
        left_speed_gpio_,
        left_direction_gpio_,
        right_speed_gpio_,
        right_direction_gpio_,
        left_stop_gpio_,
        right_stop_gpio_
    };
    for (const int gpio : pins) {
        ok = set_mode(pigpio_handle_, static_cast<unsigned>(gpio), PI_INPUT) >= 0 && ok;
    }
    return ok;
}

void LinuxZsX11Driver::fail_safe_stop() noexcept {
    bool ok = true;
    ok = write_speed(left_speed_gpio_, 0) && ok;
    ok = write_speed(right_speed_gpio_, 0) && ok;
    ok = write_level(left_stop_gpio_, 0) && ok;
    ok = write_level(right_stop_gpio_, 0) && ok;
    enabled_ = false;
    faulted_ = faulted_ || !ok;
}

bool LinuxZsX11Driver::apply(const MotorOutput& output) noexcept {
    if (faulted_ || !valid_output(output)) {
        faulted_ = true;
        stop();
        return false;
    }

    const bool left_forward = output.left >= 0.0;
    const bool right_forward = output.right >= 0.0;
    const bool left_direction = left_forward != left_direction_inverted_;
    const bool right_direction = right_forward != right_direction_inverted_;

    if (left_direction_known_ && left_forward != left_forward_ &&
        !write_speed(left_speed_gpio_, 0)) {
        faulted_ = true;
        stop();
        return false;
    }
    if (right_direction_known_ && right_forward != right_forward_ &&
        !write_speed(right_speed_gpio_, 0)) {
        faulted_ = true;
        stop();
        return false;
    }
    if (!write_level(left_direction_gpio_, left_direction ? 1 : 0) ||
        !write_level(right_direction_gpio_, right_direction ? 1 : 0) ||
        !write_speed(left_speed_gpio_, pwm_duty(output.left, max_pwm_)) ||
        !write_speed(right_speed_gpio_, pwm_duty(output.right, max_pwm_)) ||
        (!enabled_ &&
         (!write_level(left_stop_gpio_, 1) || !write_level(right_stop_gpio_, 1)))) {
        faulted_ = true;
        stop();
        return false;
    }

    left_forward_ = left_forward;
    right_forward_ = right_forward;
    left_direction_known_ = true;
    right_direction_known_ = true;
    enabled_ = true;
    return true;
}

void LinuxZsX11Driver::stop() noexcept {
    bool ok = true;
    ok = write_speed(left_speed_gpio_, 0) && ok;
    ok = write_speed(right_speed_gpio_, 0) && ok;
    ok = write_level(left_stop_gpio_, 0) && ok;
    ok = write_level(right_stop_gpio_, 0) && ok;
    enabled_ = false;
    faulted_ = faulted_ || !ok;
}

}  // namespace roberta::control
