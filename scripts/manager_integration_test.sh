#!/bin/bash
set -eo pipefail

COL_RED="\e[31;1m"
COL_GREEN="\e[32;1m"
COL_LBLUE="\e[96;1m"
COL_RESET="\e[0m"

CONTROLLER_LOGFILE="controller.log"
WORKER_NAMES=("worker1" "worker2")
TIMEOUT_DURATION=20
INIT_DURATION=2

always_log="$1"
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
    echo -e "${COL_LBLUE}Setting up socket directory...${COL_RESET}"
    sudo mkdir -p /run/controller/
    sudo chown ${user}:${group} /run/controller/
}

build() {
    echo -e "${COL_LBLUE}Building worker...${COL_RESET}"
    docker compose build
    
    echo -e "${COL_LBLUE}Building manager...${COL_RESET}" 
    cd controller && bazel build //cmd/manager:manager && cd ..
}

run_manager() {
    echo -e "${COL_LBLUE}Running manager...${COL_RESET}"
    cd controller
    timeout ${TIMEOUT_DURATION} bazel run //cmd/manager:manager &> ${CONTROLLER_LOGFILE} &
    manager_pid=$!

    echo -e "${COL_LBLUE}Waiting ${INIT_DURATION} seconds for manager to initialize...${COL_RESET}"
    sleep ${INIT_DURATION}
}

run_worker() {
    local worker=$1
    echo -e "${COL_LBLUE}Starting ${worker}...${COL_RESET}"
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
    echo -e "${COL_LBLUE}Waining for manager to finish...${COL_RESET}"
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
    [[ -z ${always_log} ]] && [[ -z ${fail} ]] && return
    
    echo -e  "\n${COL_RED}=== TEST FAILED ===${COL_RESET}"
    docker ps -a
    for worker in "${WORKER_NAMES[@]}"; do
        echo -e "\nLogs for ${worker}:"
        docker logs "${worker}"
    done

    echo -e "\nLogs for manager:"
    cat ${CONTROLLER_LOGFILE}

    echo -e "======================="
}

cleanup() {
    echo -e "${COL_LBLUE}Cleaning up...${COL_RESET}"
    sudo rm -rf /run/controller
    docker container rm -f worker1 worker2 > /dev/null
    rm -f ${CONTROLLER_LOGFILE} bazel-*
}

trap cleanup EXIT
main
exit "${fail:-0}"
