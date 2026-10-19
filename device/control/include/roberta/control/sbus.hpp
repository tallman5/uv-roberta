#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace roberta::control {

struct SbusFrame {
    std::array<std::uint16_t, 16> channels{};
    bool frame_lost{};
    bool failsafe{};
};

class SbusDecoder final {
public:
    [[nodiscard]] std::optional<SbusFrame> feed(std::uint8_t byte) noexcept;

private:
    std::array<std::uint8_t, 25> buffer_{};
    std::size_t count_{};
};

class SbusEdgeDecoder final {
public:
    [[nodiscard]] std::optional<SbusFrame> feed(int level, std::uint32_t tick) noexcept;

private:
    void append(bool logical_level, unsigned count) noexcept;

    std::array<bool, 300> bits_{};
    std::size_t count_{};
    std::uint32_t last_tick_{};
    int last_level_{};
    bool receiving_{};
};

[[nodiscard]] std::optional<SbusFrame> decode_sbus_frame(
    std::span<const std::uint8_t, 25> bytes) noexcept;
[[nodiscard]] std::optional<SbusFrame> decode_sbus_gpio_bits(
    std::span<const bool, 300> bits) noexcept;

}  // namespace roberta::control
