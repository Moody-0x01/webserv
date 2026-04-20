#!/usr/bin/env python3

import os

print("Content-Type: text/html")
print("Status: 200 OK")
print("x")
print()  # blank line = end of headers

print("<html>")
print("<body>")
print("<h1>Hello from CGI!</h1>")

print("<h2>Environment variables:</h2>")
for key, value in os.environ.items():
    print(f"<p>{key} = {value}</p>")
print("<h1> [Hello world from script] </h1>")
print("</body>")
print("</html>")
