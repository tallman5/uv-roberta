#include <iostream>
#include <unistd.h>
#include "controller/controller.h"
#include "rx_reader/rx_reader.h"
#include "pid/pidController.h"
#include <iomanip>
#include <csignal>
#include <atomic>
#include <thread>
#include <pigpio.h>

/*
g++ -o rob rob_main.cpp rx_reader/rx_reader.cpp controller/controller.cpp pid/pidContoller.cpp -lpigpio
*/

#define PIN 17
#define SBUS_CHANNEL_COUNT 16

std::atomic<bool> running(true);
std::chrono::system_clock::time_point lastTimestamp;
RxReader reader(PIN);
MotorController controller;

void handleSignal(int signal)
{
    if (signal == SIGINT)
    {
        std::cout << "\nCtrl+C pressed. Shutting down..." << std::endl << std::endl;
        running = false;
    }
}

void moveCursorTo(int row, int col)
{
    std::cout << "\033[" << row << ";" << col << "H";
}

void readerCallback(const RxReader::RxState &rxState)
{
    if (rxState.timestamp > lastTimestamp)
    {
        // Print channel values
        moveCursorTo(0, 0);
        int channelIndex = 0;

        std::cout << "Failsafe: " << (rxState.inFailsafe ? "Y" : "N") << std::endl;
        for (const auto &val : rxState.channelValues)
        {
            std::cout << std::setw(2) << std::setfill('0') << channelIndex << ": ";
            std::cout << std::setw(4) << std::setfill('0') << val << std::endl;
            channelIndex++;
        }
        std::cout << "17: " << rxState.ch17 << "   " << std::endl;
        std::cout << "18: " << rxState.ch18 << "   " << std::endl;
        std::cout << "L:  " << rxState.L << "            " << std::endl;
        std::cout << "R:  " << rxState.R << "            " << std::endl;

        lastTimestamp = rxState.timestamp;

        if (rxState.inFailsafe)
            controller.setLR(0, 0);
        else
            controller.setLR(rxState.L, rxState.R);
    }
}

int main()
{
    if (gpioInitialise() < 0)
    {
        std::cerr << "Failed to initialize pigpio" << std::endl;
        return 1;
    }

    signal(SIGINT, handleSignal);

    try
    {
        system("clear");

        reader.setReaderCallback(readerCallback);
        reader.start();
        controller.start();

        while (running)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        reader.stop();
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        controller.setLR(0, 0);
        gpioTerminate();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
