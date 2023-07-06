#include <iostream>
#include <gpiod.h>
#include <sys/time.h>
#include <csignal>
#include <cstring>
#include <iomanip>
#include <cmath>

#define SBUS_FRAME_SIZE 298

const char *chipname = "/dev/gpiochip0"; // Change this based on the GPIO chip number
int line_number = 17;                    // Change this based on the GPIO line number

struct gpiod_chip *chip;
struct gpiod_line *line;
struct gpiod_line_event event;

// Function to get current time in microseconds
unsigned long long micros()
{
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return static_cast<unsigned long long>(tv.tv_sec) * 1000000 + tv.tv_usec;
}

// Signal handler for SIGINT (Ctrl+C)
void signalHandler(int signal)
{
    std::cout << std::endl;
    std::cout << "Cleaning up..." << std::endl;
    gpiod_line_release(line);
    gpiod_chip_close(chip);

    // Terminate the program
    exit(signal);
}

int main()
{
    std::cout << "Opening chip..." << std::endl;
    chip = gpiod_chip_open(chipname);
    if (!chip)
    {
        std::cerr << "Error opening GPIO chip" << std::endl;
        return 1;
    }

    std::cout << "Opening line..." << std::endl;
    line = gpiod_chip_get_line(chip, line_number);
    if (!line)
    {
        std::cerr << "Error getting GPIO line" << std::endl;
        gpiod_chip_close(chip);
        return 1;
    }

    std::cout << "Requesting event..." << std::endl;
    int ret = gpiod_line_request_both_edges_events(line, "gpio-event");
    if (ret < 0)
    {
        std::cerr << "Error requesting events" << std::endl;
        gpiod_line_release(line);
        gpiod_chip_close(chip);
        return 1;
    }

    std::signal(SIGINT, signalHandler);

    bool sbusBuffer[SBUS_FRAME_SIZE];
    int counter = 0;
    unsigned long long startTime = micros();

    std::cout << "Running..." << std::endl;
    while (true)
    {
        ret = gpiod_line_event_wait(line, nullptr);
        if (ret < 0)
        {
            std::cerr << "Error waiting for events" << std::endl;
            break;
        }

        ret = gpiod_line_event_read(line, &event);
        if (ret < 0)
        {
            std::cerr << "Error reading event" << std::endl;
            break;
        }

        unsigned int pulseWidth = std::round((micros() - startTime) / 9);
        int readVal = (event.event_type == GPIOD_LINE_EVENT_FALLING_EDGE) ? 1 : 0;
        int newVal = (-readVal + 1);

        if (pulseWidth > 500)
        {
            for (int i = 0; i < SBUS_FRAME_SIZE; i++)
            {
                std::cout << sbusBuffer[i];
            }
            std::cout << " " << counter << std::endl;

            // Start of new packet
            memset(sbusBuffer, 0, SBUS_FRAME_SIZE);
            counter = 0;
        }
        else
        {
            int newEnd = counter + pulseWidth;
            for (int i = counter; i < newEnd; i++)
            {
                if (i < SBUS_FRAME_SIZE)
                    sbusBuffer[i] = newVal;
            }
            counter = newEnd + 1;
        }

        startTime = micros();
    }

    return 0;
}
