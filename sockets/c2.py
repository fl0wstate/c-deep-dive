#!/usr/bin/env python3
import asyncio
import websockets
import aioconsole
import sys

username = input("Please choose your username: ")

async def connect_to_server():
    uri = "ws://localhost:8080"
    
    try:
        async with websockets.connect(uri) as websocket:
            print(f"Connected to the WebSocket server as {username}")
            print("Type your messages (press Enter to send, Ctrl+C to exit)")
            
            async def send_messages():
                while True:
                    try:
                        message = await aioconsole.ainput("")
                        if not message:  # Skip empty messages
                            continue
                        if message.lower() == "exit":
                            print(f"Goodbye, {username}!")
                            sys.exit(0)
                        # Only send actual messages, not control frames
                        await websocket.send(f"{username}: {message}")
                    except Exception as e:
                        print(f"Error sending message: {e}")
                        break

            async def receive_messages():
                while True:
                    try:
                        response = await websocket.recv()
                        if not response:  # Skip empty messages
                            continue
                        print('\r' + ' ' * (len(username) + 2), end='')
                        print(f"\r{response}")
                        print(f"{username}> ", end='', flush=True)
                    except websockets.ConnectionClosed:
                        print("\nConnection closed by the server")
                        break
                    except Exception as e:
                        print(f"\nError receiving message: {e}")
                        break

            tasks = [
                asyncio.create_task(send_messages()),
                asyncio.create_task(receive_messages())
            ]
            
            try:
                await asyncio.gather(*tasks)
            except Exception as e:
                for task in tasks:
                    task.cancel()
                await asyncio.gather(*tasks, return_exceptions=True)
                raise

    except KeyboardInterrupt:
        print(f"\nGoodbye, {username}!")
    except Exception as e:
        print(f"Connection error: {e}")

if __name__ == "__main__":
    try:
        asyncio.run(connect_to_server())
    except KeyboardInterrupt:
        print(f"\nGoodbye, {username}!")
