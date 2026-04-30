#!/bin/bash
set -eo pipefail

COL_RED="\e[31;1m"
COL_GREEN="\e[32;1m"
COL_LBLUE="\e[96;1m"
COL_RESET="\e[0m"

fail=0

main() {
    build_worker
    run_integration_tests
    show_results
}

build_worker() {
    echo -e "${COL_LBLUE}Building worker...${COL_RESET}"
    cd worker && bazel build //:worker && cd ..
}

run_integration_tests() {
    echo -e "${COL_LBLUE}Running integration tests...${COL_RESET}"
    cd controller
    
    if ./test/run.sh; then
        echo -e "${COL_GREEN}Integration tests passed${COL_RESET}"
    else
        echo -e "${COL_RED}Integration tests failed${COL_RESET}"
        fail=1
    fi
    
    cd ..
}

show_results() {
    if [[ $fail -eq 0 ]]; then
        echo -e "\n${COL_GREEN}ALL TESTS PASSED${COL_RESET}"
    else
        echo -e "\n${COL_RED}SOME TESTS FAILED${COL_RESET}"
    fi
}

cleanup() {
    echo -e "${COL_LBLUE}Cleaning up...${COL_RESET}"
    cd controller && bazel clean 2>/dev/null || true
    cd ../worker && bazel clean 2>/dev/null || true
}

trap cleanup EXIT
main
exit $fail
