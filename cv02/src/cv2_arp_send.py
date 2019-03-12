#!/usr/bin/env python3

import socket
import struct

IFACE = "eth0"
SRC_IP = "192.168.88.244"
ETHER_TYPE = 0x0806

HW_TYPE = 0x0001
PROTO_TYPE = 0x0800
HW_LEN = 6
PROTO_LEN = 4

OPCODE_REQ = 1
OPCODE_RESP = 2

if __name__ == "__main__":
    dst_ip = input("Enter destination IP: ")

    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
    sock.bind((IFACE, 0))

    # ! in struct.pack - network byte order 
    dst_mac = struct.pack("!6B", 0xff,0xff,0xff,0xff,0xff,0xff) #broadcast MAC
    src_mac = struct.pack("!6B", 0xf0,0xde,0xf1,0x12,0x14,0x10) #source MAC   
    ethertype = struct.pack("!H", ETHER_TYPE)
    eth_hdr = dst_mac + src_mac + ethertype

    arp_hdr = struct.pack("!HHBBH", HW_TYPE, PROTO_TYPE, HW_LEN, PROTO_LEN, OPCODE_REQ) + src_mac + socket.inet_aton(SRC_IP) + struct.pack("!6B", 0,0,0,0,0,0) + socket.inet_aton(dst_ip)
  
    sock.send(eth_hdr + arp_hdr)
    sock.close()

