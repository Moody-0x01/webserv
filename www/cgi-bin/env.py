#!/usr/bin/env python3
import os
import sys

# Format standard output headers
print("Content-Type: text/plain")
print("Status: 200 OK")
print("") # Blank line indicating header completion

print("=========================================")
print("       WEBSERV CGI ENVIRONMENT SYSTEM     ")
print("=========================================")
for key in sorted(os.environ.keys()):
    print(f"{key:<25} = {os.environ[key]}")
