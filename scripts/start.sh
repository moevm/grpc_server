#!/bin/bash
set -e

CPU_COUNT=$(nproc)

docker-compose up -d --scale worker=$CPU_COUNT

for (( i=0; i<$CPU_COUNT; i++ ))
do
  container_id=$(docker-compose ps -a -q worker | sed -n "$((i+1))p")
  if [ -n "$container_id" ]; then
    docker update --cpuset-cpus="$i" "$container_id"
  fi
done

