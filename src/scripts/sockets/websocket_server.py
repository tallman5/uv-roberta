import asyncio
import websockets

# Define the WebSocket server handler
async def handle_websocket(websocket, path):
    # Send an initial message to the client
    await websocket.send("Connected to the Raspberry Pi WebSocket server!")

    # Continuously listen for messages from the client
    try:
        while True:
            message = await websocket.recv()
            print(f"Received message: {message}")

            # Handle different events based on the received message
            if message == "event1":
                # Perform actions for event1
                print("Event 1 occurred!")
                response = "Event 1 processed successfully"
                await websocket.send(response)
            elif message == "event2":
                # Perform actions for event2
                print("Event 2 occurred!")
                response = "Event 2 processed successfully"
                await websocket.send(response)
            else:
                # Invalid event
                print("Invalid event!")
                response = "Invalid event"
                await websocket.send(response)

    except websockets.exceptions.ConnectionClosed:
        print("WebSocket connection closed")

# Start the WebSocket server
async def start_server():
    server = await websockets.serve(handle_websocket, "0.0.0.0", 8765)
    print("WebSocket server started")
    await server.wait_closed()

# Run the WebSocket server
asyncio.get_event_loop().run_until_complete(start_server())
