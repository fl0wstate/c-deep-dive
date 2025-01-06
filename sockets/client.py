#!/usr/bin/env python3

# its a cheat but will have to make it work

from socket import socket, AF_INET, SOCK_STREAM

HOST = "127.0.0.1"
PORT = 7000

while true:
    with socket(AF_INET, SOCK_STREAM) as s:
        s.connect((HOST, PORT))
        s.sendall(b"Hello there i a python client")
        data = s.recv(1024)

print(f"Received: {data!r}")

