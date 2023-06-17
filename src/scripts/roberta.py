import gps
import os
import read_sbus_from_GPIO
import roboteq
import sys
import time

GPS_PORT = '/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0'
ROBOTEQ_PORT = '/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00'
RX_PIN = 4

def clean_up():
    move_cursor(10, 0)
    print('Closing connections...')
    time.sleep(.1)
    gpsReader.close()
    rxReader.end_listen()
    roboteqConnection.close()

def clear_console():
    os.system('cls' if os.name == 'nt' else 'clear')

def move_cursor(row, col):
    sys.stdout.write('\033[{};{}H'.format(row, col))
    sys.stdout.flush()

def read_gps():
    line = gpsReader.read()
    if line.startswith('$GPGGA'):
        # Split the line into individual fields
        fields = line.split(',')

        # Print the GPS information
        print('-----------------------------------')
        print(f'  GPS Time: {fields[1]}            ')
        print(f'  Latitude: {fields[2]}            ')
        print(f' Longitude: {fields[4]}            ')

    move_cursor(10, 0)

def read_sbus():
    move_cursor(0, 0)

    channel_data = rxReader.translate_latest_packet()
    channels_to_watch = [0,1,8,9]
    for i in channels_to_watch:
        print(f'Channel {(i+1):0>2}: {channel_data[i]}             ')

    armed = False
    if (channel_data[8] > 1000): armed = True
    print(f'     Armed: {armed}               ')

    multiplier = .33
    if channel_data[9] > 600: multiplier = .66
    if channel_data[9] > 1200: multiplier = 1
    print(f'Multiplier: {multiplier}               ')
    
    if (armed == True):
        powerCommand = f'!M {(channel_data[1]-1000) * multiplier} {(channel_data[0]-1000) * multiplier}'
        roboteqConnection.write(powerCommand)

rxReader = read_sbus_from_GPIO.SbusReader(RX_PIN)
rxReader.begin_listen()
print('Waiting for RX connection...')
while(not rxReader.is_connected()):
    time.sleep(.2)
time.sleep(.1)

print('Waiting for GPS connection...')
gpsReader = gps.Reader(GPS_PORT)
gpsReader.open()
line = gpsReader.read()
while line[0] != '$':
    line = gpsReader.read()
    time.sleep(.2)

roboteqConnection = roboteq.Connection(ROBOTEQ_PORT)
roboteqConnection.open()

clear_console()

while True:
    try:
        read_sbus()
        # read_gps()
        time.sleep(.01)
    except KeyboardInterrupt:
        clean_up()
        exit()
    except:
        clean_up()
        raise
