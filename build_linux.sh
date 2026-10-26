#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
cmake -S . -B build-local -DCMAKE_BUILD_TYPE=Release
cmake --build build-local --parallel 4
ctest --test-dir build-local --output-on-failure
exec ./build-local/bin/Lumenfall
