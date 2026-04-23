#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <limits>

#include "xuanjing-eval/eval_api.h"

namespace xuanjing {
namespace eval {

// ---------------------------------------------------------------------------
// PSNR
// ---------------------------------------------------------------------------
double ComputePsnr(const ImageView& reference, const ImageView& result) {
  if (reference.width != result.width || reference.height != result.height) {
    return -1.0;
  }
  if (reference.data == nullptr || result.data == nullptr) {
    return -1.0;
  }

  const std::uint32_t w = reference.width;
  const std::uint32_t h = reference.height;
  double mse = 0.0;
  std::uint64_t count = 0;

  for (std::uint32_t y = 0; y < h; ++y) {
    const auto* ref =
        static_cast<const std::uint8_t*>(reference.data) + y * reference.rowStrideBytes;
    const auto* res = static_cast<const std::uint8_t*>(result.data) + y * result.rowStrideBytes;
    // Only accumulate RGB channels (skip alpha)
    for (std::uint32_t x = 0; x < w; ++x) {
      for (int c = 0; c < 3; ++c) {
        const double diff =
            static_cast<double>(ref[x * 4 + c]) - static_cast<double>(res[x * 4 + c]);
        mse += diff * diff;
        ++count;
      }
    }
  }

  if (count == 0 || mse == 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  mse /= static_cast<double>(count);
  return 10.0 * std::log10(255.0 * 255.0 / mse);
}

// ---------------------------------------------------------------------------
// Accumulation helpers
// ---------------------------------------------------------------------------
void AccumulateMetrics(BenchmarkReport& report, const FrameMetrics& frame) {
  if (report.frameCount == 0) {
    report.minPsnr = frame.psnr;
    report.maxPsnr = frame.psnr;
  } else {
    report.minPsnr = std::min(report.minPsnr, frame.psnr);
    report.maxPsnr = std::max(report.maxPsnr, frame.psnr);
  }
  report.meanPsnr += frame.psnr;
  report.meanGpuTimeMs += frame.gpuTimeMs;
  report.meanCpuTimeMs += frame.cpuTimeMs;
  report.algorithmName = frame.algorithmName;
  ++report.frameCount;
}

void FinalizeReport(BenchmarkReport& report) {
  if (report.frameCount == 0) return;
  const double n = static_cast<double>(report.frameCount);
  report.meanPsnr /= n;
  report.meanGpuTimeMs /= n;
  report.meanCpuTimeMs /= n;
}

// ---------------------------------------------------------------------------
// JSON output
// ---------------------------------------------------------------------------
bool WriteReportJson(const BenchmarkReport& r, const std::string& path) {
  std::ofstream f(path);
  if (!f) return false;
  f << "{\n"
    << "  \"algorithm\": \"" << (r.algorithmName ? r.algorithmName : "") << "\",\n"
    << "  \"frame_count\": " << r.frameCount << ",\n"
    << "  \"mean_psnr_db\": " << r.meanPsnr << ",\n"
    << "  \"min_psnr_db\": " << r.minPsnr << ",\n"
    << "  \"max_psnr_db\": " << r.maxPsnr << ",\n"
    << "  \"mean_gpu_time_ms\": " << r.meanGpuTimeMs << ",\n"
    << "  \"mean_cpu_time_ms\": " << r.meanCpuTimeMs << "\n"
    << "}\n";
  return true;
}

// ---------------------------------------------------------------------------
// Comparison print
// ---------------------------------------------------------------------------
void PrintComparison(const BenchmarkReport& baseline, const BenchmarkReport& candidate) {
  const double psnrDelta = candidate.meanPsnr - baseline.meanPsnr;
  const double speedup =
      (baseline.meanCpuTimeMs > 0.0) ? baseline.meanCpuTimeMs / candidate.meanCpuTimeMs : 0.0;

  std::printf("%-20s  PSNR %6.2f dB  cpu %6.2f ms\n",
              baseline.algorithmName ? baseline.algorithmName : "baseline", baseline.meanPsnr,
              baseline.meanCpuTimeMs);
  std::printf("%-20s  PSNR %6.2f dB  cpu %6.2f ms\n",
              candidate.algorithmName ? candidate.algorithmName : "candidate", candidate.meanPsnr,
              candidate.meanCpuTimeMs);
  std::printf("delta:  PSNR %+.2f dB  speedup %.2fx\n", psnrDelta, speedup);
}

}  // namespace eval
}  // namespace xuanjing
