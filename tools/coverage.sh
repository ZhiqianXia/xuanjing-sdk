#!/usr/bin/env bash
# coverage.sh — Build with coverage instrumentation and generate an HTML report.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build/coverage"

cmake --preset coverage -S "${REPO_ROOT}" -B "${BUILD_DIR}"
cmake --build "${BUILD_DIR}" -j

ctest --test-dir "${BUILD_DIR}" --output-on-failure

lcov --capture \
     --directory "${BUILD_DIR}" \
     --output-file "${BUILD_DIR}/coverage.info" \
     --exclude "*_stub.cpp" \
     --exclude "*/tests/*"

genhtml "${BUILD_DIR}/coverage.info" \
        --output-directory "${BUILD_DIR}/coverage-report"

echo "Report: ${BUILD_DIR}/coverage-report/index.html"
