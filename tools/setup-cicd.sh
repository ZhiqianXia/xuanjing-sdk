#!/bin/bash
# setup-cicd.sh - Quick setup script for local CI/CD testing

set -e

echo "🚀 Xuanjing SDK - Local CI/CD Setup"
echo "===================================="
echo ""

# Check if running on Linux/macOS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    echo "📦 Installing build dependencies..."
    sudo apt-get update -y
    sudo apt-get install -y \
        ninja-build \
        cmake \
        clang \
        clang-tidy \
        clang-format \
        lcov \
        gcc \
        g++ \
        git
    echo "✅ Dependencies installed"
elif [[ "$OSTYPE" == "darwin"* ]]; then
    echo "📦 Installing build dependencies with Homebrew..."
    brew install \
        ninja \
        cmake \
        llvm \
        lcov
    echo "✅ Dependencies installed"
else
    echo "❌ Unsupported OS: $OSTYPE"
    exit 1
fi

echo ""
echo "🔧 Available local test commands:"
echo ""
echo "1. Quick lint check:"
echo "   bash tools/lint.sh"
echo ""
echo "2. Development build + test:"
echo "   cmake --preset dev"
echo "   cmake --build --preset dev -j"
echo "   ctest --preset dev --output-on-failure"
echo ""
echo "3. Memory safety check (ASan/UBSan):"
echo "   cmake --preset asan"
echo "   cmake --build --preset asan -j"
echo "   ctest --preset asan --output-on-failure"
echo ""
echo "4. Release build:"
echo "   cmake --preset release"
echo "   cmake --build --preset release -j"
echo ""
echo "5. Code coverage report:"
echo "   bash tools/coverage.sh"
echo "   # Report at: build-coverage/coverage-report/index.html"
echo ""
echo "✅ Setup complete! You can now run CI/CD checks locally."
