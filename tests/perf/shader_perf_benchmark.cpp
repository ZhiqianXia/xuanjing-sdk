#include <cstdint>
#include <cstdio>
#include <vector>
#include <chrono>
#include <iomanip>
#include <algorithm>

#include "xuanjing-runtime/runtime_api.h"
#include "xuanjing-shader/shader_api.h"
#include "xuanjing-upscale/upscale_api.h"
#include "xuanjing-eval/eval_api.h"

using namespace xuanjing;

// Helper: Create a synthetic checkerboard pattern (RGBA8).
std::vector<std::uint8_t> CreateCheckerboard(std::uint32_t width,
                                              std::uint32_t height,
                                              std::uint32_t checkerSize) {
  std::vector<std::uint8_t> buf(width * height * 4);
  for (std::uint32_t y = 0; y < height; ++y) {
    for (std::uint32_t x = 0; x < width; ++x) {
      bool isWhite = ((x / checkerSize) + (y / checkerSize)) % 2 == 0;
      std::uint8_t gray = isWhite ? 255 : 0;
      buf[(y * width + x) * 4 + 0] = gray;
      buf[(y * width + x) * 4 + 1] = gray;
      buf[(y * width + x) * 4 + 2] = gray;
      buf[(y * width + x) * 4 + 3] = 255;
    }
  }
  return buf;
}

runtime::ImageView ViewOf(std::vector<std::uint8_t>& buf, std::uint32_t w,
                          std::uint32_t h) {
  runtime::ImageView v{};
  v.data = buf.data();
  v.width = w;
  v.height = h;
  v.rowStrideBytes = w * 4;
  v.format = runtime::PixelFormat::kRGBA8UNorm;
  v.colorSpace = runtime::ColorSpace::kLinear;
  return v;
}

// Benchmark upscaler throughput and latency
void BenchmarkUpscaler(upscale::IUpscaler* upscaler,
                       std::uint32_t inputWidth,
                       std::uint32_t inputHeight,
                       std::uint32_t outputWidth,
                       std::uint32_t outputHeight,
                       std::uint32_t frameCount) {
  std::printf("\nBenchmark: %s\n", upscaler->Name());
  std::printf("Input:  %ux%u -> Output: %ux%u\n", inputWidth, inputHeight,
              outputWidth, outputHeight);
  std::printf("Frames: %u\n", frameCount);
  std::printf("---\n");

  // Prepare
  if (!upscaler->Prepare(inputWidth, inputHeight, outputWidth, outputHeight,
                         upscale::QualityPreset::kBalanced)) {
    std::fprintf(stderr, "  [FAIL] Prepare failed\n");
    return;
  }

  // Create test data
  auto inputBuf = CreateCheckerboard(inputWidth, inputHeight, 16);
  auto outputBuf(inputBuf);
  outputBuf.resize(outputWidth * outputHeight * 4);

  runtime::FrameInput input{};
  input.lowResColor = ViewOf(inputBuf, inputWidth, inputHeight);
  input.metadata.outputWidth = outputWidth;
  input.metadata.outputHeight = outputHeight;

  runtime::FrameOutput output{};
  output.highResColor = ViewOf(outputBuf, outputWidth, outputHeight);

  // Warmup (2 frames)
  for (int i = 0; i < 2; ++i) {
    upscaler->Upscale(input, output);
  }

  // Benchmark loop
  std::vector<double> frameTimes;
  frameTimes.reserve(frameCount);

  for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
    input.metadata.frameIndex = frame;

    auto tStart = std::chrono::high_resolution_clock::now();
    bool ok = upscaler->Upscale(input, output);
    auto tEnd = std::chrono::high_resolution_clock::now();

    if (!ok) {
      std::fprintf(stderr, "  [FAIL] Frame %u failed\n", frame);
      return;
    }

    double frameTime =
        std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    frameTimes.push_back(frameTime);
  }

  // Compute statistics
  double minTime = *std::min_element(frameTimes.begin(), frameTimes.end());
  double maxTime = *std::max_element(frameTimes.begin(), frameTimes.end());
  double sumTime = 0.0;
  for (double t : frameTimes) {
    sumTime += t;
  }
  double avgTime = sumTime / frameCount;

  // Compute throughput
  std::uint32_t pixelsPerFrame = outputWidth * outputHeight;
  double throughputGpixels = (pixelsPerFrame * frameCount) / (sumTime * 1e6);
  double throughputFps = 1000.0 / avgTime;

  std::printf("Latency (ms):\n");
  std::printf("  Min:  %.2f\n", minTime);
  std::printf("  Max:  %.2f\n", maxTime);
  std::printf("  Avg:  %.2f\n", avgTime);
  std::printf("Throughput:\n");
  std::printf("  %.1f Gp/s\n", throughputGpixels);
  std::printf("  %.1f FPS @ %ux%u\n", throughputFps, outputWidth, outputHeight);
}

