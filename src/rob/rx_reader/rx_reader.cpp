#include "rx_reader.h"

RxReader::RxReader(int gpioPin)
    : gpioPin_(gpioPin)
{
}

RxReader::~RxReader()
{
    stop();
}

void RxReader::populateChannels()
{
    std::bitset<UART_FRAME_LENGTH> uartFrame{"000000000001"};

    for (int packet_bits_ptr = 0; packet_bits_ptr < PACKET_LENGTH - UART_FRAME_LENGTH; packet_bits_ptr += UART_FRAME_LENGTH)
    {
        for (int i = 0; i < UART_FRAME_LENGTH; i++)
            uartFrame.set(UART_FRAME_LENGTH - i - 1, sbusBuffer_[packet_bits_ptr + i]);

        // Check UART start and stop bits
        if ((uartConformance_ & uartFrame).to_ulong() != 2048)
        {
            return;
        }

        // Check parity bit in UART
    }

    std::bitset<176> channel_bits;
    channel_bits.reset();
    int channel_bits_ptr = 0;

    for (int packet_bits_ptr = UART_FRAME_LENGTH; packet_bits_ptr < UART_FRAME_LENGTH + 22 * UART_FRAME_LENGTH; packet_bits_ptr += UART_FRAME_LENGTH)
    {
        // Extract from UART frame and invert each byte
        for (int i = 0; i < 8; i++)
        {
            channel_bits[channel_bits_ptr + i] = !sbusBuffer_[packet_bits_ptr + i + 1];
        }
        channel_bits_ptr += 8;
    }

    std::vector<int> channelValues;
    for (int channel_ptr = 0; channel_ptr < 16 * 11; channel_ptr += 11)
    {
        // Iterate through 11-bit numbers, converting them to ints. Note little endian.
        int value = 0;
        for (int i = 0; i < 11; i++)
            value |= (channel_bits[channel_ptr + i] << i);
        channelValues.push_back(value);
    }

    RxState newValues;
    newValues.channelValues = channelValues;
    newValues.inFailsafe = !(sbusBuffer_[279] && sbusBuffer_[280]);

    int scaleTo = 255;
    int scaleFrom = scaleTo * -1;
    int scaledL = scaleValue(channelValues[0], 172, 1811, scaleTo * -1, scaleTo);
    int scaledR = scaleValue(channelValues[1], 172, 1811, scaleTo * -1, scaleTo);

    newValues.L = std::max(scaleFrom, std::min(scaledL + scaledR, scaleTo));
    newValues.R = std::max(scaleFrom, std::min(scaledR - scaledL, scaleTo));

    if (newValues != rxState_)
    {
        newValues.timestamp = std::chrono::system_clock::now();
        rxState_ = newValues;
        if (readerCallback_)
            readerCallback_(rxState_);
    }
}

int RxReader::scaleValue(int value, int minValue, int maxValue, int scaledMinValue, int scaledMaxValue)
{
    int scaledValue = scaledMinValue + static_cast<int>((static_cast<double>(value - minValue) / (maxValue - minValue)) * (scaledMaxValue - scaledMinValue));
    return std::max(scaledMinValue, std::min(scaledValue, scaledMaxValue));
}

void RxReader::sbusCallback(int gpio, int level, uint32_t tick)
{
    uint32_t timeElapsed = tick - lastTick_;
    if (timeElapsed < 0)
        timeElapsed = 4294967295 - lastTick_ + tick;

    if (timeElapsed >= PACKET_BOUNDRY_TIME)
    {
        populateChannels();
        sbusBuffer_.reset();
        bitIndex_ = 0;
    }
    else
    {
        int elapsedBits = (timeElapsed + 5) / 10;
        bool newVal = static_cast<bool>(-level + 1);
        for (int i = bitIndex_; i < bitIndex_ + elapsedBits; i++)
        {
            if (i < PACKET_LENGTH)
                sbusBuffer_.set(i, newVal);
        }
        bitIndex_ = bitIndex_ + elapsedBits;
    }

    lastTick_ = tick;
}

void RxReader::sbusCallbackWrapper(int gpio, int level, uint32_t tick, void *user)
{
    RxReader *mySelf = (RxReader *)user;
    mySelf->sbusCallback(gpio, level, tick);
}

void RxReader::setReaderCallback(std::function<void(const RxState &)> callback)
{
    readerCallback_ = std::move(callback);
}

void RxReader::start()
{
    bitIndex_ = 0;
    lastTick_ = 0;

    if (gpioSetMode(gpioPin_, PI_INPUT) < 0)
    {
        std::cerr << "Failed to set pin mode" << std::endl;
        return;
    }

    if (gpioSetAlertFuncEx(gpioPin_, sbusCallbackWrapper, (void *)this) < 0)
    {
        std::cerr << "Failed to set callback" << std::endl;
        return;
    }
}

void RxReader::stop()
{
    // Clearing the callback
    if (gpioSetAlertFuncEx(gpioPin_, NULL, NULL) < 0)
    {
        std::cerr << "Failed to clear callback" << std::endl;
    }
    std::cout << "RX reader stopped" << std::endl;
}

void RxReader::writeBuffer()
{
    for (int i = 0; i < PACKET_LENGTH; i++)
        std::cout << sbusBuffer_[i];
    std::cout << std::endl;
}
