import serial

BAUD_RATE = 4800

def check_string_encoding(text):
    try:
        text.decode('utf-8')
    except UnicodeDecodeError:
        return False
    return True

class Reader:
    def __init__(self, port):
        self.port = port
    
    def open(self):
        self.ser = serial.Serial(self.port, BAUD_RATE)

    def read(self):
        if hasattr(self, 'ser'):
            if self.ser.closed == True: return 'GPS connection is closed, did you call Reader.open()?'

            line = self.ser.readline()
            if check_string_encoding(line):
                decodedLine = line.decode('utf-8').strip()
                return decodedLine
            
            return line
        return 'GPS not initialized, did you call Reader.open()?'
    
    def close(self):
        if hasattr(self, 'ser'):
            if self.ser.closed == False:
                self.ser.close()
