#!/usr/bin/env python3
import sys
import os

print("Content-Type: text/plain")

try:
    filename = os.environ.get("HTTP_X_TARGET_FILENAME", "uploaded_media.bin")
    content_len = int(os.environ.get("CONTENT_LENGTH", 0))
    
    print(f"file = {filename}", file=sys.stderr)
    print(f"Expected content_len = {content_len}", file=sys.stderr)
    
    if content_len > 0:
        binary_payload = bytearray()
        bytes_received = 0
        chunk_size = 4096  # Read in safe 4KB increments
        
        print(f"INNNN: Starting incremental read loop...", file=sys.stderr)
        
        while bytes_received < content_len:
            # Calculate how much to read next so we don't over-read past content_len
            remaining = content_len - bytes_received
            to_read = min(chunk_size, remaining)
            
            chunk = sys.stdin.buffer.read(to_read)
            if not chunk:
                # If the pipe returns EOF early, the server closed the connection prematurely
                print(f"ALERT: Pipe closed early! Got {bytes_received} of {content_len} bytes.", file=sys.stderr)
                break
                
            binary_payload.extend(chunk)
            bytes_received += len(chunk)
            
            # This will show you exactly where the server stops sending data
            print(f"Progress: {bytes_received}/{content_len} bytes read.", file=sys.stderr)
        
        print(f"OUT:: Total payload read = {len(binary_payload)} bytes", file=sys.stderr)
        
        # Save the file
        storage_path = os.path.join(".", filename)
        with open(storage_path, "wb") as storage_vault:
            storage_vault.write(binary_payload)
        
        print("Status: 201 Created")
        print("")
        print(f"CGI Success: Saved {len(binary_payload)} bytes cleanly to: {filename}")
    else:
        print("Status: 400 Bad Request")
        print("")
        print("CGI Failure: CONTENT_LENGTH came back empty or unreadable.")

except Exception as error:
    print("Status: 500 Internal Server Error")
    print("")
    print(f"CGI Pipeline Crashed: {str(error)}")
