import requests

# Change this to match your webserv's IP, port, and route
url = 'http://localhost:9090/images/'

# Define your multiple headers here
headers = {
    'Content-Type': 'application/json',
    'Content-Length': '12',
    'User-Agent': 'Webserv-Tester-Script/1.0',
    'Accept': '*/*',
    'X-Custom-Header-1': 'Hello-From-Python',
    'X-Secret-Token': 'super-secret-42-token'
}

# The body of your POST request
payload = {
    "username": "norminet",
    "project": "webserv",
    "status": "testing"
}

print(f"Sending POST request to {url}...")

# Send the request
response = requests.post(url, headers=headers, json=payload)

# Print the results from your C++ server
print("\n--- SERVER RESPONSE ---")
print(f"Status Code: {response.status_code}")
print("Headers received back:")
for key, value in response.headers.items():
    print(f"  {key}: {value}")
print(f"Body:\n{response.text}")