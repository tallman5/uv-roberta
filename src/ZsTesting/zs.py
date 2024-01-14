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

def set_direction(direction):
    pi.write(L_DIR_PIN, not direction)
    pi.write(R_DIR_PIN, direction)

def set_speed(new_speed):
    pi.set_PWM_dutycycle(L_SPEED_PIN, new_speed)
    pi.set_PWM_dutycycle(R_SPEED_PIN, new_speed)

def stop_motor():
    pi.set_PWM_dutycycle(L_SPEED_PIN, 0)
    pi.set_PWM_dutycycle(R_SPEED_PIN, 0)

try:
    set_direction(False)
    set_speed(100)
    time.sleep(5)
    set_speed(255)
    time.sleep(5)
    stop_motor()
    time.sleep(1)
    set_direction(True)
    set_speed(100)
    time.sleep(5)
    set_speed(255)
    time.sleep(5)

except KeyboardInterrupt:
    print("Program stopped by user")

finally:
    stop_motor()
    pi.stop()
