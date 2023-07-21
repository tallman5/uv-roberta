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
std::chrono::system_clock::time_point lastTimestamp;

RxReader reader(PIN);
int fd = -1;

void readerCallback(const RxReader::RxState &rxState)
{
    if (rxState.timestamp > lastTimestamp)
    {
        // truncate(pipePath.c_str(), 0);
        off_t position = lseek(fd, 0, SEEK_SET);
        if (position == -1)
        {
            std::cerr << "Failed to set the position within the named pipe" << std::endl;
            close(fd);
            return;
        }

        // Calculate the size of the RxState struct
        std::size_t dataSize = sizeof(rxState.channelValues[0]) * rxState.channelValues.size() + sizeof(rxState.inFailsafe) + sizeof(rxState.ch17) + sizeof(rxState.ch18);

        // Create a byte buffer
        std::vector<char> buffer(dataSize);

        // Copy the struct data to the buffer
        char *bufferPtr = buffer.data();
        std::memcpy(bufferPtr, rxState.channelValues.data(), sizeof(rxState.channelValues[0]) * rxState.channelValues.size());
        bufferPtr += sizeof(rxState.channelValues[0]) * rxState.channelValues.size();
        std::memcpy(bufferPtr, &rxState.inFailsafe, sizeof(rxState.inFailsafe));
        bufferPtr += sizeof(rxState.inFailsafe);
        std::memcpy(bufferPtr, &rxState.ch17, sizeof(rxState.ch17));
        bufferPtr += sizeof(rxState.ch17);
        std::memcpy(bufferPtr, &rxState.ch18, sizeof(rxState.ch18));

        write(fd, buffer.data(), buffer.size());

        lastTimestamp = rxState.timestamp;
    }
}

void cleanup(int signal)
{
    std::cout << std::endl
              << "Cleaning up and exiting..." << std::endl;
    reader.stop();
    close(fd);
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

    fd = open(pipePath.c_str(), O_WRONLY);
    if (fd < 0)
    {
        std::cerr << "Failed to open the named pipe for writing" << std::endl;
        reader.stop();
        gpioTerminate();
        return 1;
    }

    while (true)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
