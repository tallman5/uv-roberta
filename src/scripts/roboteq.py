import serial

class Connection:
    def __init__(self, port):
        self.port = port
        self._mxmd = 1

    def close(self):
        if self.connected():
            self.ser.close()

    def connected(self):
        if hasattr(self, 'ser'):
            if self.ser.closed == False:
                return True
        print('Roboteq connection not initialized, did you call open()?')
        return False

    def open(self):
        self.ser  = serial.Serial(
            port = self.port,
            baudrate = 9600,
            parity = serial.PARITY_NONE,
            stopbits = serial.STOPBITS_ONE,
            bytesize = serial.EIGHTBITS,
            timeout = 1
        )
        self.write('^ECHOF 1')  # Turn off response messages
        self.write(f'^MXMD {self._mxmd}')   # Set to tank mixing on right Tx stick

    def write(self, line):
        if self.connected():
            newLine = line + '\r'
            self.ser.write(newLine.encode())
