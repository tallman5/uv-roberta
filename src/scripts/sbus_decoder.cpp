#include <iostream>
#include <fstream>
#include <cstring>
#include <unistd.h>

// S.Bus Parameters
const int SBUS_NUM_CHANNELS = 16;
const int SBUS_FRAME_SIZE = 25;
const int SBUS_SIGNAL_LOST = 0x04;

// Serial Port Parameters
const char *SERIAL_PORT = "/dev/ttyAMA0";
const int SERIAL_BAUD = 100000;

// Function to parse the S.Bus data
void parseSBusData(unsigned char *buffer, int size)
{
    if (size >= SBUS_FRAME_SIZE)
    {
        // Check for valid SBUS signal
        if ((buffer[0] == 0x0F) && (buffer[SBUS_FRAME_SIZE - 1] == 0x00))
        {
            // Extract channel data
            int channels[SBUS_NUM_CHANNELS];
            channels[0] = ((buffer[1] | buffer[2] << 8) & 0x07FF);
            channels[1] = ((buffer[2] >> 3 | buffer[3] << 5) & 0x07FF);
            // Extract more channels here...

            // Print channel values
            for (int i = 0; i < SBUS_NUM_CHANNELS; ++i)
            {
                std::cout << "Channel " << i << ": " << channels[i] << std::endl;
            }

            // Check for signal loss
            if (buffer[SBUS_FRAME_SIZE - 2] & SBUS_SIGNAL_LOST)
            {
                std::cout << "Signal Lost" << std::endl;
            }
        }
    }
}

int main()
{
    std::cout << "46 ";

    // Open the serial port
    std::fstream serialPort;
    serialPort.open(SERIAL_PORT, std::ios::in | std::ios::binary);
    if (!serialPort.is_open())
    {
        std::cerr << "Unable to open serial port." << std::endl;
        return 1;
    }

    // Configure serial port settings
    serialPort.sync();
    serialPort.seekg(0, std::ios::beg);
    serialPort.clear();

    // Read and parse S.Bus data
    unsigned char buffer[SBUS_FRAME_SIZE];
    while (true)
    {
        serialPort.read(reinterpret_cast<char *>(buffer), SBUS_FRAME_SIZE);

        if (serialPort.gcount() == SBUS_FRAME_SIZE)
        {
            // Parse the received S.Bus data
            parseSBusData(buffer, SBUS_FRAME_SIZE);
        }
        else
        {
            // Sleep to avoid high CPU usage
            usleep(100);
        }
    }

    // Close the serial port
    serialPort.close();

    return 0;
}
