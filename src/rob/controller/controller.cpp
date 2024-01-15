#include <stdexcept>
#include "controller.h"

MotorController::MotorController() {
    gpioSetMode(L_SPEED_PIN, PI_OUTPUT);
    gpioSetMode(L_DIR_PIN, PI_OUTPUT);
    gpioSetMode(R_SPEED_PIN, PI_OUTPUT);
    gpioSetMode(R_DIR_PIN, PI_OUTPUT);
}

MotorController::~MotorController() {
    stopMotor();
    gpioTerminate();
}

void MotorController::setDirection(bool direction) {
    gpioWrite(L_DIR_PIN, !direction);
    gpioWrite(R_DIR_PIN, direction);
}

void MotorController::setSpeed(int newSpeed) {
    gpioPWM(L_SPEED_PIN, newSpeed);
    gpioPWM(R_SPEED_PIN, newSpeed);
}

void MotorController::stopMotor() {
    gpioPWM(L_SPEED_PIN, 0);
    gpioPWM(R_SPEED_PIN, 0);
}
