#!/usr/bin/env python3
import sys
import requests
import time

# 1. Use \r\n for headers
# 2. Don't send headers until you know the remote request worked
try:
    # Use stream=True to avoid loading the whole video into RAM
    response = requests.get('https://dn721809.ca.archive.org/0/items/youtube-xvFZjo5PgG0/xvFZjo5PgG0.mp4', stream=True)
    
    if response.status_code == 200:
        sys.stdout.buffer.write(b"Status: 200 OK\r\n")
        sys.stdout.buffer.write(b"Content-Type: video/mp4\r\n")
        sys.stdout.buffer.write(b"\r\n") # End of headers
        sys.stdout.buffer.flush()
        # Stream the chunks directly to the server
        for chunk in response.iter_content(chunk_size=4096):
            if chunk:
                sys.stdout.buffer.write(chunk)
                sys.stdout.buffer.flush()
    else:
        # Proper error status if the download fails
        sys.stdout.buffer.write(b"Status: 502 Bad Gateway\r\n\r\n")
        sys.stdout.buffer.write(b"Remote source returned error.")

except Exception as e:
    sys.stdout.buffer.write(b"Status: 500 Internal Server Error\r\n\r\n")
