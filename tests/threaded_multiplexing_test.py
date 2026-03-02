#!/usr/bin/env python3
import socket
import threading
import sys

HOST = '127.0.0.1'
PORT = 8080 # Change this if your C++ server is on a different port

# Here we craft the EXACT raw string we want to send. 
# No library can stop us from sending this garbage data now!
RAW_HTTP_REQUEST = (
    "GET /index.html HTTP/1.1\r\n"
    "Host: localhost:8080\r\n"
    "User-Agent: Mozilla/902.0:12 :: test               :\r\n"
    "Accept:text/html\r\n"
    "X-Empty-Value:\r\n"
    "X-Trailing-Spaces: Some value       \r\n"
    "cOnTeNt-LeNgTh: 42\r\n"
    "X-Special-Chars: a=1; b=2; c=\"hello:world\"\r\n"
    "Valid-Header-With-Tab:\tTabbed-value\r\n"
    "Bad-Header : value-with-space-before-colon\r\n" 
    "\r\n"
    "This is the request body."
)

responses = []
threads = []

def blast_server(thread_id):
    try:
        # Create a raw TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(5.0) # 5 second timeout so it doesn't hang
            s.connect((HOST, PORT))
            
            # Send the raw string encoded as bytes
            s.sendall(RAW_HTTP_REQUEST.encode('utf-8'))
            
            # Wait for the C++ server to respond
            response = s.recv(4096)
            
            if response:
                print(f"[+] Thread {thread_id}: Server responded with {len(response)} bytes.")
            else:
                print(f"[-] Thread {thread_id}: Server closed connection without responding.")
                
    except ConnectionRefusedError:
        print(f"[-] Thread {thread_id}: Connection Refused. Is your C++ server running on port {PORT}?")
    except socket.timeout:
        print(f"[-] Thread {thread_id}: Server timed out (it might be hanging/stuck in an infinite loop).")
    except Exception as e:
        print(f"[-] Thread {thread_id}: Error: {e}")

def main():
    req_count = 1
    if len(sys.argv) > 1: 
        req_count = int(sys.argv[1])
        
    print(f"[*] Blasting {req_count} raw requests to {HOST}:{PORT}...")
    
    for i in range(req_count): 
        threads.append(threading.Thread(target=blast_server, args=(i,)))
        
    for t in threads: t.start()
    for t in threads: t.join()


if __name__ == "__main__":
    main()