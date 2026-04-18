#!/usr/bin/env python3

import sys
import time
sys.stdout.buffer.write(b"Status: 200 OK\n")
sys.stdout.buffer.write(b"Content-Type: video/mp4\n") # Add \r just to be safe with HTTP
sys.stdout.buffer.write(b"\n\r")  # Blank line for headers
sys.stdout.buffer.flush()

# time.sleep(40)

file = "/home/lazmoud/Downloads/Rick Astley - Never Gonna Give You Up (Official Video) (4K Remaster).mp4"

with open(file, "rb") as f:
    while True:
        data = f.read(4096)
        if not data: 
            break
        sys.stdout.buffer.write(data)

sys.stdout.buffer.flush()
