#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <pigpio.h>

class MotorController
{
public:
    MotorController();
    ~MotorController();

    void setLR(int l, int r);
    void start();

private:
    const int L_SPEED_PIN = 12;
    const int L_DIR_PIN = 24;
    const int R_SPEED_PIN = 13;
    const int R_DIR_PIN = 23;
};

#endif // CONTROLLER_H
