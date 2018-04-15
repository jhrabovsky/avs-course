#!/usr/bin/env python3

import socket

SERVER = "127.0.0.1"
PORT   = 6050

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.connect((SERVER, PORT))

message = input("Enter message: ")
sock.send(message.encode())
sock.close()
