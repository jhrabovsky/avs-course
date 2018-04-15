#!/usr/bin/env python3

import socket
from threading import Thread

SERVER = "0.0.0.0"
MLEN = 80
PORT = 6050
QUEUE_LENGTH = 10

def HandleClient(paClientSocket, paClientAddr):
    message = paClientSocket.recv(MLEN)
    print("Message from [IP: {}, port: {}]: {}".format(paClientAddr[0], paClientAddr[1], message.decode()))
    paClientSocket.close()


if __name__ == "__main__":
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind((SERVER, PORT))
    sock.listen(QUEUE_LENGTH)

    while True:
        (clientSock, clientAddr) = sock.accept()
        t = Thread(target=HandleClient, args=(clientSock, clientAddr, ))
        t.start()

    sock.close()
