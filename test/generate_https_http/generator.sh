#!/bin/bash

PID_FILE="pids"
VENV_DIR="venv"
LOG_LEVEL="info"

show_help() {
    cat << EOF
Usage: $(basename "$0") [COMMAND] [OPTIONS]

Commands:
    start       start process
    stop        stop all proces

Options:
    --count=N           number of process (default: 2)
    --help              show help
    --max_concurrent     max concurent requests

Examples:
    $(basename "$0") start --count=3 --max_concurrent=4
    $(basename "$0") stop
EOF
}

check_dependencies() {

    if [ ! -f "main.py" ]; then
        echo "Not found file main.py"
        exit 1
    fi

    if [ ! -f "config.json" ]; then
        echo "Not found file config.json"
        exit 1
    fi

    if [ ! -f "requirements.txt" ]; then
        echo "Not found file requirements.txt"
        exit 1
    fi

}

setup_venv() {

    if [ ! -d "$VENV_DIR" ]; then
        python3 -m venv venv
    fi

    source "$VENV_DIR/bin/activate"

    if [ -f "requirements.txt" ]; then
        pip install -r requirements.txt
    fi
}

is_process_running() {
    local pid=$1
    if [ -n "$pid" ] && kill -0 "$pid" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

start_processes() {

    local count=$1
    local max_concurent=$2

    if [ -f "$PID_FILE" ]; then
        local running_pids=()
        while read pid; do
            if is_process_running "$pid"; then
                running_pids+=("$pid")
            fi
        done < "$PID_FILE"

        if [ ${#running_pids[@]} -gt 0 ]; then
            echo "There are already running clients, stop them with the stop command"
            return 1
        else
            > "$PID_FILE"
        fi
    fi

    check_dependencies
    setup_venv

    mkdir -p logs

    for ((i=0;i<count;i++)); do

        python3 main.py --config config.json --log "$LOG_LEVEL" --max_concurent $max_concurent > "logs/logs_${i}.log" 2>&1 & local pid=$!

        echo "$pid" >> "$PID_FILE"

    done

}

stop_process() {

    if [ ! -f "$PID_FILE" ]; then
        echo "No active processes"
        return 0
    fi

    while read pid; do
        if is_process_running "$pid" ; then
            kill -2 "$pid" 2>/dev/null
        fi
    done < "$PID_FILE"

    rm "$PID_FILE"

}

COMMAND=""
COUNT=2
MAX_CONCURENT=5

if [ $# -gt 0 ] && [[ ! "$1" =~ ^-- ]]; then
    COMMAND="$1"
    shift
fi

for arg in "$@"; do
    case $arg in
        --count=*)
            COUNT="${arg#*=}"
            shift
            ;;
        --help)
            show_help
            exit 0
            ;;
        --max_concurent=*)
            MAX_CONCURENT="${arg#*=}"
            shift
            ;;
        *)
            echo "Unknown option: $arg"
            ;;
    esac
done


case "$COMMAND" in
    start)
        start_processes "$COUNT" "$MAX_CONCURENT"
        ;;
    stop)
        stop_process
        ;;
    *)
        echo "Unknown command: $COMMAND"
        ;;
esac
