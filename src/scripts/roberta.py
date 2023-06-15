import os
import read_sbus_from_GPIO
import serial
import sys
import time

GPS_PORT = '/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0'
ROBOTEQ_PORT = '/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00'
SBUS_PIN = 4

def check_string_encoding(text):
    try:
        text.decode('utf-8')
    except UnicodeDecodeError:
        return False
    return True

def clean_up():
    print()
    print()
    print()
    print()
    print('Closing connections...')
    reader.end_listen()
    # gpsConnection.close()
    roboteqConnection.close()

def clear_console():
    os.system('cls' if os.name == 'nt' else 'clear')

def move_cursor(row, col):
    sys.stdout.write('\033[{};{}H'.format(row, col))
    sys.stdout.flush()

# def read_gps():
#         if (gpsConnection.closed == True): return

#         line = gpsConnection.readline()
#         if check_string_encoding(line):
#             decodedLine = line.decode('utf-8').strip()

#             # Check if the line contains GPS data
#             if decodedLine.startswith('$GPGGA'):
#                 # Split the line into individual fields
#                 fields = decodedLine.split(',')

#                 move_cursor(0, 0)
#                 # Print the GPS information
#                 print('Time:', fields[1])
#                 print('Latitude:', fields[2])
#                 print('Longitude:', fields[4])

def read_sbus():
    channel_data = reader.translate_latest_packet()
    move_cursor(4, 0)

    armed = False
    if (channel_data[8] > 1000): armed = True

    print('Channel 1:', channel_data[0])
    print('Channel 2:', channel_data[1])
    print('Channel 9:', channel_data[8])
    print('Armed:    ', armed)

    if (armed == True):
        powerCommand = f'!M {channel_data[1]-1000} {(channel_data[0]-1000)*-1}'
        print(f'{powerCommand}               ')
        write_roboteq(powerCommand)

def write_roboteq(line):
    newLine = line + ' \r'
    roboteqConnection.write(newLine.encode())

reader = read_sbus_from_GPIO.SbusReader(SBUS_PIN)
reader.begin_listen()
while(not reader.is_connected()):
    time.sleep(.2)
time.sleep(.1)

# gpsConnection = serial.Serial(GPS_PORT, 4800)
roboteqConnection = serial.Serial(
    port = '/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00',
    baudrate = 9600,
    parity = serial.PARITY_NONE,
    stopbits = serial.STOPBITS_ONE,
    bytesize = serial.EIGHTBITS,
    timeout = 1
)
write_roboteq('^ECHOF 1')
write_roboteq('^MXMD 1')

clear_console()
    
while True:
    try:
        # read_gps()
        read_sbus()

    except KeyboardInterrupt:
        clean_up()
        exit()
    except:
        clean_up()
        raise
