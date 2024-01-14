import pigpio
import time

L_SPEED_PIN = 12
L_DIR_PIN = 24
R_SPEED_PIN = 13
R_DIR_PIN = 23

INITIAL_SPEED = 0   # Speed range is 0 (off) to 255 (full speed)

pi = pigpio.pi()
if not pi.connected:
    exit()

pi.set_mode(L_SPEED_PIN, pigpio.OUTPUT)
pi.set_mode(L_DIR_PIN, pigpio.OUTPUT)
pi.set_mode(R_SPEED_PIN, pigpio.OUTPUT)
pi.set_mode(R_DIR_PIN, pigpio.OUTPUT)

def start_motor(speed=INITIAL_SPEED, direction=True):
    """Start the motor with the given speed and direction."""
    pi.write(MOTOR_DIR_PIN, direction)  # True for one direction, False for the other
    pi.set_PWM_dutycycle(MOTOR_SPEED_PIN, speed)

def change_speed(new_speed):
    """Change the motor speed."""
    pi.set_PWM_dutycycle(MOTOR_SPEED_PIN, new_speed)

def change_direction(direction):
    """Change the motor direction."""
    pi.write(MOTOR_DIR_PIN, direction)

def stop_motor():
    """Stop the motor."""
    pi.set_PWM_dutycycle(MOTOR_SPEED_PIN, 0)

try:
    # Start the motor in the initial direction
    start_motor(direction=True)  # Set False for the opposite direction

    # Change speed and direction after 5 seconds
    time.sleep(5)
    change_speed(200)  # Change the speed as needed
    change_direction(False)  # Change direction

    # Run the motor for 10 more seconds
    time.sleep(10)

except KeyboardInterrupt:
    print("Program stopped by user")

finally:
    # Stop the motor and cleanup
    stop_motor()
    pi.stop()
