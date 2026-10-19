#include "roberta/control/linux_sbus_receiver.hpp"

#include <pigpiod_if2.h>

#include <stdexcept>

namespace roberta::control {

LinuxSbusReceiver::LinuxSbusReceiver(
    unsigned pigpio_handle,
    int gpio_pin,
    RcMapping mapping,
    std::chrono::milliseconds timeout)
    : gpio_pin_(gpio_pin),
      pigpio_handle_(pigpio_handle),
      mapping_(mapping),
      timeout_(timeout) {
    if (!valid_mapping(mapping_)) {
        throw std::invalid_argument("Invalid RC channel mapping.");
    }
    if (timeout_ < std::chrono::milliseconds(30) ||
        timeout_ > std::chrono::milliseconds(500)) {
        throw std::invalid_argument("Invalid RC frame timeout.");
    }
    if (set_mode(pigpio_handle_, static_cast<unsigned>(gpio_pin_), PI_INPUT) < 0 ||
        set_alert_func_ex(
            pigpio_handle_,
            static_cast<unsigned>(gpio_pin_),
            &LinuxSbusReceiver::alert_callback,
            this) < 0) {
        throw std::runtime_error("Unable to configure SBUS input GPIO. Check the BCM pin and GPIO permissions.");
    }
}

LinuxSbusReceiver::~LinuxSbusReceiver() {
    set_alert_func_ex(pigpio_handle_, static_cast<unsigned>(gpio_pin_), nullptr, nullptr);
}

void LinuxSbusReceiver::alert_callback(int, int level, std::uint32_t tick, void* userdata) {
    static_cast<LinuxSbusReceiver*>(userdata)->on_edge(level, tick);
}

void LinuxSbusReceiver::on_edge(int level, std::uint32_t tick) noexcept {
    const auto frame = decoder_.feed(level, tick);
    if (frame) {
        std::lock_guard lock(mutex_);
        received_at_ = std::chrono::steady_clock::now();
        if (frame->frame_lost || frame->failsafe || !valid_sbus_drive_channels(*frame, mapping_)) {
            latest_frame_.reset();
            return;
        }
        latest_frame_ = frame;
    }
}

std::optional<SbusFrame> LinuxSbusReceiver::latest_frame() const {
    std::lock_guard lock(mutex_);
    return latest_frame_;
}

std::optional<RcFrame> LinuxSbusReceiver::read(MonotonicTime now) noexcept {
    std::lock_guard lock(mutex_);
    if (!latest_frame_ || received_at_ > now ||
        now - received_at_ > timeout_) {
        return std::nullopt;
    }

    return RcFrame{
        map_sbus_drive(*latest_frame_, mapping_),
        ControlSource::Rc,
        false,
        received_at_
    };
}

}  // namespace roberta::control
