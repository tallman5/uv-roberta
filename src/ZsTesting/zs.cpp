#include <iostream>
#include <pigpio.h>
#include <unistd.h>

/*
g++ -o zs zs.cpp -lpigpio
*/

const int L_SPEED_PIN = 12;
const int L_DIR_PIN = 24;
const int R_SPEED_PIN = 13;
const int R_DIR_PIN = 23;

const int INITIAL_SPEED = 0;  // Speed range is 0 (off) to 255 (full speed)

int main() {
    gpioSetMode(L_SPEED_PIN, PI_OUTPUT);
    gpioSetMode(L_DIR_PIN, PI_OUTPUT);
    gpioSetMode(R_SPEED_PIN, PI_OUTPUT);
    gpioSetMode(R_DIR_PIN, PI_OUTPUT);

    auto setDirection = [](bool direction) {
        gpioWrite(L_DIR_PIN, !direction);
        gpioWrite(R_DIR_PIN, direction);
    };

    auto setSpeed = [](int newSpeed) {
        gpioPWM(L_SPEED_PIN, newSpeed);
        gpioPWM(R_SPEED_PIN, newSpeed);
    };

    auto stopMotor = []() {
        gpioPWM(L_SPEED_PIN, 0);
        gpioPWM(R_SPEED_PIN, 0);
    };

    try {
        setDirection(false);
        setSpeed(100);
        usleep(5000000); // Sleep for 5 seconds
        setSpeed(255);
        usleep(5000000); // Sleep for 5 seconds
        stopMotor();
        usleep(1000000); // Sleep for 1 second
        setDirection(true);
        setSpeed(100);
        usleep(5000000); // Sleep for 5 seconds
        setSpeed(255);
        usleep(5000000); // Sleep for 5 seconds
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    stopMotor();
    gpioTerminate();

    return 0;
}
