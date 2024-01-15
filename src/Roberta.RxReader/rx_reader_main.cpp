#include <iostream>
#include <bitset>
#include "rx_reader.h"
#include <chrono>
#include <thread>
#include <signal.h>
#include <pigpio.h>
#include <vector>
#include <iomanip>
#include <fstream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

/*
g++ -o rx_reader_main rx_reader_main.cpp rx_reader.cpp -lpigpio
*/

#define PIN 17
const std::string pipePath = "/tmp/rxReader";
std::chrono::sgystem_clock::time_point lastTimestamp;

RxReader reader(PIN);
int fd = -1;

void readerCallback(const RxReader::RxState &rxState)
{
    if (rxState.timestamp > lastTimestamp)
    {
        std::cout << "New values" << std::endl;
        lastTimestamp = rxState.timestamp;
    }
}

void cleanup(int signal)
{
    std::cout << std::endl
              << "Cleaning up and exiting..." << std::endl;
    reader.stop();
    // close(fd);
    // gpioTerminate();
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

    // fd = open(pipePath.c_str(), O_WRONLY);
    // if (fd < 0)
    // {
    //     std::cerr << "Failed to open the named pipe for writing" << std::endl;
    //     reader.stop();
    //     gpioTerminate();
    //     return 1;
    // }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
