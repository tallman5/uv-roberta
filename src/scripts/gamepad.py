import os
import struct
import sys

TIMESTAMP = 0
VALUE = 1
CODE = 2
CHANNEL = 3

# Replace '/dev/input/js0' with the appropriate path to your gamepad
gamepad_file = open('/dev/input/js0', 'rb')

event_format = 'IhBB'  # Event structure format
event_size = struct.calcsize(event_format)

def clear_console():
    os.system('cls' if os.name == 'nt' else 'clear')

def move_cursor(row, col):
    sys.stdout.write('\033[{};{}H'.format(row, col))
    sys.stdout.flush()

clear_console()

# Read gamepad events
while True:
    try:
        event_data = gamepad_file.read(event_size)
        event = struct.unpack(event_format, event_data)

        move_cursor(0, 0)

        try:
            print(f'{event_data}                      ')
        except:
            print("Cannot decode")

        print(f'{event}                                    ')

        if (event[CHANNEL] < 7):
            move_cursor(event[CHANNEL] + 3, 0)
            print(f'Channel {event[CHANNEL] + 1}: {event[VALUE] / 255}               ')

    except KeyboardInterrupt:
        move_cursor(11, 0)
        print()
        exit()
    except:
        raise
