#!/usr/bin/env python3

import os

file = "/home/lazmoud/Desktop/webserv/www/cgi-bin/ra.mp4"
data = b''
with open(file, "rb") as f:
    data = f.read()

print("Content-Type: video/mp4")
print("Status: 200 OK")
print(f"Content-Length: {len(data)}")
print()  # blank line = end of headers

print(data, end="")
