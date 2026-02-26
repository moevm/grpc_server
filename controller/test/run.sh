#!/bin/bash

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

cd "$PROJECT_ROOT/controller"

echo "Сборка теста"
bazel build //test:integration_test

echo "Запуск"
./bazel-bin/test/integration_test_/integration_test
