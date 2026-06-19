#!/usr/bin/env python3
import sys
import os
import urllib.parse

# Function to parse query parameter strings cleanly manually
def get_query_param(param_name):
    query_string = os.environ.get("QUERY_STRING", "")
    params = urllib.parse.parse_qs(query_string)
    return params.get(param_name, [None])[0]

filename = get_query_param("file")

if not filename:
    print("Content-Type: text/plain")
    print("Status: 400 Bad Request")
    print("")
    print("Error: Missing target 'file' argument inside query context.")
    sys.exit(0)

# Path checking constraints
target_file_path = os.path.join(".", filename)

if not os.path.exists(target_file_path) or os.path.isdir(target_file_path):
    print("Content-Type: text/plain")
    print("Status: 404 Not Found")
    print("")
    print(f"Error: Target file reference '{filename}' could not be located on CGI subsystem storage.")
    sys.exit(0)

# Map media definitions to give clean instruction context to browser layout channels
extension = os.path.splitext(filename)[1].lower()
mime_types = {
    ".png":  "image/png",
    ".jpg":  "image/jpeg",
    ".jpeg": "image/jpeg",
    ".gif":  "image/gif",
    ".mp3":  "audio/mpeg",
    ".wav":  "audio/wav",
    ".mp4":  "video/mp4",
    ".webm": "video/webm"
}
content_type = mime_types.get(extension, "application/octet-stream")

# Flush data through stdout pipeline channels
print(f"Content-Type: {content_type}")
print(f"Content-Length: {os.path.getsize(target_file_path)}")
print("Status: 200 OK")
print("") # Flushes headers cleanly

sys.stdout.flush()

# Read and output binary chunks 
with open(target_file_path, "rb") as target_source:
    sys.stdout.buffer.write(target_source.read())
