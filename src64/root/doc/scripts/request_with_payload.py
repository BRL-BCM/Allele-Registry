#!/usr/bin/env python

import requests
import hashlib
import time
import sys


# return response
# throw exceptions in case of errors 
def putData(url, data, login, password):
    # calculate token & full URL
    identity = hashlib.sha1((login + password).encode('utf-8')).hexdigest()
    gbTime = str(int(time.time()))
    token = hashlib.sha1((url + identity + gbTime).encode('utf-8')).hexdigest()
    request = url + '&gbLogin=' + login + '&gbTime=' + gbTime + '&gbToken=' + token
    # send request & parse response
    res = requests.put(request, data=data)
    response = res.text
    # check status
    if res.status_code != 200:
        raise  Exception("Error for PUT requests: " + response)
    return response


# return response
# throw exceptions in case of errors 
def postData(url, data):
    # send request & parse response
    res = requests.post(url, data=data)
    response = res.text
    # check status
    if res.status_code != 200:
        raise  Exception("Error for POST requests: " + response)
    return response


if len(sys.argv) != 3 and len(sys.argv) != 5:
    sys.stderr.write("Incorrect parameters!\n")
    print("This program sends POST or PUT request with payload and print the response.")
    print("Parameters for POST request:  URL  file_with_payload")
    print("Parameters for PUT  request:  URL  file_with_payload  login  password")
    sys.exit()

url      = sys.argv[1]
file     = sys.argv[2]
if len(sys.argv) > 3:
    login    = sys.argv[3]
    password = sys.argv[4]
else:
    login = password = None

data = open(file).read()
if not url.startswith(("http://","https://")):
    url = "http://" + url

if login is None or password is None:
    response = postData(url, data)
else:
    response = putData(url, data, login, password)

print response

