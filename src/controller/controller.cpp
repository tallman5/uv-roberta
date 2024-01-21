#include <iostream>
#include <stdexcept>
#include "controller.h"

MotorController::MotorController()
{
}

MotorController::~MotorController()
{
    // setLR(0, 0);
}

void MotorController::start()
{
    gpioSetMode(L_SPEED_PIN, PI_OUTPUT);
    gpioSetMode(L_DIR_PIN, PI_OUTPUT);
    gpioSetMode(R_SPEED_PIN, PI_OUTPUT);
    gpioSetMode(R_DIR_PIN, PI_OUTPUT);
}

void MotorController::setLR(int l, int r)
{
    if (l < 0)
    {
        gpioWrite(L_DIR_PIN, false);
        gpioPWM(L_SPEED_PIN, l * -1);
    }
    else
    {
        gpioWrite(L_DIR_PIN, true);
        gpioPWM(L_SPEED_PIN, l);
    }

    if (r < 0)
    {
        gpioWrite(R_DIR_PIN, true);
        gpioPWM(R_SPEED_PIN, r * -1);
    }
    else
    {
        gpioWrite(R_DIR_PIN, false);
        gpioPWM(R_SPEED_PIN, r);
    }
}
