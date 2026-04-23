#!/usr/bin/env bash
# lint.sh — Run clang-format check and clang-tidy across the project.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${REPO_ROOT}/build/dev"

echo "=== clang-format check ==="
find "${REPO_ROOT}/modules" -name "*.h" -o -name "*.cpp" | \
  xargs clang-format --dry-run --Werror

echo "=== clang-tidy ==="
if command -v clang-tidy &> /dev/null; then
  if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
    echo "No compile_commands.json found. Configure first: cmake --preset dev"
    exit 1
  fi
  find "${REPO_ROOT}/modules" -name "*.cpp" | \
    xargs clang-tidy -p "${BUILD_DIR}"
else
  echo "Warning: clang-tidy not found. Skipping static analysis checks."
  echo "To enable: sudo apt-get install clang-tools"
fi

echo "=== Lint passed ==="
