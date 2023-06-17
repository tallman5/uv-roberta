import evdev

# Define the event device path
event_device = '/dev/input/by-id/usb-FrSky_FrSky_Simulator_4995316A3546-event-joystick'  # Replace with the actual device path

# Create an InputDevice object
gamepad = evdev.InputDevice(event_device)

# Read input events from the gamepad
for event in gamepad.read_loop():
    if event.type == evdev.ecodes.EV_KEY:
        # Button event
        key_event = evdev.categorize(event)
        print(f"Button {key_event.keycode} {'pressed' if key_event.keystate == 1 else 'released'}")

    elif event.type == evdev.ecodes.EV_ABS:
        # Axis event
        axis_event = evdev.categorize(event)
        print(f"Axis {axis_event.event.code} value: {axis_event.event.value}")
