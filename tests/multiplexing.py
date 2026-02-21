#!/bin/python3
from requests import get, post, delete
from threading import Thread
from sys import argv

url = "http://localhost:8080"
req_count = 10
responses = []
threads = []

def crawl():
    response = get(url)
    responses.append(response)
    __import__('pprint').pprint(response)

def main():
    if len(argv) > 1: req_count = int(argv[1])
    for i in range(req_count): threads.append(Thread(target=crawl))
    for t in threads: t.start()
    for t in threads: t.join()
    # TODO: Check if they r all ok.
    for i, r in enumerate(responses): print("req: ", r)
main()
