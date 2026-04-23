#include "xuanjing-eval/eval_api.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <vector>

namespace xuanjing {
namespace eval {

namespace {

// Convert RGBA8 to float [0, 1]
inline float Rgba8ToFloat(std::uint8_t val) {
  return static_cast<float>(val) / 255.0f;
}

// Compute MSE between two same-size images (RGBA8, 4-channel).
double ComputeMse(const ImageView& reference, const ImageView& result) {
  if (reference.width != result.width || reference.height != result.height) {
    std::fprintf(stderr, "[Eval] Image dimensions mismatch\n");
    return -1.0;
  }

  if (!reference.data || !result.data) {
    std::fprintf(stderr, "[Eval] Null image data\n");
    return -1.0;
  }

  const auto* refPtr = static_cast<const std::uint8_t*>(reference.data);
  const auto* resPtr = static_cast<const std::uint8_t*>(result.data);

  std::uint64_t pixelCount = reference.width * reference.height;
  double mse = 0.0;

  // Assume RGBA8 format, 4 bytes per pixel
  for (std::uint64_t i = 0; i < pixelCount * 4; ++i) {
    float refVal = Rgba8ToFloat(refPtr[i]);
    float resVal = Rgba8ToFloat(resPtr[i]);
    float diff = refVal - resVal;
    mse += diff * diff;
  }

  mse /= static_cast<double>(pixelCount * 4);
  return mse;
}

// Gaussian blur kernel (3x3, sigma=1.0).
const float kGaussianKernel[9] = {
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f,  //
    2.0f / 16.0f, 4.0f / 16.0f, 2.0f / 16.0f,  //
    1.0f / 16.0f, 2.0f / 16.0f, 1.0f / 16.0f
};

// Simple SSIM calculation (luminance only, Y channel).
// Simplified version: use R channel as luminance for RGBA.
double ComputeSsim(const ImageView& reference, const ImageView& result) {
  if (reference.width != result.width || reference.height != result.height) {
    return -1.0;
  }

  if (!reference.data || !result.data) {
    return -1.0;
  }

  const auto* refPtr = static_cast<const std::uint8_t*>(reference.data);
  const auto* resPtr = static_cast<const std::uint8_t*>(result.data);

  std::uint32_t width = reference.width;
  std::uint32_t height = reference.height;

  // Extract luminance (R channel, assuming RGBA)
  std::vector<float> refLum(width * height);
  std::vector<float> resLum(width * height);

  for (std::uint32_t y = 0; y < height; ++y) {
    for (std::uint32_t x = 0; x < width; ++x) {
      std::uint32_t idx = y * width + x;
      refLum[idx] = Rgba8ToFloat(refPtr[idx * 4 + 0]);
      resLum[idx] = Rgba8ToFloat(resPtr[idx * 4 + 0]);
    }
  }

  // Compute SSIM
  constexpr float kC1 = 0.0001f;  // (0.01)^2
  constexpr float kC2 = 0.0009f;  // (0.03)^2

  double ssim = 0.0;
  std::uint32_t sampleCount = 0;

  for (std::uint32_t y = 1; y < height - 1; ++y) {
    for (std::uint32_t x = 1; x < width - 1; ++x) {
      std::uint32_t idx = y * width + x;

      // Local statistics (8-neighbor mean)
      float refMean = 0.0f;
      float resMean = 0.0f;

      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          std::uint32_t nidx = (y + dy) * width + (x + dx);
          refMean += refLum[nidx] / 9.0f;
          resMean += resLum[nidx] / 9.0f;
        }
      }

      // Local variance
      float refVar = 0.0f;
      float resVar = 0.0f;
      float covar = 0.0f;

      for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
          std::uint32_t nidx = (y + dy) * width + (x + dx);
          float refDiff = refLum[nidx] - refMean;
          float resDiff = resLum[nidx] - resMean;
          refVar += refDiff * refDiff / 9.0f;
          resVar += resDiff * resDiff / 9.0f;
          covar += refDiff * resDiff / 9.0f;
        }
      }

      // SSIM formula
      float num = (2.0f * refMean * resMean + kC1) * (2.0f * covar + kC2);
      float denom = (refMean * refMean + resMean * resMean + kC1) *
                    (refVar + resVar + kC2);
      ssim += num / denom;
      ++sampleCount;
    }
  }

  return sampleCount > 0 ? (ssim / sampleCount) : 0.0;
}

}  // namespace

