#!/bin/bash

die() { echo "Error: $@" 1>&2; exit 1; }

PIDS=$(pgrep -f qemu-system-riscv64)

if [ -n "$PIDS" ]; then
    echo "Terminating the existing process..."

    pkill -f qemu-system-riscv64 || die "Failed to terminate QEMU processes."

    for PID in $PIDS; do
        if command -v pidwait >/dev/null 2>&1; then
            pidwait $PID 2>/dev/null || wait $PID 2>/dev/null
        else
            wait $PID 2>/dev/null
        fi
    done

    echo "All QEMU processes terminated successfully."
else
    echo "No running QEMU processes found."
fi