// Benchmark shader composer throughput
void BenchmarkShader(shader::IShaderComposer* shaderComposer,
                     std::uint32_t width,
                     std::uint32_t height,
                     std::uint32_t frameCount) {
  std::printf("\nBenchmark: %s (Shader)\n", shaderComposer->Name());
  std::printf("Resolution: %ux%u\n", width, height);
  std::printf("Frames: %u\n", frameCount);
  std::printf("---\n");

  // Prepare
  shader::ShaderComposeConfig config;
  config.enableProfiling = true;
  config.enableUiComposite = true;

  if (!shaderComposer->Prepare(width, height, config)) {
    std::fprintf(stderr, "  [FAIL] Prepare failed\n");
    return;
  }

  // Create test data
  auto buf = CreateCheckerboard(width, height, 32);

  runtime::FrameInput input{};
  input.lowResColor = ViewOf(buf, width, height);
  input.metadata.outputWidth = width;
  input.metadata.outputHeight = height;

  runtime::FrameOutput output{};
  output.highResColor = ViewOf(buf, width, height);

  // Warmup
  for (int i = 0; i < 2; ++i) {
    shaderComposer->Compose(input, output);
  }

  // Benchmark loop
  std::vector<double> frameTimes;
  frameTimes.reserve(frameCount);

  for (std::uint32_t frame = 0; frame < frameCount; ++frame) {
    input.metadata.frameIndex = frame;

    auto tStart = std::chrono::high_resolution_clock::now();
    bool ok = shaderComposer->Compose(input, output);
    auto tEnd = std::chrono::high_resolution_clock::now();

    if (!ok) {
      std::fprintf(stderr, "  [FAIL] Frame %u failed\n", frame);
      return;
    }

    double frameTime =
        std::chrono::duration<double, std::milli>(tEnd - tStart).count();
    frameTimes.push_back(frameTime);
  }

  // Statistics
  double minTime = *std::min_element(frameTimes.begin(), frameTimes.end());
  double maxTime = *std::max_element(frameTimes.begin(), frameTimes.end());
  double sumTime = 0.0;
  for (double t : frameTimes) {
    sumTime += t;
  }
  double avgTime = sumTime / frameCount;

  std::uint32_t pixelsTotal = width * height * frameCount;
  double throughputGpixels = pixelsTotal / (sumTime * 1e6);
  double throughputFps = 1000.0 / avgTime;

  std::printf("Latency (ms):\n");
  std::printf("  Min:  %.2f\n", minTime);
  std::printf("  Max:  %.2f\n", maxTime);
  std::printf("  Avg:  %.2f\n", avgTime);
  std::printf("Throughput:\n");
  std::printf("  %.1f Gp/s\n", throughputGpixels);
  std::printf("  %.1f FPS\n", throughputFps);
}

int main(int argc, char* argv[]) {
  std::printf("\n========================================\n");
  std::printf(" Xuanjing Performance Benchmarks\n");
  std::printf("========================================\n");

  // Benchmark parameters
  constexpr std::uint32_t kWarmupFrames = 5;
  constexpr std::uint32_t kBenchmarkFrames = 100;

  // Resolutions to test
  struct Resolution {
    std::uint32_t inputW, inputH;
    std::uint32_t outputW, outputH;
    const char* label;
  };

  Resolution resolutions[] = {
      {256, 144, 1024, 576, "Upscale 4x (256p->1080p)"},
      {512, 288, 1024, 576, "Upscale 2x (512p->1080p)"},
      {1920, 1080, 3840, 2160, "Upscale 2x (1080p->4K)"},
  };

  // Test upscalers
  std::printf("\n\n=== UPSCALER BENCHMARKS ===\n");

  auto* passthroughUpscaler = upscale::CreatePassthroughUpscaler();
  auto* bilinearUpscaler = upscale::CreateBilinearUpscaler();

  if (passthroughUpscaler) {
    for (const auto& res : resolutions) {
      std::printf("\n%s\n", res.label);
      BenchmarkUpscaler(passthroughUpscaler, res.inputW, res.inputH,
                        res.outputW, res.outputH, kBenchmarkFrames);
    }
  }

  if (bilinearUpscaler) {
    for (const auto& res : resolutions) {
      std::printf("\n%s\n", res.label);
      BenchmarkUpscaler(bilinearUpscaler, res.inputW, res.inputH, res.outputW,
                        res.outputH, kBenchmarkFrames);
    }
  }

  // Test shaders
  std::printf("\n\n=== SHADER BENCHMARKS ===\n");

  auto* passthroughShader = shader::CreatePassthroughShaderComposer();
  auto* uiShader = shader::CreateSimpleUiShaderComposer();

  struct ShaderResolution {
    std::uint32_t w, h;
    const char* label;
  };

  ShaderResolution shaderResolutions[] = {
      {1024, 576, "1080p"},
      {1920, 1080, "1080p"},
      {3840, 2160, "4K"},
  };

  if (passthroughShader) {
    for (const auto& res : shaderResolutions) {
      std::printf("\n%s\n", res.label);
      BenchmarkShader(passthroughShader, res.w, res.h, kBenchmarkFrames);
    }
  }

  if (uiShader) {
    for (const auto& res : shaderResolutions) {
      std::printf("\n%s\n", res.label);
      BenchmarkShader(uiShader, res.w, res.h, kBenchmarkFrames);
    }
  }

  std::printf("\n========================================\n");
  std::printf(" Benchmarks Complete\n");
  std::printf("========================================\n\n");

  return 0;
}
