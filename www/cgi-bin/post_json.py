#!/usr/bin/env python2
import sys
import os

# 1. Get the expected size from the environment variables set by your C++ server
content_length = int(os.environ.get('CONTENT_LENGTH', 0))

# 2. Read EXACTLY that many bytes from stdin
# sys.stdin.read() is better for text, sys.stdin.buffer.read() for binary
if content_length > 0:
    json_data = sys.stdin.read(content_length)
else:
    json_data = ""

print(f"Got from client[{len(json_data)}]: {json_data[:50]}...", file=sys.stderr)

sys.stdout.write("Status: 200 OK\r\n")
sys.stdout.write("Content-Type: text/html\r\n\r\n")

# 4. Send body
body = f"<h1> Json </h1><p> data recv size: {len(json_data)} </p>"
body += f"<h1> Json </h1><p> data recv size: {json_data} </p>"
sys.stdout.write(body)
sys.stdout.flush()
