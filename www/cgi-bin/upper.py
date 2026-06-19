#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/plain")
print("Status: 200 OK")
print("")

try:
    # Safely extract dynamic content length from parent headers
    content_len = int(os.environ.get("CONTENT_LENGTH", 0))
    if content_len > 0:
        # Read precisely the incoming content body bytes from stdin
        incoming_body = sys.stdin.read(content_len)
        print("--- Modified Stdin Content ---")
        print(incoming_body.upper())
    else:
        print("[Warning]: Stdin buffer reached empty state. No CONTENT_LENGTH specified.")
except Exception as error:
    print(f"Error parsing incoming stream matrix: {str(error)}")
