#!/usr/bin/env python3
import sys
import requests
import time
import json

json_data = input("")

sys.stderr.buffer.write("Got from client: ", json_data)
sys.stdout.buffer.write(b"Status: 200 OK\r\n")
sys.stdout.buffer.write(b"Content-Type: text/html\r\n")
sys.stdout.buffer.write(b"\r\n") # End of headers

# Sending the body
body = f"<h1> Json </h1><p> {json_data} </p>"
sys.stdout.buffer.write(body.encode())
sys.stdout.buffer.flush()
