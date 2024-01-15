#include <iostream>
#include <unistd.h>
// #include "controller/controller.h"
#include "rx_reader/rx_reader.h"
#include <iomanip>
#include <signal.h>
#include <thread>

/*
g++ -o rob main.cpp rx_reader/rx_reader.cpp -lpigpio
*/

#define PIN 17
#define SBUS_CHANNEL_COUNT 16

std::chrono::system_clock::time_point lastTimestamp;
RxReader reader(PIN);
// MotorController controller;

void cleanup(int signal)
{
    std::cout << std::endl << "Received " << signal << ", cleaning up and exiting..." << std::endl;
    reader.stop();
    exit(0);
}

// void moveCursorTo(int row, int col)
// {
//     std::cout << "\033[" << row << ";" << col << "H";
// }

void readerCallback(const RxReader::RxState &rxState)
{
    if (rxState.timestamp > lastTimestamp)
    {
        std::cout << "New values" << std::endl;
        lastTimestamp = rxState.timestamp;
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

    try
    {
        system("clear");

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
