#!/bin/bash

set -e  # stop on first error
set -u  # stop when tries to use uninitialized variable


# parameters: url, file_with_data, login, password
# print response
# exit script in case of error
function putData
{
    local URL=${1}
    local FILE=${2}
    local LOGIN=${3}
    local PASSWORD=${4}
    # calculate token & full URL
    local IDENTITY=`echo -n "${LOGIN}${PASSWORD}" | sha1sum | cut -d \  -f 1`
    local TIME=`date +%s | tr -d "\n"`
    local TOKEN=`echo -n "${URL}${IDENTITY}${TIME}" | sha1sum | cut -d \  -f 1`
    local REQUEST="${URL}&gbLogin=${LOGIN}&gbTime=${TIME}&gbToken=${TOKEN}"
    # send request & parse response
    curl -X PUT "${REQUEST}" --data-binary "@${FILE}"
}


# parameters: url, file_with_data
# print response
# exit script in case of error
function postData
{
    local URL=${1}
    local FILE=${2}
    # send request & parse response
    curl -X POST "${URL}" --data-binary "@${FILE}"
}



if [ "$#" -eq 2 ]; then
    postData "$@"
elif [ "$#" -eq 4 ]; then
    putData "$@"
else
    echo "Incorrect parameters!"
    echo "This program sends POST or PUT request with payload and print the response."
    echo "Parameters for POST request:  URL  file_with_payload"
    echo "Parameters for PUT  request:  URL  file_with_payload  login  password"
    exit 1
fi

