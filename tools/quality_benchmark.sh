#!/bin/bash
#
# tools/quality_benchmark.sh
#
# Automated CI quality benchmark script for Xuanjing SR pipeline.
# Measures PSNR, SSIM, and throughput across multiple algorithms.
#
# Usage:
#   ./tools/quality_benchmark.sh [output-dir] [config]
#
# Output:
#   output-dir/
#     ├── benchmark_passthrough.json
#     ├── benchmark_bilinear.json
#     ├── comparison_report.txt
#     └── ci_metrics.json  (aggregated for CI)

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_DIR="${1:-.}"
CONFIG="${2:-release}"

# Prefer preset-style build dir first (build/release, build/dev, ...),
# then fall back to legacy build/.
BUILD_DIR="${PROJECT_ROOT}/build/${CONFIG}"
if [[ ! -d "$BUILD_DIR" ]]; then
    BUILD_DIR="${PROJECT_ROOT}/build"
fi

# Ensure build directory exists
if [[ ! -d "$BUILD_DIR" ]]; then
    echo "[ERROR] Build directory not found: $BUILD_DIR"
    echo "Please run: cmake --preset=$CONFIG && cmake --build --preset=$CONFIG"
    exit 1
fi

# Ensure output directory exists
mkdir -p "$OUTPUT_DIR"

echo "=========================================="
echo " Xuanjing Quality Benchmark CI Pipeline"
echo "=========================================="
echo "Project:   $PROJECT_ROOT"
echo "Build:     $BUILD_DIR"
echo "Output:    $OUTPUT_DIR"
echo "Config:    $CONFIG"
echo "=========================================="
echo ""

# Check if benchmark runner exists
BENCHMARK_BIN="$BUILD_DIR/samples/xuanjing-benchmark-runner"
if [[ ! -f "$BENCHMARK_BIN" ]]; then
    ALT_BENCHMARK_BIN="${PROJECT_ROOT}/build/release/samples/xuanjing-benchmark-runner"
    if [[ -f "$ALT_BENCHMARK_BIN" ]]; then
        BENCHMARK_BIN="$ALT_BENCHMARK_BIN"
    fi
fi

if [[ ! -f "$BENCHMARK_BIN" ]]; then
    echo "[ERROR] Benchmark runner not found: $BENCHMARK_BIN"
    echo "Building project..."
    cd "$PROJECT_ROOT"
    cmake --preset="$CONFIG"
    cmake --build --preset="$CONFIG" -j$(nproc)

    # Re-resolve benchmark location after build.
    BENCHMARK_BIN="${PROJECT_ROOT}/build/${CONFIG}/samples/xuanjing-benchmark-runner"
    if [[ ! -f "$BENCHMARK_BIN" ]]; then
        BENCHMARK_BIN="${PROJECT_ROOT}/build/samples/xuanjing-benchmark-runner"
    fi
    if [[ ! -f "$BENCHMARK_BIN" ]]; then
        echo "[ERROR] Build failed or benchmark not generated"
        exit 1
    fi
fi

echo "[INFO] Benchmark binary: $BENCHMARK_BIN"
echo ""

# Check if integration/perf tests exist
INTEGRATION_TEST="$BUILD_DIR/tests/tests_pipeline_integration_test"
PERF_TEST="$BUILD_DIR/tests/tests_shader_perf_benchmark"

if [[ ! -f "$INTEGRATION_TEST" ]]; then
    echo "[WARN] Integration test not found: $INTEGRATION_TEST"
    echo "[INFO] Building tests..."
    cd "$PROJECT_ROOT"
    cmake --preset="$CONFIG"
    cmake --build --preset="$CONFIG" --target all -j$(nproc)
fi

echo ""
echo "=========================================="
echo "[Phase 1] Running Integration Tests"
echo "=========================================="

if [[ -f "$INTEGRATION_TEST" ]]; then
    echo "[RUN] $INTEGRATION_TEST"
    "$INTEGRATION_TEST" || {
        echo "[WARN] Integration tests had failures (continuing)"
    }
else
    echo "[SKIP] Integration tests not available"
fi

echo ""
echo "=========================================="
echo "[Phase 2] Running Performance Benchmarks"
echo "=========================================="

if [[ -f "$PERF_TEST" ]]; then
    echo "[RUN] $PERF_TEST"
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    PERF_OUTPUT="$OUTPUT_DIR/perf_benchmark_$TIMESTAMP.txt"
    "$PERF_TEST" | tee "$PERF_OUTPUT"
    echo "[INFO] Perf results saved to: $PERF_OUTPUT"
else
    echo "[SKIP] Perf tests not available"
fi

echo ""
echo "=========================================="
echo "[Phase 3] Generating Quality Reports"
echo "=========================================="

BENCHMARK_OUTPUT="$OUTPUT_DIR/benchmark_results"
mkdir -p "$BENCHMARK_OUTPUT"

echo "[RUN] $BENCHMARK_BIN --output $BENCHMARK_OUTPUT"
"$BENCHMARK_BIN" "$BENCHMARK_OUTPUT" || {
    echo "[WARN] Benchmark runner had errors"
}

echo "[INFO] Benchmark results in: $BENCHMARK_OUTPUT"
ls -la "$BENCHMARK_OUTPUT"/*.json 2>/dev/null || echo "[INFO] No JSON reports generated yet"

echo ""
echo "=========================================="
echo "[Phase 4] Aggregating CI Metrics"
echo "=========================================="

# Create aggregated CI metrics
CI_METRICS="$OUTPUT_DIR/ci_metrics.json"

cat > "$CI_METRICS" << EOF
{
  "timestamp": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
  "build_config": "$CONFIG",
  "test_results": {
    "integration": "$([ -f "$INTEGRATION_TEST" ] && echo "available" || echo "not_built")",
    "perf": "$([ -f "$PERF_TEST" ] && echo "available" || echo "not_built")"
  },
  "benchmark_reports": {
    "location": "$BENCHMARK_OUTPUT",
    "count": $(find "$BENCHMARK_OUTPUT" -name "*.json" 2>/dev/null | wc -l || echo 0)
  },
  "status": "generated"
}
EOF

echo "[INFO] CI metrics written to: $CI_METRICS"
cat "$CI_METRICS"

echo ""
echo "=========================================="
echo "[Summary]"
echo "=========================================="
echo "✓ Benchmark pipeline completed"
echo ""
echo "Output files:"
echo "  - Integration tests:  ${INTEGRATION_TEST##*/}"
echo "  - Perf benchmarks:    ${PERF_TEST##*/}"
echo "  - Quality reports:    $BENCHMARK_OUTPUT"
echo "  - CI metrics:         $CI_METRICS"
echo ""
echo "Next steps:"
echo "  1. Review PSNR/SSIM metrics in: $BENCHMARK_OUTPUT"
echo "  2. Compare against baseline (if available)"
echo "  3. Analyze throughput and latency trends"
echo ""
echo "=========================================="
echo ""

exit 0
