#pragma once

#include "roberta/control/hardware.hpp"
#include "roberta/control/rc_mapping.hpp"

#include <chrono>
#include <mutex>
#include <optional>

namespace roberta::control {

class LinuxSbusReceiver final : public RcReceiver {
public:
    LinuxSbusReceiver(
        unsigned pigpio_handle,
        int gpio_pin,
        RcMapping mapping,
        std::chrono::milliseconds timeout);
    ~LinuxSbusReceiver() override;

    LinuxSbusReceiver(const LinuxSbusReceiver&) = delete;
    LinuxSbusReceiver& operator=(const LinuxSbusReceiver&) = delete;

    [[nodiscard]] std::optional<RcFrame> read(MonotonicTime now) noexcept override;
    [[nodiscard]] std::optional<SbusFrame> latest_frame() const;

private:
    static void alert_callback(
        int pi,
        unsigned gpio,
        unsigned level,
        std::uint32_t tick,
        void* userdata);
    void on_edge(int level, std::uint32_t tick) noexcept;
    int gpio_pin_;
    unsigned pigpio_handle_;
    int callback_id_{-1};
    RcMapping mapping_;
    std::chrono::milliseconds timeout_;
    mutable std::mutex mutex_;
    std::optional<SbusFrame> latest_frame_;
    MonotonicTime received_at_{};
    SbusEdgeDecoder decoder_;
};

}  // namespace roberta::control
