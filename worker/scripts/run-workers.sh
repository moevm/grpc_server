#!/bin/bash
set -e

docker run -d -p 50051:50051 --name worker1 qemu-riscv-cluster/worker
docker run -d -p 50052:50051 --name worker2 qemu-riscv-cluster/worker
docker run -d -p 50053:50051 --name worker3 qemu-riscv-cluster/worker
docker run -d -p 50054:50051 --name worker4 qemu-riscv-cluster/worker
docker run -d -p 50055:50051 --name worker5 qemu-riscv-cluster/worker