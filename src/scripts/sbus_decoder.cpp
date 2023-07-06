#include <iostream>
#include <iomanip>
#include <pigpio.h>
#include <cstring>
#include <bitset>
#include <vector>

#define PIN 17
#define PACKET_BOUNDRY_TIME 5000
#define PACKET_LENGTH 298
#define SBUS_CHANNEL_COUNT 16
#define SBUS_START_BYTE 0x0F
#define SBUS_END_BYTE 0x00
#define UART_FRAME_LENGTH 12

uint32_t lastTick = 0;
int bitIndex = 0;
bool sbusBuffer[PACKET_LENGTH];
static unsigned int channelData[SBUS_CHANNEL_COUNT];

void moveCursorTo(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "H";
}

void populateChannels()
{
    std::bitset<176> channel_bits;
    channel_bits.reset();
    int channel_bits_ptr = 0;

    for (int packet_bits_ptr = UART_FRAME_LENGTH; packet_bits_ptr < UART_FRAME_LENGTH + 22 * UART_FRAME_LENGTH; packet_bits_ptr += UART_FRAME_LENGTH)
    {
        // Extract from UART frame and invert each byte
        for (int i = 0; i < 8; i++)
        {
            channel_bits[channel_bits_ptr + i] = !sbusBuffer[packet_bits_ptr + i + 1];
        }
        channel_bits_ptr += 8;
    }

    std::vector<int> ret_list;
    for (int channel_ptr = 0; channel_ptr < 16 * 11; channel_ptr += 11)
    {
        // Iterate through 11-bit numbers, converting them to ints. Note little endian.
        int value = 0;
        for (int i = 0; i < 11; i++)
        {
            value |= (channel_bits[channel_ptr + i] << i);
        }
        ret_list.push_back(value);
    }

    // Print channel values
    moveCursorTo(0, 0);
    for (int i = 0; i < SBUS_CHANNEL_COUNT; i++)
        std::cout << std::setw(4) << std::setfill('0') << ret_list[i] << "  ";
    std::cout << std::endl;
}

void sbusCallback(int gpio, int level, uint32_t tick)
{
    uint32_t timeElapsed = tick - lastTick;

    if (timeElapsed < 0)
        timeElapsed = 4294967295 - lastTick + tick;

    if (timeElapsed >= PACKET_BOUNDRY_TIME)
    {
        bool goodSbusBuffer = true;

        // Check for SBUS_START_BYTE
        char firstByte = 0;
        for (int i = 1; i < 9; i++)
            firstByte = (firstByte << 1) | sbusBuffer[i];
        if (firstByte != SBUS_START_BYTE)
            goodSbusBuffer = false;

        // Could use some more validation here

        if (goodSbusBuffer)
        {
            populateChannels();
        }

        memset(sbusBuffer, 0, PACKET_LENGTH);
        bitIndex = 0;
        lastTick = tick;
    }
    else
    {
        int elapsedBits = static_cast<int>((timeElapsed + 5) / 10);
        bool newVal = static_cast<bool>(-level + 1);
        for (int i = bitIndex; i < bitIndex + elapsedBits; i++)
            sbusBuffer[i] = newVal;

        bitIndex = bitIndex + elapsedBits;
    }

    lastTick = tick;
}

int main()
{
#ifdef _WIN32
    system("cls"); // For Windows
#else
    system("clear"); // For Unix-like systems
#endif

    if (gpioInitialise() < 0)
    {
        std::cerr << "Failed to initialize pigpio" << std::endl;
        return 1;
    }

    // Set pin PIN as an input
    if (gpioSetMode(PIN, PI_INPUT) < 0)
    {
        std::cerr << "Failed to set pin mode" << std::endl;
        return 1;
    }

    // Set the SBUScallback function for pin PIN
    if (gpioSetAlertFunc(PIN, sbusCallback) < 0)
    {
        std::cerr << "Failed to set callback" << std::endl;
        return 1;
    }

    std::cout << "SBUS decoder running. Press Enter to exit." << std::endl;
    std::cin.ignore();

    std::cout << std::endl;

    gpioTerminate();

    return 0;
}
