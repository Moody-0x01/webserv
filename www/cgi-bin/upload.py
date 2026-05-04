#!/usr/bin/env python3

import os
import sys
import urllib.parse

print("Content-Type: text/html\n")

UPLOAD_DIR = "/tmp/uploads"

if not os.path.exists(UPLOAD_DIR):
    try:
        os.makedirs(UPLOAD_DIR)
    except Exception as e:
        print(f"Error: Cannot create upload directory. {e}")
        sys.exit(1)

query_string = os.environ.get("QUERY_STRING", "")
parsed_query = urllib.parse.parse_qs(query_string)

filename = "uploaded_stream.bin"
if "filename" in parsed_query:
    filename = parsed_query["filename"][0]
filename = os.path.basename(filename)
filepath = os.path.join(UPLOAD_DIR, filename)
try:
    bytes_written = 0
    
    with open(filepath, 'wb') as f:
        while True:
            chunk = sys.stdin.buffer.read(8192) 
            if not chunk:
                break # EOF reached
            
            f.write(chunk)
            bytes_written += len(chunk)
            sys.stderr.buffer.write(f"File saved to: {filepath}\n".encode())
            sys.stderr.buffer.flush()
    print(f"<h1>File Upload Successful</h1>")
    print(f"<p> name: {filename}</p>")
except Exception as e:
    print(f"An unexpected error occurred: {e}")
