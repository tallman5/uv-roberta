#include "roberta/control/sbus.hpp"

#include <algorithm>

namespace roberta::control {
namespace {

constexpr std::uint8_t kStartByte = 0x0F;
constexpr std::uint32_t kBitPeriodMicros = 10;
constexpr std::uint32_t kFrameGapMicros = 3000;

bool valid_end_byte(std::uint8_t byte) noexcept {
    return byte == 0x00 || byte == 0x04 || byte == 0x14 ||
           byte == 0x24 || byte == 0x34;
}

}  // namespace

void SbusEdgeDecoder::append(bool logical_level, unsigned count) noexcept {
    const auto available = static_cast<unsigned>(bits_.size() - count_);
    const auto bounded_count = std::min(count, available);
    std::fill_n(bits_.begin() + static_cast<std::ptrdiff_t>(count_), bounded_count, logical_level);
    count_ += bounded_count;
}

std::optional<SbusFrame> SbusEdgeDecoder::feed(int level, std::uint32_t tick) noexcept {
    if (level != 0 && level != 1) {
        return std::nullopt;
    }

    std::optional<SbusFrame> decoded;
    if (receiving_) {
        const std::uint32_t elapsed = tick - last_tick_;
        if (elapsed >= kFrameGapMicros) {
            append(last_level_ == 0, (elapsed + kBitPeriodMicros / 2) / kBitPeriodMicros);
            if (count_ == bits_.size()) {
                decoded = decode_sbus_gpio_bits(std::span<const bool, 300>(bits_));
            }
            receiving_ = false;
            count_ = 0;
        } else {
            const auto bit_count = (elapsed + kBitPeriodMicros / 2) / kBitPeriodMicros;
            const auto represented_micros = bit_count * kBitPeriodMicros;
            const auto timing_error = elapsed > represented_micros
                ? elapsed - represented_micros
                : represented_micros - elapsed;
            if (bit_count == 0 || timing_error > 4 ||
                count_ + bit_count > bits_.size()) {
                receiving_ = false;
                count_ = 0;
            } else {
                append(last_level_ == 0, bit_count);
            }
        }
    }

    last_tick_ = tick;
    last_level_ = level;
    if (!receiving_ && level == 1) {
        receiving_ = true;
        count_ = 0;
    }
    return decoded;
}

std::optional<SbusFrame> decode_sbus_frame(
    std::span<const std::uint8_t, 25> bytes) noexcept {
    if (bytes[0] != kStartByte || !valid_end_byte(bytes[24])) {
        return std::nullopt;
    }

    SbusFrame frame;
    for (std::size_t channel = 0; channel < frame.channels.size(); ++channel) {
        const std::size_t bit_offset = channel * 11;
        std::uint16_t value = 0;
        for (std::size_t bit = 0; bit < 11; ++bit) {
            const std::size_t source_bit = bit_offset + bit;
            const auto source_byte = bytes[1 + source_bit / 8];
            if ((source_byte & (1U << (source_bit % 8))) != 0) {
                value |= static_cast<std::uint16_t>(1U << bit);
            }
        }
        frame.channels[channel] = value;
    }

    const auto flags = bytes[23];
    frame.frame_lost = (flags & 0x04U) != 0;
    frame.failsafe = (flags & 0x08U) != 0;
    return frame;
}

std::optional<SbusFrame> decode_sbus_gpio_bits(
    std::span<const bool, 300> bits) noexcept {
    constexpr std::size_t bits_per_character = 12;
    std::array<std::uint8_t, 25> bytes{};
    for (std::size_t byte_index = 0; byte_index < bytes.size(); ++byte_index) {
        const std::size_t offset = byte_index * bits_per_character;
        if (bits[offset] || !bits[offset + 10] || !bits[offset + 11]) {
            return std::nullopt;
        }

        unsigned ones = 0;
        for (unsigned data_bit = 0; data_bit < 8; ++data_bit) {
            if (bits[offset + 1 + data_bit]) {
                bytes[byte_index] |= static_cast<std::uint8_t>(1U << data_bit);
                ++ones;
            }
        }
        if (bits[offset + 9] != (ones % 2U != 0)) {
            return std::nullopt;
        }
    }
    return decode_sbus_frame(std::span<const std::uint8_t, 25>(bytes));
}

std::optional<SbusFrame> SbusDecoder::feed(std::uint8_t byte) noexcept {
    if (count_ == 0 && byte != kStartByte) {
        return std::nullopt;
    }

    buffer_[count_++] = byte;
    if (count_ != buffer_.size()) {
        return std::nullopt;
    }

    count_ = 0;
    return decode_sbus_frame(std::span<const std::uint8_t, 25>(buffer_));
}

}  // namespace roberta::control
