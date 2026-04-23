#pragma once

#include "xuanjing-runtime/runtime_api.h"

#include <cstdint>
#include <string>

namespace xuanjing {
namespace eval {

using runtime::ImageView;

// Metric result for one frame.
struct FrameMetrics {
  double psnr = 0.0;          // dB; higher is better
  double ssim = 0.0;          // [0,1]; stub = 0 until implemented
  double lpips = 0.0;         // stub = 0 until implemented
  double gpuTimeMs = 0.0;     // GPU dispatch time
  double cpuTimeMs = 0.0;     // CPU round-trip time
  std::uint64_t frameIndex = 0;
  const char* algorithmName = nullptr;
};

// Accumulated results across multiple frames.
struct BenchmarkReport {
  double meanPsnr = 0.0;
  double minPsnr = 0.0;
  double maxPsnr = 0.0;
  double meanGpuTimeMs = 0.0;
  double meanCpuTimeMs = 0.0;
  std::uint32_t frameCount = 0;
  const char* algorithmName = nullptr;
};

// Compute PSNR between two same-size RGBA8 ImageViews.
// Returns negative value on error.
double ComputePsnr(const ImageView& reference, const ImageView& result);

// Accumulate a frame metric into report.
void AccumulateMetrics(BenchmarkReport& report, const FrameMetrics& frame);

// Finalise averages. Call once after all frames.
void FinalizeReport(BenchmarkReport& report);

// Write report as JSON to the given file path.
bool WriteReportJson(const BenchmarkReport& report, const std::string& path);

// Print a compact comparison of two reports to stdout.
void PrintComparison(const BenchmarkReport& baseline,
                     const BenchmarkReport& candidate);

}  // namespace eval
}  // namespace xuanjing
