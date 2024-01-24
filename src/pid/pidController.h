#ifndef PIDCONTROLLER_H
#define PIDCONTROLLER_H

class PidController {
private:
    double kp; // Proportional gain
    double ki; // Integral gain
    double kd; // Derivative gain

    double previous_error;
    double integral;

public:
    PidController(double proportional_gain, double integral_gain, double derivative_gain);
    double calculate(double setpoint, double pv, double dt);
};

#endif // PIDCONTROLLER_H
