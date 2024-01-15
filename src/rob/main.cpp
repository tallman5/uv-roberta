#include <iostream>
#include <unistd.h>
#include "controller/controller.h"

int main() {
    try {
        MotorController motorController;

        motorController.setDirection(false);
        motorController.setSpeed(100);
        usleep(5000000); // Sleep for 5 seconds
        motorController.setSpeed(255);
        usleep(5000000); // Sleep for 5 seconds
        motorController.stopMotor();
        usleep(1000000); // Sleep for 1 second
        motorController.setDirection(true);
        motorController.setSpeed(100);
        usleep(5000000); // Sleep for 5 seconds
        motorController.setSpeed(255);
        usleep(5000000); // Sleep for 5 seconds
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