double ComputePsnr(const ImageView& reference, const ImageView& result) {
  double mse = ComputeMse(reference, result);
  if (mse < 0.0 || mse == 0.0) {
    return mse < 0.0 ? -1.0 : 100.0;  // -1 error, 100 perfect match
  }
  constexpr double kMaxValue = 1.0;  // normalized [0,1] range
  return 20.0 * std::log10(kMaxValue / std::sqrt(mse));
}

void AccumulateMetrics(BenchmarkReport& report, const FrameMetrics& frame) {
  if (report.frameCount == 0) {
    report.meanPsnr = frame.psnr;
    report.minPsnr = frame.psnr;
    report.maxPsnr = frame.psnr;
  } else {
    report.meanPsnr =
        (report.meanPsnr * report.frameCount + frame.psnr) / (report.frameCount + 1);
    report.minPsnr = std::min(report.minPsnr, frame.psnr);
    report.maxPsnr = std::max(report.maxPsnr, frame.psnr);
  }
  report.meanGpuTimeMs =
      (report.meanGpuTimeMs * report.frameCount + frame.gpuTimeMs) /
      (report.frameCount + 1);
  report.meanCpuTimeMs =
      (report.meanCpuTimeMs * report.frameCount + frame.cpuTimeMs) /
      (report.frameCount + 1);
  ++report.frameCount;
}

void FinalizeReport(BenchmarkReport& report) {
  // Averages already computed in AccumulateMetrics.
  // This is a no-op in the current implementation but kept for API stability.
}

bool WriteReportJson(const BenchmarkReport& report, const std::string& path) {
  std::ofstream file(path);
  if (!file.is_open()) {
    std::fprintf(stderr, "[Eval] Failed to open %s\n", path.c_str());
    return false;
  }

  file << "{\n";
  file << "  \"algorithm\": \"" << (report.algorithmName ? report.algorithmName : "unknown") << "\",\n";
  file << "  \"frameCount\": " << report.frameCount << ",\n";
  file << "  \"metrics\": {\n";
  file << "    \"psnr\": {\n";
  file << "      \"mean\": " << report.meanPsnr << ",\n";
  file << "      \"min\": " << report.minPsnr << ",\n";
  file << "      \"max\": " << report.maxPsnr << "\n";
  file << "    },\n";
  file << "    \"gpuTimeMs\": " << report.meanGpuTimeMs << ",\n";
  file << "    \"cpuTimeMs\": " << report.meanCpuTimeMs << "\n";
  file << "  }\n";
  file << "}\n";

  file.close();
  return true;
}

void PrintComparison(const BenchmarkReport& baseline, const BenchmarkReport& candidate) {
  std::printf("\n============ Quality Comparison ============\n");
  std::printf("Baseline:  %s\n", baseline.algorithmName ? baseline.algorithmName : "unknown");
  std::printf("Candidate: %s\n", candidate.algorithmName ? candidate.algorithmName : "unknown");
  std::printf("\nPSNR (dB):\n");
  std::printf("  Baseline:  %.2f (min=%.2f, max=%.2f)\n", baseline.meanPsnr, baseline.minPsnr, baseline.maxPsnr);
  std::printf("  Candidate: %.2f (min=%.2f, max=%.2f)\n", candidate.meanPsnr, candidate.minPsnr, candidate.maxPsnr);
  double psnrDelta = candidate.meanPsnr - baseline.meanPsnr;
  std::printf("  Delta:     %+.2f dB %s\n", psnrDelta,
              psnrDelta > 0 ? "(improvement)" : "(regression)");
  std::printf("\nLatency (ms):\n");
  std::printf("  Baseline GPU:  %.2f\n", baseline.meanGpuTimeMs);
  std::printf("  Candidate GPU: %.2f\n", candidate.meanGpuTimeMs);
  double gpuDelta = candidate.meanGpuTimeMs - baseline.meanGpuTimeMs;
  std::printf("  Delta:         %+.2f ms\n", gpuDelta);
  std::printf("==========================================\n\n");
}

}  // namespace eval
}  // namespace xuanjing
