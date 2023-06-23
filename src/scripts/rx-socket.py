import json
import read_sbus_from_GPIO
import socket
import time

# SBUS
SBUS_PIN = 4
reader = read_sbus_from_GPIO.SbusReader(SBUS_PIN)
reader.begin_listen()
while(not reader.is_connected()):
    time.sleep(.2)
time.sleep(.1)

# Socket
HOST = ''  # Bind to all available interfaces
PORT = 12345  # Use a free port number
server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
# Bind the socket to a specific address and port
server_socket.bind((HOST, PORT))
# Listen for incoming connections
server_socket.listen(1)
print('Waiting for socket connections...')

while True:
    # Accept a client connection
    client_socket, client_address = server_socket.accept()
    print('Connected to:', client_address)

    try:
        while True:
            channel_data = reader.translate_latest_packet()
            # Send the processed data back to the client
            channelsJson = json.dumps(channel_data)
            try:
                client_socket.sendall(channelsJson.encode('utf-8'))
                time.sleep(.01)
            except:
                time.sleep(.01)

    except KeyboardInterrupt:
        print()
        reader.end_listen()
        client_socket.close()
        exit()
    except:
        print()
        reader.end_listen()
        client_socket.close()
        raise