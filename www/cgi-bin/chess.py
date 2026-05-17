#!/usr/bin/env python3
import sys
import requests
import time

# url = ""

sys.stdout.buffer.write(b"Status: 301 Moved Permanently\r\n")
#
# sys.stdout.buffer.write(b"Status: 200 OK\r\n")
sys.stdout.buffer.write(b"Content-Type: text/html\r\n")
sys.stdout.buffer.write(b"Location: https://www.chess.com/\r\n")
sys.stdout.buffer.write(b"\r\n") # End of headers
sys.stdout.buffer.write(b"<h1> Go to url </h1>\r\n\r\n")
sys.stdout.buffer.flush()
