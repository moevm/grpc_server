#!/bin/bash

find src -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' | xargs clang-format --dry-run --Werror