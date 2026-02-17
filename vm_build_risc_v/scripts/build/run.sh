#!/bin/bash

set -e

if [ -d "/home/builder/qemu/poky" ]; then
  cd /home/builder/qemu/poky
else
  echo "error: /home/builder/qemu/poky/ doesn't exist"
  exit 1
fi

source oe-init-build-env

if [ -d "tmp/deploy/images/qemuriscv64/" ]; then
  cd tmp/deploy/images
  runqemu qemuriscv64 slirp nographic
else
  echo "error: Image doesn't exist. Please run \"make build\" before executing this command"
fi
