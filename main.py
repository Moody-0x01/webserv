#!/bin/python3
import os

body = input("")
print("Content-Type: text/plain")
print("")

# 2. THE BODY
print("--- CGI Environment Variables Received ---")

# These are the variables you set in your envp vector
vars_to_check = [
    "REQUEST_METHOD", 
    "QUERY_STRING", 
    "CONTENT_LENGTH", 
    "SCRIPT_NAME", 
    "REMOTE_ADDR",
    "SERVER_PROTOCOL"
]

for var in vars_to_check:
    # os.environ.get avoids a crash if the variable is missing
    value = os.environ.get(var, "NOT SET")
    print(f"{var}: {value}")
print(f"Body: {body}")
print("\n--- End of Script ---")
