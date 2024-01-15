#include <iostream>
#include <unistd.h>
#include "controller/controller.h"
#include "rx_reader/rx_reader.h"
#include <iomanip>
#include <signal.h>
#include <thread>

#define PIN 17
#define SBUS_CHANNEL_COUNT 16
std::chrono::system_clock::time_point lastTimestamp;
RxReader reader(PIN);

void cleanup(int signal)
{
    std::cout << std::endl
              << "Cleaning up and exiting..." << std::endl;
    reader.stop();
    exit(0);
}

void moveCursorTo(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "H";
}

void readerCallback(const RxReader::RxState &rxState)
{
    if (rxState.timestamp > lastTimestamp)
    {
        lastTimestamp = rxState.timestamp;

        moveCursorTo(0, 0);
        for (int i = 0; i < SBUS_CHANNEL_COUNT; i++)
            std::cout << std::setw(4) << std::setfill('0') << rxState.channelValues[i] << "  ";
        std::cout << std::endl;
    }
}

int main()
{
    signal(SIGINT, cleanup);

    if (gpioInitialise() < 0)
    {
        std::cerr << "Failed to initialize pigpio" << std::endl;
        return 1;
    }

#ifdef _WIN32
    system("cls"); // For Windows
#else
    system("clear"); // For Unix-like systems
#endif

    try
    {
        reader.setReaderCallback(readerCallback);
        reader.start();

        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
