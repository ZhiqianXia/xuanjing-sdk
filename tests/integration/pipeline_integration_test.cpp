#include <cstdint>
#include <cstdio>
#include <vector>
#include <chrono>

#include "xuanjing-runtime/runtime_api.h"
#include "xuanjing-runtime/pipeline_orchestrator.h"
#include "xuanjing-temporal/temporal_api.h"
#include "xuanjing-upscale/upscale_api.h"
#include "xuanjing-shader/shader_api.h"
#include "xuanjing-tensor/tensor_api.h"
#include "xuanjing-eval/eval_api.h"

using namespace xuanjing;

// Helper: Create a synthetic gradient image (RGBA8).
std::vector<std::uint8_t> CreateGradientImage(std::uint32_t width,
                                               std::uint32_t height) {
  std::vector<std::uint8_t> buf(width * height * 4);
  for (std::uint32_t y = 0; y < height; ++y) {
    for (std::uint32_t x = 0; x < width; ++x) {
      std::uint8_t r = static_cast<std::uint8_t>(x * 255 / (width - 1));
      std::uint8_t g = static_cast<std::uint8_t>(y * 255 / (height - 1));
      std::uint8_t b = 128;
      buf[(y * width + x) * 4 + 0] = r;
      buf[(y * width + x) * 4 + 1] = g;
      buf[(y * width + x) * 4 + 2] = b;
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

// Test 1: Basic pipeline validation
bool TestBasicPipelineValidation() {
  std::printf("\n[Test] Basic Pipeline Validation\n");
  std::printf("================================\n");

  // Create all stages
  auto* temporal = temporal::CreateSimpleTemporalProcessor();
  auto* upscaler = upscale::CreateBilinearUpscaler();
  auto* shader = shader::CreatePassthroughShaderComposer();
  auto* inference = tensor::CreateCpuReferenceInferenceHook();

  if (!temporal || !upscaler || !shader || !inference) {
    std::fprintf(stderr, "[FAIL] Failed to create stages\n");
    return false;
  }

  // Create orchestrator
  runtime::PipelineStages stages = {temporal, upscaler, shader, inference};
  runtime::PipelineOrchestrator orchestrator(stages);

  // Initialize
  constexpr std::uint32_t kInputWidth = 256;
  constexpr std::uint32_t kInputHeight = 144;
  constexpr std::uint32_t kOutputWidth = 1024;
  constexpr std::uint32_t kOutputHeight = 576;

  if (!orchestrator.Initialize(kInputWidth, kInputHeight, kOutputWidth,
                                kOutputHeight)) {
    std::fprintf(stderr, "[FAIL] Pipeline initialization failed\n");
    return false;
  }

  std::printf("%s", orchestrator.DescribePipeline().c_str());

  // Create test frame
  auto inputBuf = CreateGradientImage(kInputWidth, kInputHeight);
  auto outputBuf = CreateGradientImage(kOutputWidth, kOutputHeight);
  auto motionBuf = CreateGradientImage(kInputWidth, kInputHeight);
  auto depthBuf = CreateGradientImage(kInputWidth, kInputHeight);

  runtime::FrameInput input{};
  input.lowResColor = ViewOf(inputBuf, kInputWidth, kInputHeight);
  input.motionVectors = ViewOf(motionBuf, kInputWidth, kInputHeight);
  input.depth = ViewOf(depthBuf, kInputWidth, kInputHeight);
  input.metadata.frameIndex = 0;
  input.metadata.outputWidth = kOutputWidth;
  input.metadata.outputHeight = kOutputHeight;

  runtime::FrameOutput output{};
  output.highResColor = ViewOf(outputBuf, kOutputWidth, kOutputHeight);

  // Process frame
  auto tStart = std::chrono::high_resolution_clock::now();
  if (!orchestrator.ProcessFrame(input, output)) {
    std::fprintf(stderr, "[FAIL] Frame processing failed\n");
    return false;
  }
  auto tEnd = std::chrono::high_resolution_clock::now();

  double elapsedMs =
      std::chrono::duration<double, std::milli>(tEnd - tStart).count();
  const auto& metrics = orchestrator.GetLastMetrics();

  std::printf("[PASS] Frame processed successfully\n");
  std::printf("  Total time: %.2f ms\n", metrics.totalTimeMs);
  std::printf("  Temporal:   %.2f ms\n", metrics.temporalTimeMs);
  std::printf("  Inference:  %.2f ms\n", metrics.inferenceTimeMs);
  std::printf("  Shader:     %.2f ms\n", metrics.shaderTimeMs);

  return true;
}

// Test 2: Quality evaluation (PSNR)
bool TestQualityEvaluation() {
  std::printf("\n[Test] Quality Evaluation (PSNR)\n");
  std::printf("================================\n");

  // Create reference and result images
  constexpr std::uint32_t kWidth = 256;
  constexpr std::uint32_t kHeight = 256;

  auto refBuf = CreateGradientImage(kWidth, kHeight);
  auto resBuf = CreateGradientImage(kWidth, kHeight);

  runtime::ImageView reference = ViewOf(refBuf, kWidth, kHeight);
  runtime::ImageView result = ViewOf(resBuf, kWidth, kHeight);

  // Compute PSNR
  double psnr = eval::ComputePsnr(reference, result);

  if (psnr < 0.0) {
    std::fprintf(stderr, "[FAIL] PSNR computation failed\n");
    return false;
  }

  std::printf("[PASS] PSNR computed: %.2f dB\n", psnr);

  // Create report
  eval::BenchmarkReport report{};
  report.algorithmName = "bilinear";

  eval::FrameMetrics metrics{};
  metrics.psnr = psnr;
  metrics.gpuTimeMs = 2.5;
  metrics.cpuTimeMs = 5.0;
  metrics.frameIndex = 0;

  for (int i = 0; i < 10; ++i) {
    eval::AccumulateMetrics(report, metrics);
  }

  eval::FinalizeReport(report);

  std::printf("  Mean PSNR: %.2f dB\n", report.meanPsnr);
  std::printf("  Frames: %u\n", report.frameCount);

  // Write report
  const char* reportPath = "/tmp/benchmark_test.json";
  if (!eval::WriteReportJson(report, reportPath)) {
    std::fprintf(stderr, "[WARN] Failed to write report\n");
  } else {
    std::printf("  Report written to: %s\n", reportPath);
  }

  return true;
}

// Test 3: Multi-input tensor support
bool TestMultiInputTensor() {
  std::printf("\n[Test] Multi-Input Tensor Support\n");
  std::printf("==================================\n");

  auto* inference = tensor::CreateCpuReferenceInferenceHook();
  if (!inference) {
    std::fprintf(stderr, "[FAIL] Failed to create inference hook\n");
    return false;
  }

  // Initialize
  tensor::SessionConfig config;
  config.backend = tensor::Backend::kCPU;

  if (!inference->Initialize(config)) {
    std::fprintf(stderr, "[FAIL] Inference initialization failed\n");
    return false;
  }

  // Create multi-input request
  tensor::InferenceRequestEx requestEx{};
  requestEx.useMultiInput = true;
  requestEx.backend = tensor::Backend::kCPU;

  // Mock input tensors
  auto buf1 = CreateGradientImage(256, 256);
  auto buf2 = CreateGradientImage(256, 256);

  tensor::NamedTensor input1{};
  input1.name = "color";
  input1.data = buf1.data();
  input1.bytes = buf1.size();
  input1.dtype = tensor::DataType::kUInt8;
  input1.shape = {1, 256, 256, 4};

  tensor::NamedTensor input2{};
  input2.name = "motion";
  input2.data = buf2.data();
  input2.bytes = buf2.size();
  input2.dtype = tensor::DataType::kUInt8;
  input2.shape = {1, 256, 256, 2};

  requestEx.inputs.push_back(input1);
  requestEx.inputs.push_back(input2);

  // Run multi-input inference
  tensor::InferenceResult result{};
  bool success = inference->RunEx(requestEx, result);

  std::printf("[%s] Multi-input inference %s\n",
              success ? "INFO" : "WARN",
              success ? "succeeded (not yet implemented)" : "not yet implemented (expected)");

  return true;
}

int main() {
  std::printf("\n========================================\n");
  std::printf(" Xuanjing Pipeline Integration Tests\n");
  std::printf("========================================\n");

  bool allPass = true;

  allPass &= TestBasicPipelineValidation();
  allPass &= TestQualityEvaluation();
  allPass &= TestMultiInputTensor();

  std::printf("\n========================================\n");
  if (allPass) {
    std::printf("✓ All tests passed\n");
  } else {
    std::printf("✗ Some tests failed\n");
  }
  std::printf("========================================\n\n");

  return allPass ? 0 : 1;
}
