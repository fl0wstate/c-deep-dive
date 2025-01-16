#!/usr/bin/env python3

import asyncio
import websockets

async def connect_to_server():
    uri = "ws://localhost:8080"  # Change this to your server's URI
    async with websockets.connect(uri) as websocket:
        print("Connected to the WebSocket server")

        async def send_periodic_messages():
            while True:
                message = "Hello, Server!"
                await websocket.send(message)
                print(f"Sent: {message}")
                await asyncio.sleep(5)  # Send a message every 5 seconds

        async def receive_messages():
            while True:
                try:
                    response = await websocket.recv()
                    print(f"Received: {response}")
                except websockets.ConnectionClosed:
                    print("Connection closed by the server")
                    break

        # Run both sending and receiving concurrently
        send_task = asyncio.create_task(send_periodic_messages())
        receive_task = asyncio.create_task(receive_messages())

        await asyncio.gather(send_task, receive_task)

# Run the client
asyncio.get_event_loop().run_until_complete(connect_to_server())
