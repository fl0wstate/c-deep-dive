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
                        # Using aioconsole instead of input() for non-blocking input
                        message = await aioconsole.ainput("")
                        if message.lower() == "exit":
                            print(f"Goodbye, {username}!")
                            sys.exit(0)
                        await websocket.send(f"{message}")
                    except Exception as e:
                        print(f"Error sending message: {e}")
                        break

            async def receive_messages():
                while True:
                    try:
                        response = await websocket.recv()
                        # Clear the current line to prevent message overlap
                        print('\r' + ' ' * (len(username) + 2), end='')  # Clear the input prompt
                        print(f"\r{response}")
                        print(f"{username}> ", end='', flush=True)  # Reprint the prompt
                    except websockets.ConnectionClosed:
                        print("\nConnection closed by the server")
                        break
                    except Exception as e:
                        print(f"\nError receiving message: {e}")
                        await websocket.close()
                        break

            # Run both tasks concurrently
            await asyncio.gather(send_messages(), receive_messages())

    except KeyboardInterrupt:
        print(f"\nGoodbye, {username}!")
    except Exception as e:
        print(f"Connection error: {e}")

if __name__ == "__main__":
    try:
        asyncio.get_event_loop().run_until_complete(connect_to_server())
    except KeyboardInterrupt:
        print(f"\nGoodbye, {username}!")
