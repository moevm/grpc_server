#!/bin/bash
set -e

COL_RED="\e[31;1m"
COL_GREEN="\e[32;1m"
COL_RESET="\e[0m"


mkdir -p /run/controller
docker compose build

cd controller
go build cmd/manager/test.go
set +e
timeout 20 ./test &
manager_pid=$!
cd ..

sleep 2

# Run 2 workers
docker run -d -v /run/controller:/run/controller -e "METRICS_GATEWAY_ADDRESS=metrics" -e "METRICS_GATEWAY_PORT=9091" -e "METRICS_WORKER_NAME=worker1" --name worker1 grpc_server-worker:latest
docker run -d -v /run/controller:/run/controller -e "METRICS_GATEWAY_ADDRESS=metrics" -e "METRICS_GATEWAY_PORT=9091" -e "METRICS_WORKER_NAME=worker2" --name worker2 grpc_server-worker:latest

wait $manager_pid
manager_exit_code=$?
fail=

if [ ! -z "$(docker logs worker1 | grep -E '\[error\]' )" ]; then
    echo -e "${COL_RED}* worker1: bad logs${COL_RESET}"
    fail=1
else
    echo -e "${COL_GREEN}* worker1: OK${COL_RESET}"
fi

if [ ! -z "$(docker logs worker2 | grep -E '\[error\]' )" ]; then
    echo -e "${COL_RED}* worker2: bad logs${COL_RESET}"
    fail=1
else
    echo -e "${COL_GREEN}* worker2: OK${COL_RESET}"
fi

if [ ${manager_exit_code} -ne 0 ]; then
    echo -e "${COL_RED}* manager: bad exit code${COL_RESET}"
    fail=1
else
    echo -e "${COL_GREEN}* manager: OK${COL_RESET}"
fi

if [ ! -z ${fail} ]; then
    docker ps
    echo -e "worker1:"
    docker logs worker1

    echo -e "\nworker2:"
    docker logs worker2
fi

rm -f  controller/test
docker container rm -f worker1 worker2 > /dev/null
exit ${fail}
