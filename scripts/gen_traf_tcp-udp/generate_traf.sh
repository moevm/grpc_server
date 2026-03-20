#!/bin/bash

count=$1
site=$2

if [ -z "$count" ] || [ -z "$site" ]; then
    echo "Error: the argument was not passed!"
    echo "Usage: ./generate_traf.sh <quantity> <site>"
    echo "Example: ./generate_traf.sh 8 google.com"
    exit 1
fi

if ! [[ "$count" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: '$count' is not a positive number!"
    echo "Usage: ./generate_traf.sh <quantity> <site>"
    echo "Example: ./generate_traf.sh 8 google.com"
    exit 1
fi

for (( i=1; i<=$count;i++ )); do
    nc -zv $site 80
    nc -uzv $site 80
done
