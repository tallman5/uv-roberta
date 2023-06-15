import time
import serial

ser = serial.Serial(

    port = '/dev/serial/by-id/usb-Roboteq_Motor_Controller_MDC2XXX-if00',
    baudrate = 9600,
    parity = serial.PARITY_NONE,
    stopbits = serial.STOPBITS_ONE,
    bytesize = serial.EIGHTBITS,
    timeout = 1
)

while True:
    command = '!M 500 500 \r'
    ser.write(command.encode())

    data = ser.readline().decode().strip()
    print(data)

ser.close()