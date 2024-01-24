#include "pidController.h"

PidController::PidController(double proportional_gain, double integral_gain, double derivative_gain)
    : kp(proportional_gain), ki(integral_gain), kd(derivative_gain), previous_error(0), integral(0) {}

double PidController::calculate(double setpoint, double pv, double dt) {
    // Calculate error
    double error = setpoint - pv;

    // Proportional term
    double P_out = kp * error;

    // Integral term
    integral += error * dt;
    double I_out = ki * integral;

    // Derivative term
    double derivative = (error - previous_error) / dt;
    double D_out = kd * derivative;

    // Calculate total output
    double output = P_out + I_out + D_out;

    // Save error to previous_error
    previous_error = error;

    return output;
}
