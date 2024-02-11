import pigpio
import time
# import subprocess

# GPIO pin number where the Hall sensor is connected
HALL_SENSOR_PIN = 17  # Change this to the correct GPIO pin number

# Initialize pigpio
pi = pigpio.pi()
if not pi.connected:
    exit()

# Callback function to run when the Hall sensor is triggered
def sensor_triggered(gpio, level, tick):
    print("Hall sensor triggered")
    # try:
    #     # Replace 'your_shell_command' with your desired shell command
    #     subprocess.run(["paplay alarm.wav"], check=True)
    # except subprocess.CalledProcessError as e:
    #     print(f"Error occurred while executing shell command: {e}")

# Set the GPIO pin as an input
pi.set_mode(HALL_SENSOR_PIN, pigpio.INPUT)

# Set up an interrupt handler for when the pin goes HIGH or LOW
pi.callback(HALL_SENSOR_PIN, pigpio.EITHER_EDGE, sensor_triggered)

# Keep the script running
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    print("\nScript stopped by user")

# Clean up
pi.stop()
