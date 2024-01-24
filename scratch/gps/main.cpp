#include <iostream>
#include <libserial/SerialPort.h>
#include <libserial/SerialStream.h>
#include <unistd.h>
#include <string>

/*
g++ -o gps-test main.cpp -lserial
*/

int main()
{
    std::cout << "Starting..." << std::endl;
    using namespace LibSerial;

    std::cout << "Creating port...";
    // Create a SerialPort object.
    SerialPort serial_port;
    std::cout << "done" << std::endl;

    try
    {
        // Open the Serial Port at the desired hardware port.
        std::cout << "Opening port...";

        serial_port.Open("/dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.3:1.0-port0");
        // serial_port.Open("/dev/serial/by-path/platform-fd500000.pcie-pci-0000:01:00.0-usb-0:1.4:1.0-port0");
        // serial_port.Open("/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller-if00-port0");

        std::cout << "done" << std::endl;
    }
    catch (const OpenFailed &)
    {
        std::cerr << "\nThe serial port did not open correctly." << std::endl;
        return EXIT_FAILURE;
    }

    std::cout << "Configuring port..." << std::endl;
    // Set the baud rate of the serial port.
    serial_port.SetBaudRate(BaudRate::BAUD_4800);
    // Set the number of data bits.
    serial_port.SetCharacterSize(CharacterSize::CHAR_SIZE_8);
    // Turn off hardware flow control.
    serial_port.SetFlowControl(FlowControl::FLOW_CONTROL_NONE);
    // Disable parity.
    serial_port.SetParity(Parity::PARITY_NONE);
    // Set the number of stop bits.
    serial_port.SetStopBits(StopBits::STOP_BITS_1);

    // Time to wait after reading data.
    size_t msTimeout = 25;

    std::cout << "Listening..." << std::endl;
    // Continuously read data from serial port.
    while (true)
    {
        try
        {
            std::string data;
            serial_port.ReadLine(data, 0, msTimeout);
            std::cout << data;
        }
        catch (const ReadTimeout &)
        {
            std::cerr << "The Read() call has timed out." << std::endl;
        }
    }

    // Successful program completion.
    return EXIT_SUCCESS;
}
