#!/usr/bin/env python3

# its a cheat but will have to make it work

from socket import socket, AF_INET, SOCK_STREAM

HOST = "127.0.0.1"
PORT = 7000

# make a python client, request for the name,
# make sure that client can create multiple rooms (later on)
with socket(AF_INET, SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(b"Hello there i a python client")
    data = s.recv(1024)

print(f"Received: {data!r}")


# make a class that takes in the client name and room name
# creates a simple message that will be sent to the first client
