#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unistd.h>
#include <iomanip>
#include <cstring>
#include <thread>
#include <chrono>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <thread>
#include <atomic>
#include <csignal>
#include <sys/epoll.h>
#include <fcntl.h>

// GPIO pin number
const int PIN_NUMBER = 4;

// Path to the GPIO directory
const std::string GPIO_PATH = "/sys/class/gpio/";

// Function to export the GPIO pin
bool exportPin(int pinNumber)
{
    std::ofstream exportFile(GPIO_PATH + "export");
    if (exportFile)
    {
        exportFile << pinNumber;
        return true;
    }
    else
    {
        std::cerr << "Failed to export GPIO pin." << std::endl;
        return false;
    }
}

// Function to set the GPIO pin direction
bool setPinDirection(int pinNumber, const std::string& direction)
{
    std::string gpioPinPath = GPIO_PATH + "gpio" + std::to_string(pinNumber) + "/direction";
    std::ofstream directionFile(gpioPinPath);
    if (directionFile)
    {
        directionFile << direction;
        return true;
    }
    else
    {
        std::cerr << "Failed to set GPIO pin direction." << std::endl;
        return false;
    }
}

int main()
{
    // Export the GPIO pin
    if (!exportPin(PIN_NUMBER))
    {
        return 1;
    }

    // Set the GPIO pin as input
    if (!setPinDirection(PIN_NUMBER, "in"))
    {
        return 1;
    }

    // Open the GPIO value file for the pin
    std::string gpioValuePath = GPIO_PATH + "gpio" + std::to_string(PIN_NUMBER) + "/value";
    int gpioValueFile = open(gpioValuePath.c_str(), O_RDONLY);
    if (gpioValueFile == -1)
    {
        std::cerr << "Failed to open GPIO value file." << std::endl;
        return 1;
    }

    // Open the GPIO edge file for the pin and set it to "both" for both rising and falling edge events
    std::string gpioEdgePath = GPIO_PATH + "gpio" + std::to_string(PIN_NUMBER) + "/edge";
    int gpioEdgeFile = open(gpioEdgePath.c_str(), O_WRONLY);
    if (gpioEdgeFile == -1)
    {
        std::cerr << "Failed to open GPIO edge file." << std::endl;
        return 1;
    }
    if (write(gpioEdgeFile, "both", 4) == -1)
    {
        std::cerr << "Failed to set GPIO edge." << std::endl;
        return 1;
    }

    // Create an epoll instance
    int epollfd = epoll_create1(0);
    if (epollfd == -1)
    {
        std::cerr << "Failed to create epoll instance." << std::endl;
        return 1;
    }

    // Register the GPIO value file for monitoring
    struct epoll_event event;
    event.events = EPOLLPRI;  // Set the event type to EPOLLPRI for high-priority data (edge-triggered)
    event.data.fd = gpioValueFile;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, gpioValueFile, &event) == -1)
    {
        std::cerr << "Failed to add GPIO value file to epoll." << std::endl;
        return 1;
    }

    // Buffer to read the interrupt event
    constexpr int MAX_BUF = 64;
    char buffer[MAX_BUF];

    while (true)
    {
        // Wait for an event to occur
        int ready = epoll_wait(epollfd, &event, 1, -1);
        if (ready == -1)
        {
            std::cerr << "Failed to wait for epoll event." << std::endl;
            return 1;
        }

        // Read the interrupt event
        lseek(gpioValueFile, 0, SEEK_SET);  // Reset file offset to read from the beginning
        ssize_t bytesRead = read(gpioValueFile, buffer, MAX_BUF - 1);
        if (bytesRead == -1)
        {
            std::cerr << "Failed to read GPIO value." << std::endl;
            return 1;
        }

        // Null-terminate the buffer
        buffer[bytesRead] = '\0';

        // Handle the pin change event
        std::cout << "Pin 4 value changed: " << buffer << " Size: " << bytesRead << std::endl;
    }

    // Cleanup and exit
    close(gpioValueFile);
    close(gpioEdgeFile);
    close(epollfd);

    return 0;
}
