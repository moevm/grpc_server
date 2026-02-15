#!/bin/bash

PID_FILE="pids"
VENV_DIR="venv"
LOG_FILE="logs.log"
LOG_LEVEL="info"

show_help() {
    cat << EOF
Usage: $(basename "$0") [COMMAND] [OPTIONS]

Commands:
    start       Запустить процессы
    stop        Остановить все процессы

Options:
    --count=N   Количество процессов (по умолчанию: 2)
    --help      Показать эту справку

Examples:
    $(basename "$0") start --count=3
    $(basename "$0") stop
EOF
}

check_dependencies() {

    if [ ! -f "main.py" ]; then
        echo "Не найден файл main.py"
        exit 1
    fi

    if [ ! -f "config.json" ]; then
        echo "Не найден файл config.json"
        exit 1
    fi

    if [ ! -f "requirements.txt" ]; then
        echo "Не найден файл requirements.txt"
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

    if [ -f "$PID_FILE" ]; then
        local running_pids=()
        while read pid; do
            if is_process_running "$pid"; then
                running_pids+=("$pid")
            fi
        done < "$PID_FILE"

        if [ ${#running_pids[@]} -gt 0 ]; then
            echo "Уже есть запущенные клиенты, остновите их командой stop"
            return 1
        else
            > "$PID_FILE"
        fi
    fi

    check_dependencies
    setup_venv

    > "$LOG_FILE"

    for ((i=0;i<count;i++)); do

        python3 main.py --config config.json --log "$LOG_LEVEL" >> "$LOG_FILE" 2>&1 & local pid=$!

        echo "$pid" >> "$PID_FILE"

    done

}

stop_process() {

    if [ ! -f "$PID_FILE" ]; then
        echo "Нет активных процессов"
        return 0
    fi

    while read pid; do
        if is_process_running "$pid" ; then
            kill "$pid" 2>/dev/null
        fi
    done < "$PID_FILE"

    rm "$PID_FILE"

}

COMMAND=""
COUNT=2

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
        *)
            echo "Неизвестная опция: $arg"
            ;;
    esac
done


case "$COMMAND" in
    start)
        start_processes "$COUNT"
        ;;
    stop)
        stop_process
        ;;
    *)
        echo "Неизвестная команда: $COMMAND"
        ;;
esac
