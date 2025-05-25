#!/bin/bash
set -eo pipefail

COL_RED="\e[31;1m"
COL_GREEN="\e[32;1m"
COL_RESET="\e[0m"

CONTROLLER_LOGFILE="controller.log"
WORKER_NAMES=("worker1" "worker2")
TIMEOUT_DURATION=20
INIT_DURATION=2

user=$(id -un)
group=$(id -gn)
manager_pid=
fail=

main() {
    setup
    build
    run_manager
    run_workers
    wait_for_manager
    check_results
}

setup() {
    echo -e "Setting up socket directory..."
    sudo mkdir -p /run/controller/
    sudo chown ${user}:${group} /run/controller/
}

build() {
    echo -e "Building worker..."
    docker compose build
    
    echo -e "Building manager..." 
    cd controller && go build cmd/manager/test.go && cd ..
}

run_manager() {
    echo -e "Running manager..."
    timeout ${TIMEOUT_DURATION} controller/test &> ${CONTROLLER_LOGFILE} &
    manager_pid=$!

    echo -e "Waiting ${INIT_DURATION} seconds for manager to initialize..."
    sleep ${INIT_DURATION}
}

run_worker() {
    local worker=$1
    echo -e "Starting ${worker}..."
    docker run \
        --user "$(id -u):$(id -g)" \
        -d \
        -v "/run/controller:/run/controller" \
        -e "METRICS_GATEWAY_ADDRESS=metrics" \
        -e "METRICS_GATEWAY_PORT=9091" \
        -e "METRICS_WORKER_NAME=${worker}" \
        --name "${worker}" \
        "grpc_server-worker"
}

run_workers() {
    for worker in "${WORKER_NAMES[@]}"; do
        run_worker "${worker}"
    done   
}

wait_for_manager() {
    echo -e "Waining for manager to finish..."
    set +e
    wait $manager_pid
    manager_exit_code=$?
    set -e
}

check_results() {
    for worker in "${WORKER_NAMES[@]}"; do
        check_worker "${worker}"
    done
    
    check_manager_exit_code
    show_failure_details
}

check_worker() {
    local worker=$1
    if docker logs "${worker}" | grep -qE '\[error\]'; then
        echo -e "${COL_RED}* ${worker}: Error logs detected${COL_RESET}"
        fail=1
    else
        echo -e "${COL_GREEN}* ${worker}: OK${COL_RESET}"
    fi
}

check_manager_exit_code() {
    if [[ ${manager_exit_code} -ne 0 ]]; then
        echo -e "${COL_RED}* Manager: Bad exit code (${manager_exit_code})${COL_RESET}"
        fail=1
    else
        echo -e "${COL_GREEN}* Manager: OK${COL_RESET}"
    fi
}

show_failure_details() {
    [[ -z ${fail} ]] && return
    
    echo -e  "\n${COL_RED}=== TEST FAILED ==="
    docker ps -a
    for worker in "${WORKER_NAMES[@]}"; do
        echo -e "\nLogs for ${worker}:"
        docker logs "${worker}"
    done

    echo -e "\nLogs for manager:"
    cat ${CONTROLLER_LOGFILE}

    echo -e "=======================${COL_RESET}"
}

cleanup() {
    echo -e "Cleaning up..."
    rm -f controller/test ${CONTROLLER_LOGFILE}
    sudo rm -rf /run/controller
    docker container rm -f worker1 worker2 > /dev/null
}

trap cleanup EXIT
main
exit "${fail:-0}"
