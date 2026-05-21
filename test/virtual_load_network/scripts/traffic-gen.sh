#!/bin/bash

URLS=(
    "http://www.google.com"
    "https://youtube.com"
    "https://www.tiktok.com"
    "https://1xbet.com/"
    "https://betboom.ru/"
)

while true; do
    url=${URLS[$RANDOM % ${#URLS[@]}]}
    echo "[$(date '+%H:%M:%S')] -> request $url"
    #status=$(curl -s -o /dev/null -w "%{http_code}" --max-time 5 "$url")
    echo "[$(date '+%H:%M:%S')] <- response $url, status = $status"
    sleep 5
done
