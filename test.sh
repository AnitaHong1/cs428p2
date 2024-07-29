#!/bin/bash

count=0
while [ "$count" -lt 10 ]; do
    curl -sS --connect-timeout 0.5 -o "output_$count.pdf" http://127.0.0.2:29000/notpresent.pdf
    if [ $? -ne 0 ]; then
        echo "curl command failed for output_$count.pdf"
    else
        echo "Downloaded output_$count.pdf successfully"
    fi
    count=$((count + 1))
done