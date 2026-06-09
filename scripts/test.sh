#!/usr/bin/env sh

set -eu

SOURCE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"${SOURCE_DIR}/build"}

cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -Dvalijson_BUILD_TESTS=1
make -C "${BUILD_DIR}"

(cd "${BUILD_DIR}" && ./test_suite)
