#!/usr/bin/env sh

set -eu

SOURCE_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=${BUILD_DIR:-"${SOURCE_DIR}/build"}

detect_build_jobs() {
    if command -v nproc >/dev/null 2>&1; then
        detected_jobs=$(nproc 2>/dev/null) && {
            echo "${detected_jobs}"
            return
        }
    fi

    if command -v getconf >/dev/null 2>&1; then
        detected_jobs=$(getconf _NPROCESSORS_ONLN 2>/dev/null) && {
            echo "${detected_jobs}"
            return
        }
    fi

    if command -v sysctl >/dev/null 2>&1; then
        detected_jobs=$(sysctl -n hw.ncpu 2>/dev/null) && {
            echo "${detected_jobs}"
            return
        }
    fi

    echo 1
}

BUILD_JOBS=${BUILD_JOBS:-$(detect_build_jobs)}
case "${BUILD_JOBS}" in
    ''|*[!0-9]*|0)
        BUILD_JOBS=1
        ;;
esac

cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" -Dvalijson_BUILD_TESTS=1
make -C "${BUILD_DIR}" -j "${BUILD_JOBS}"

(cd "${BUILD_DIR}" && ./test_suite)
