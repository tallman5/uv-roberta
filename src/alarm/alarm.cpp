#include <iostream>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <array>
#include <pigpiod_if2.h>
#include <csignal>
#include <chrono>
#include <thread>
#include <pigpio.h>
#include <ctime>
#include <sstream>
#include <iomanip>

/*
g++ -o alarm alarm.cpp -lpigpiod_if2 -pthread
*/

const int gpioPin = 17;
std::chrono::steady_clock::time_point last_call = std::chrono::steady_clock::now();
volatile sig_atomic_t interrupted = 0;
bool quietMode = false;

std::string getCurrentDateTime()
{
    // Get current time as a time_point
    auto now = std::chrono::system_clock::now();

    // Convert to a time_t object
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

    // Convert to local time
    std::tm *now_tm = std::localtime(&now_c);

    // Use stringstream to format the date and time
    std::stringstream ss;
    ss << std::put_time(now_tm, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}

void connectBluetoothSpeaker(const std::string &btAddress)
{
    std::string command = "bluetoothctl connect " + btAddress;
    system(command.c_str());
}

void setVolume(int volume)
{
    std::cout << getCurrentDateTime() << ": Setting volume to " << volume << "%" << std::endl;
    std::string command = "amixer set Master " + std::to_string(volume) + "%";
    std::system(command.c_str());
}

void executeCommand(int volume)
{
    std::cout << getCurrentDateTime() << ": Playing alarm.wav at a volumt of " << volume << "%" << std::endl;
    setVolume(volume);
    system("aplay alarm.wav");
    setVolume(0);
}

void gpioCallback(int pi, unsigned user_gpio, unsigned level, uint32_t tick)
{
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_call).count();

    std::cout << getCurrentDateTime() << ": level: " << level << std::endl;

    if (elapsed >= 60 && level == 1)
    {
        last_call = now;
        if (!quietMode)
        {
            std::cout << getCurrentDateTime() << ": Alarm Triggered" << std::endl;
            std::thread soundThread(executeCommand, 100);
            soundThread.detach();
        }
    }
}

bool isBluetoothSpeakerConnected(const std::string &btAddress)
{
    std::string command = "bluetoothctl info " + btAddress + " | grep 'Connected: yes'";
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(command.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
    {
        result += buffer.data();
    }
    return !result.empty();
}

void signal_handler(int signal)
{
    interrupted = 1;
}

int main(int argc, char *argv[])
{
    std::signal(SIGINT, signal_handler);

    for (int i = 1; i < argc; ++i)
    {
        std::string arg(argv[i]);
        if (arg == "-q")
        {
            std::cout << getCurrentDateTime() << ": Setting mode to quiet" << std::endl;
            quietMode = true;
            break;
        }
    }

    if (!quietMode)
    {
        std::string btSpeakerAddress = "98:52:3D:C2:57:51";
        if (!isBluetoothSpeakerConnected(btSpeakerAddress))
        {
            std::cout << getCurrentDateTime() << ": Connecting to Bluetooth speaker..." << std::endl;
            connectBluetoothSpeaker(btSpeakerAddress);
        }
        else
        {
            std::cout << getCurrentDateTime() << ": Bluetooth speaker already connected." << std::endl;
        }
    }

    int pi = pigpio_start(NULL, NULL);
    if (pi < 0)
    {
        std::cerr << "Failed to start pigpio." << std::endl;
        return 1;
    }
    set_pull_up_down(pi, gpioPin, PI_PUD_UP);
    int callback_id = callback(pi, gpioPin, EITHER_EDGE, gpioCallback);
    if (callback_id < 0)
    {
        std::cerr << "Failed to set callback." << std::endl;
        pigpio_stop(pi);
        return 1;
    }

    std::cout << getCurrentDateTime() << ": Waiting for GPIO17 to change state. Press CTRL+C to exit." << std::endl;

    auto startTime = std::chrono::steady_clock::now();

    while (!interrupted)
    {
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsedMinutes = std::chrono::duration_cast<std::chrono::minutes>(currentTime - startTime).count();
        if (elapsedMinutes >= 10)
        {
            executeCommand(0);
            startTime = std::chrono::steady_clock::now();
        }

        time_sleep(5);
    }

    callback_cancel(callback_id);
    pigpio_stop(pi);
    std::cout << getCurrentDateTime() << ": Program terminated gracefully." << std::endl;
    return 0;
}
