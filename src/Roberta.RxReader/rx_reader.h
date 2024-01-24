#ifndef RXREADER_H
#define RXREADER_H

#include <iostream>
#include <array>
#include <cstdint>
#include <bitset>
#include <functional>
#include <vector>
#include <chrono>
#include <pigpio.h>
#include <cstring>

#define PACKET_BOUNDRY_TIME 5000
#define PACKET_LENGTH 298
#define SBUS_CHANNEL_COUNT 16
#define SBUS_START_BYTE 0x0F
#define SBUS_END_BYTE 0x00
#define UART_FRAME_LENGTH 12

class RxReader
{
public:
    struct RxState
    {
        std::vector<int> channelValues;
        bool inFailsafe;
        bool ch17, ch18;
        std::chrono::system_clock::time_point timestamp;

        bool operator==(const RxState &other) const
        {
            // Compare all members except for the timestamp
            return (channelValues == other.channelValues &&
                    inFailsafe == other.inFailsafe &&
                    ch17 == other.ch17 &&
                    ch18 == other.ch18);
        }

        bool operator!=(const RxState &other) const
        {
            return !(*this == other);
        }
    };

    RxReader(int gpioPin);
    ~RxReader();

    void start();
    void stop();
    void writeBuffer();

    void setReaderCallback(std::function<void(const RxState &)> callback);
    const RxState &getLatestRxState() const { return rxState_; }

private:
    int bitIndex_;
    std::function<void(const RxState &)> readerCallback_;
    RxState rxState_;
    int gpioPin_;
    int lastTick_;
    void populateChannels();
    std::bitset<PACKET_LENGTH> sbusBuffer_;
    void sbusCallback(int gpio, int level, uint32_t tick);
    static void sbusCallbackWrapper(int gpio, int level, uint32_t tick, void *user);
    const std::bitset<UART_FRAME_LENGTH> uartConformance_{"100000000011"};

    static constexpr uint8_t CH17_MASK_ = 0x01;
    static constexpr uint8_t CH18_MASK_ = 0x02;
    static constexpr uint8_t LOST_FRAME_MASK_ = 0x04;
    static constexpr uint8_t FAILSAFE_MASK_ = 0x08;
};

#endif // RXREADER_H
