#!/bin/bash

find worker -name '*.cpp' -o -name '*.hpp' -o -name '*.h' -o -name '*.c' | xargs clang-format --dry-run --Werror