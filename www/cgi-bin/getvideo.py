#!/usr/bin/env python3

import sys

sys.stdout.buffer.write("Status: 200 OK\n")
sys.stdout.buffer.write("Content-Type: video/mp4\n") # Add \r just to be safe with HTTP
sys.stdout.buffer.write("\n\r")  # Blank line for headers
sys.stdout.buffer.flush()

file = "/home/lazmoud/Downloads/Rick Astley - Never Gonna Give You Up (Official Video) (4K Remaster).mp4"

with open(file, "rb") as f:
    while True:
        data = f.read()
        if not data: 
            break
        sys.stdout.buffer.write(data)
        sys.stdout.buffer.flush()
