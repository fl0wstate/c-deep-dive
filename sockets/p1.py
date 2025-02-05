import asyncio
import websockets

async def send_message():
    async with websockets.connect("ws://localhost:8080") as websocket:
        print("Connected to WebSocket server. Type 'exit' to quit.")
        while True:
            message = input("Enter a message to send: ")
            if message.lower() == "exit":
                print("Exiting...")
                break

            await websocket.send(message)
            print(f"Sent: {message}")

            response = await websocket.recv()
            print(f"Received: {response}")

asyncio.run(send_message())
