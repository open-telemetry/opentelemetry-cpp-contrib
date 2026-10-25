#!/bin/bash
TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
HTTPD_DIR="$(cd "${TESTS_DIR}/.." && pwd)"

echo "==> Building OpenTelemetry module with CMake"
cd "${HTTPD_DIR}"

rm -rf build

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

cmake --build build --parallel "$(nproc)"

echo "==> Creating test load"
./create-otel-load.sh

echo "==> Installing module"
sudo ./httpd_install_otel.sh

echo "==> Running tests"
cd "${TESTS_DIR}"

ERR=0

for testfile in ??-*.sh; do
    sudo "./${testfile}" || ERR=1
done

exit "${ERR}"
