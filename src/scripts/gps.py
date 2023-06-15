import serial
import time

BAUD_RATE = 4800

class Connection:
    def __init__(self, port):
        self.lastLine = ''
        self.port = port
        self.isOpen = False
    
    def open(self):
        self.ser = serial.Serial(self.port, BAUD_RATE)
        self.isOpen = True

        while (self.isOpen == True):
            try:
                self.lastLine = self.ser.readline().decode('utf-8').strip()
                time.sleep(.1)
            except:
                self.lastLine = '' 

        self.ser.close()

    def lastLine(self):
        return self.lastLine
    
    def close(self):
        self.isOpen = False
