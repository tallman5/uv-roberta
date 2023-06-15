import serial

# Configure serial port
port = '/dev/serial/by-id/usb-Prolific_Technology_Inc._USB-Serial_Controller_D-if00-port0'
baud_rate = 4800

# Open the serial port
ser = serial.Serial(port, baud_rate)

# Read and print GPS data
try:
    while True:
        # Read a line from the serial port
        line = ser.readline()

        try:      
            # Decode the line from bytes to string
            line = line.decode('utf-8').strip()
            
            # Check if the line contains GPS data
            if line.startswith('$GPGGA'):
                # Split the line into individual fields
                fields = line.split(',')
                
                # Extract relevant GPS information
                time = fields[1]
                latitude = fields[2]
                longitude = fields[4]
                
                # Print the GPS information
                print('Time:', time)
                print('Latitude:', latitude)
                print('Longitude:', longitude)
                print('---')
        except:
            print('')
            
except KeyboardInterrupt:
    # Close the serial port on Ctrl+C
    ser.close()
