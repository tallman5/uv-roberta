#include <iostream>
#include <bitset>
#include "rx_reader.h"
#include <chrono>
#include <thread>
#include <signal.h>
#include <pigpio.h>
#include <vector>
#include <iomanip>

/*
g++ -o rx_reader_main rx_reader_main.cpp rx_reader.cpp -lpigpio
*/

#define PIN 17

RxReader reader(PIN);
std::chrono::system_clock::time_point lastTimestamp;

void readerCallback(const RxReader::RxState &rxState)
{
    if (rxState.timestamp > lastTimestamp)
    {
        // reader.writeBuffer();
        std::cout << "Failsafe: " << (rxState.inFailsafe ? "Y" : "N") << " ";
        for (const auto &val : rxState.channelValues)
            std::cout << std::setw(4) << std::setfill('0') << val << " ";
        std::cout << rxState.ch17 << " ";
        std::cout << rxState.ch18 << std::endl;

        lastTimestamp = rxState.timestamp;
    }
}

void cleanup(int signal)
{
    std::cout << std::endl
              << "Cleaning up and exiting..." << std::endl;
    reader.stop();
    gpioTerminate();
    exit(0);
}

int main()
{
    signal(SIGINT, cleanup);

    if (gpioInitialise() < 0)
    {
        std::cerr << "Failed to initialize pigpio" << std::endl;
        return 1;
    }

    reader.setReaderCallback(readerCallback);
    reader.start();

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
