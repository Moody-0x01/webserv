#!/bin/python3
from requests import get, post, delete
from threading import Thread
from sys import argv

url = "http://localhost:8080"
req_count = 10
responses = []
threads = []
# GET /index.html HTTP/1.1
# Host: localhost:8080
# User-Agent: Mozilla/5.0
# Accept: text/html
headers = {
    'User-Agent': 'Mozilla/902.0',
    'Accept': 'text/html'
}

def crawl():
    response = get(url, headers=headers)
    print("REQUEST HEADERS: ", response.request.headers)
    # print("RESPONSE HEADERS: ", response.headers)
    # print("RESPONSE CONTENT: ", response.content)
    responses.append(response)
    __import__('pprint').pprint(response)

def main():
    req_count = 1
    if len(argv) > 1: req_count = int(argv[1])
    for i in range(req_count): threads.append(Thread(target=crawl))
    for t in threads: t.start()
    for t in threads: t.join()
    # TODO: Check if they r all ok.
    for i, r in enumerate(responses): print("req: ", r)
main()
