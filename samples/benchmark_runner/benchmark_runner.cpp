// benchmark_runner.cpp
// Run multiple upscale algorithms on the same static FrameInput and compare
// PSNR / latency. Writes per-algorithm JSON reports.
//
// Usage:
//   ./xuanjing-benchmark-runner [output-dir]
//
// Output:
//   <output-dir>/benchmark_passthrough.json
//   <output-dir>/benchmark_bilinear.json
//   <output-dir>/benchmark_comparison.json

#include "xuanjing-runtime/runtime_api.h"
#include "xuanjing-upscale/upscale_api.h"
#include "xuanjing-genframe/genframe_api.h"
#include "xuanjing-eval/eval_api.h"

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace {

using namespace xuanjing;

struct PipelineStats {
  std::uint32_t generatedFrames = 0;
};

// Build a synthetic RGBA8 gradient image of size w x h.
std::vector<std::uint8_t> MakeGradientRgba8(std::uint32_t w,
                                             std::uint32_t h) {
  std::vector<std::uint8_t> buf(w * h * 4);
  for (std::uint32_t y = 0; y < h; ++y) {
    for (std::uint32_t x = 0; x < w; ++x) {
      std::uint8_t r = static_cast<std::uint8_t>(x * 255 / (w - 1));
      std::uint8_t g = static_cast<std::uint8_t>(y * 255 / (h - 1));
      std::uint8_t b = static_cast<std::uint8_t>(128);
      buf[(y * w + x) * 4 + 0] = r;
      buf[(y * w + x) * 4 + 1] = g;
      buf[(y * w + x) * 4 + 2] = b;
      buf[(y * w + x) * 4 + 3] = 255;
    }
  }
  return buf;
}

runtime::ImageView ViewOf(std::vector<std::uint8_t>& buf,
                           std::uint32_t w, std::uint32_t h) {
  runtime::ImageView v{};
  v.data = buf.data();
  v.width = w;
  v.height = h;
  v.rowStrideBytes = w * 4;
  v.format = runtime::PixelFormat::kRGBA8UNorm;
  v.colorSpace = runtime::ColorSpace::kLinear;
  return v;
}

bool WritePpm(const std::string& path, const runtime::ImageView& img) {
  std::ofstream f(path, std::ios::binary);
  if (!f) return false;
  f << "P6\n" << img.width << " " << img.height << "\n255\n";
  for (std::uint32_t y = 0; y < img.height; ++y) {
    const auto* row =
        static_cast<const std::uint8_t*>(img.data) + y * img.rowStrideBytes;
    for (std::uint32_t x = 0; x < img.width; ++x) {
      f.put(static_cast<char>(row[x * 4 + 0]));
      f.put(static_cast<char>(row[x * 4 + 1]));
      f.put(static_cast<char>(row[x * 4 + 2]));
    }
  }
  return true;
}

// Run an upscaler for kFrames frames, return its BenchmarkReport.
eval::BenchmarkReport RunAlgorithm(upscale::IUpscaler& upscaler,
                                    genframe::IFrameGenerator* frameGenerator,
                                    const runtime::FrameInput& inputTemplate,
                                    const runtime::ImageView& groundTruth,
                                    std::uint32_t kFrames,
                                    const std::string& outDir,
                                    PipelineStats& stats) {
  eval::BenchmarkReport report{};

  std::vector<std::uint8_t> prevHistoryBuffer(
      static_cast<std::size_t>(groundTruth.width) * groundTruth.height * 4U, 0U);
  runtime::ImageView prevHistoryView = ViewOf(prevHistoryBuffer,
                                              groundTruth.width,
                                              groundTruth.height);

  for (std::uint32_t i = 0; i < kFrames; ++i) {
    runtime::FrameInput input = inputTemplate;
    input.metadata.frameIndex = i;
    input.hasPrevHistory = (i > 0);
    if (input.hasPrevHistory) {
      input.prevHistory = prevHistoryView;
    }

    runtime::FrameOutput output{};

    const auto cpuStart = std::chrono::steady_clock::now();
    const bool ok = upscaler.Upscale(input, output);

    if (!ok) {
      std::fprintf(stderr, "[%s] Upscale() failed at frame %u\n",
                   upscaler.Name(), i);
      continue;
    }

    if (frameGenerator != nullptr) {
      const bool genOk = frameGenerator->Generate(input, output);
      if (!genOk) {
        std::fprintf(stderr, "[%s] Frame generation failed at frame %u\n",
                     frameGenerator->Name(), i);
      } else if (output.hasGeneratedFrame) {
        ++stats.generatedFrames;
      }
    }

    const auto cpuEnd = std::chrono::steady_clock::now();

    const double cpuMs =
        std::chrono::duration<double, std::milli>(cpuEnd - cpuStart).count();

    eval::FrameMetrics fm{};
    fm.algorithmName = upscaler.Name();
    fm.frameIndex = i;
    fm.psnr = eval::ComputePsnr(groundTruth, output.highResColor);
    fm.cpuTimeMs = cpuMs;

    eval::AccumulateMetrics(report, fm);

    // Write first frame output as preview PPM
    if (i == 0) {
      const std::string ppmPath =
          outDir + "/benchmark_" + upscaler.Name() + "_frame0.ppm";
      WritePpm(ppmPath, output.highResColor);
    }

    if (output.highResColor.data != nullptr &&
        output.highResColor.width == prevHistoryView.width &&
        output.highResColor.height == prevHistoryView.height) {
      for (std::uint32_t y = 0; y < prevHistoryView.height; ++y) {
        const auto* src = static_cast<const std::uint8_t*>(output.highResColor.data) +
                          y * output.highResColor.rowStrideBytes;
        auto* dst = prevHistoryBuffer.data() +
                    static_cast<std::size_t>(y) * prevHistoryView.rowStrideBytes;
        std::memcpy(dst, src, prevHistoryView.rowStrideBytes);
      }
    }
  }

  eval::FinalizeReport(report);
  return report;
}

}  // namespace

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
  const std::string outDir = (argc > 1) ? argv[1] : ".";

  // ---- Static input (low-res 32x18, upscale to 64x36) -------------------
  constexpr std::uint32_t kSrcW = 32;
  constexpr std::uint32_t kSrcH = 18;
  constexpr std::uint32_t kDstW = 64;
  constexpr std::uint32_t kDstH = 36;
  constexpr std::uint32_t kFrames = 8;

  auto colorBuf = MakeGradientRgba8(kSrcW, kSrcH);
  auto mvBuf    = std::vector<std::uint8_t>(kSrcW * kSrcH * 4, 128);
  auto depthBuf = std::vector<std::uint8_t>(kSrcW * kSrcH * 4, 200);

  // Ground truth: same gradient at output resolution
  auto groundTruthBuf = MakeGradientRgba8(kDstW, kDstH);

  runtime::FrameInput input{};
  input.lowResColor  = ViewOf(colorBuf, kSrcW, kSrcH);
  input.motionVectors = ViewOf(mvBuf, kSrcW, kSrcH);
  input.depth        = ViewOf(depthBuf, kSrcW, kSrcH);
  input.metadata.outputWidth  = kDstW;
  input.metadata.outputHeight = kDstH;
  input.metadata.exposure = 1.0f;
  input.hasPrevHistory = false;
  input.hasUiLayer    = false;

  auto groundTruth = ViewOf(groundTruthBuf, kDstW, kDstH);

  genframe::FrameGenConfig frameGenConfig{};
  frameGenConfig.enableConfidenceGate = true;
  frameGenConfig.blendAlpha = 0.5F;
  std::unique_ptr<genframe::IFrameGenerator> frameGenerator(
      genframe::CreateSimpleBlendFrameGenerator());
  if (!frameGenerator->Prepare(kDstW, kDstH, frameGenConfig)) {
    std::fprintf(stderr, "[%s] Prepare() failed\n", frameGenerator->Name());
    return 1;
  }

  // ---- Algorithms to benchmark -------------------------------------------
  std::vector<std::unique_ptr<upscale::IUpscaler>> algorithms;
  algorithms.emplace_back(upscale::CreatePassthroughUpscaler());
  algorithms.emplace_back(upscale::CreateBilinearUpscaler());

  std::vector<eval::BenchmarkReport> reports;

  for (auto& algo : algorithms) {
    const bool prepOk = algo->Prepare(kSrcW, kSrcH, kDstW, kDstH,
                                      upscale::QualityPreset::kBalanced);
    if (!prepOk) {
      std::fprintf(stderr, "[%s] Prepare() failed\n", algo->Name());
      continue;
    }

    std::printf("Running algorithm: %s (%u frames)\n", algo->Name(), kFrames);
    PipelineStats stats{};
    eval::BenchmarkReport rep =
      RunAlgorithm(*algo, frameGenerator.get(), input, groundTruth, kFrames,
             outDir, stats);

    const std::string jsonPath =
        outDir + "/benchmark_" + rep.algorithmName + ".json";
    eval::WriteReportJson(rep, jsonPath);
    std::printf("  -> %s\n", jsonPath.c_str());
    std::printf("  framegen(%s): %u/%u frames\n", frameGenerator->Name(),
          stats.generatedFrames, kFrames);

    reports.push_back(rep);
  }

  // ---- Comparison --------------------------------------------------------
  if (reports.size() >= 2) {
    std::printf("\n--- Comparison (baseline: %s vs candidate: %s) ---\n",
                reports[0].algorithmName, reports[1].algorithmName);
    eval::PrintComparison(reports[0], reports[1]);

    // Write merged comparison JSON
    const std::string cmpPath = outDir + "/benchmark_comparison.json";
    std::ofstream cf(cmpPath);
    cf << "[\n";
    for (std::size_t i = 0; i < reports.size(); ++i) {
      const auto& r = reports[i];
      cf << "  {\"algorithm\":\"" << (r.algorithmName ? r.algorithmName : "") << "\""
         << ",\"frame_count\":" << r.frameCount
         << ",\"mean_psnr_db\":" << r.meanPsnr
         << ",\"min_psnr_db\":" << r.minPsnr
         << ",\"max_psnr_db\":" << r.maxPsnr
         << ",\"mean_cpu_time_ms\":" << r.meanCpuTimeMs
         << "}";
      cf << (i + 1 < reports.size() ? ",\n" : "\n");
    }
    cf << "]\n";
    std::printf("Comparison JSON: %s\n", cmpPath.c_str());
  }

  return 0;
}
