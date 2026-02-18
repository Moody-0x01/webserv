#!/bin/python3
from requests import get, post, delete

url = "http://localhost:8080"

def main():
    response = get(url)
    __import__('pprint').pprint(response)
main()
