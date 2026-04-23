#include "xuanjing-runtime/pipeline_orchestrator.h"

#include <chrono>
#include <cstdio>
#include <sstream>

namespace xuanjing {
namespace runtime {

PipelineOrchestrator::PipelineOrchestrator(const PipelineStages& stages)
    : stages_(stages) {}

PipelineOrchestrator::~PipelineOrchestrator() = default;

bool PipelineOrchestrator::Initialize(std::uint32_t inputWidth,
                                       std::uint32_t inputHeight,
                                       std::uint32_t outputWidth,
                                       std::uint32_t outputHeight) {
  if (!stages_.temporalProcessor || !stages_.upscaler ||
      !stages_.shaderComposer || !stages_.inferenceHook) {
    std::fprintf(stderr, "PipelineOrchestrator: incomplete stages\n");
    return false;
  }

  inputWidth_ = inputWidth;
  inputHeight_ = inputHeight;
  outputWidth_ = outputWidth;
  outputHeight_ = outputHeight;

  // Initialize temporal processor
  temporal::TemporalConfig temporalConfig;
  temporalConfig.enableDisocclusionReject = true;
  if (!stages_.temporalProcessor->Prepare(inputWidth, inputHeight, temporalConfig)) {
    std::fprintf(stderr, "Temporal processor initialization failed\n");
    return false;
  }

  // Initialize upscaler
  constexpr upscale::QualityPreset kPreset = upscale::QualityPreset::kBalanced;
  if (!stages_.upscaler->Prepare(inputWidth, inputHeight, outputWidth,
                                 outputHeight, kPreset)) {
    std::fprintf(stderr, "Upscaler initialization failed\n");
    return false;
  }

  // Initialize shader composer
  shader::ShaderComposeConfig shaderConfig;
  shaderConfig.enableUiComposite = true;
  shaderConfig.enableProfiling = true;
  if (!stages_.shaderComposer->Prepare(outputWidth, outputHeight, shaderConfig)) {
    std::fprintf(stderr, "Shader composer initialization failed\n");
    return false;
  }

  // Initialize inference hook
  tensor::SessionConfig sessionConfig;
  sessionConfig.backend = tensor::Backend::kCPU;
  if (!stages_.inferenceHook->Initialize(sessionConfig)) {
    std::fprintf(stderr, "Inference hook initialization failed\n");
    return false;
  }

  initialized_ = true;
  std::printf(
      "[Pipeline] Initialized: %ux%u -> %ux%u\n", inputWidth, inputHeight,
      outputWidth, outputHeight);
  return true;
}

bool PipelineOrchestrator::ProcessFrame(const FrameInput& input,
                                         FrameOutput& output) {
  if (!initialized_) {
    std::fprintf(stderr, "Pipeline not initialized\n");
    return false;
  }

  auto tStart = std::chrono::high_resolution_clock::now();

  // Stage 1: Temporal alignment
  auto tStage1 = std::chrono::high_resolution_clock::now();
  temporal::TemporalResult temporalResult;
  if (!stages_.temporalProcessor->Process(input, temporalResult)) {
    std::fprintf(stderr, "[Pipeline] Temporal processing failed\n");
    return false;
  }
  auto tStage2 = std::chrono::high_resolution_clock::now();
  double temporalMs =
      std::chrono::duration<double, std::milli>(tStage2 - tStage1).count();

  // Stage 2: Upscaling (inference will be integrated here in future)
  auto tStage3 = std::chrono::high_resolution_clock::now();
  if (!stages_.upscaler->Upscale(input, output)) {
    std::fprintf(stderr, "[Pipeline] Upscaling failed\n");
    return false;
  }
  auto tStage4 = std::chrono::high_resolution_clock::now();
  double inferenceMs =
      std::chrono::duration<double, std::milli>(tStage4 - tStage3).count();

  // Stage 3: Shader composition (final output)
  auto tStage5 = std::chrono::high_resolution_clock::now();
  if (!stages_.shaderComposer->Compose(input, output)) {
    std::fprintf(stderr, "[Pipeline] Shader composition failed\n");
    return false;
  }
  auto tStage6 = std::chrono::high_resolution_clock::now();
  double shaderMs =
      std::chrono::duration<double, std::milli>(tStage6 - tStage5).count();

  auto tEnd = std::chrono::high_resolution_clock::now();
  double totalMs = std::chrono::duration<double, std::milli>(tEnd - tStart).count();

  // Record metrics
  lastMetrics_.frameIndex = input.metadata.frameIndex;
  lastMetrics_.inputWidth = inputWidth_;
  lastMetrics_.inputHeight = inputHeight_;
  lastMetrics_.outputWidth = outputWidth_;
  lastMetrics_.outputHeight = outputHeight_;
  lastMetrics_.temporalTimeMs = temporalMs;
  lastMetrics_.inferenceTimeMs = inferenceMs;
  lastMetrics_.shaderTimeMs = shaderMs;
  lastMetrics_.totalTimeMs = totalMs;

  return true;
}

std::string PipelineOrchestrator::DescribePipeline() const {
  std::ostringstream oss;
  oss << "Pipeline Configuration:\n";
  oss << "  Temporal:     " << stages_.temporalProcessor->Name() << "\n";
  oss << "  Upscaler:     " << stages_.upscaler->Name() << "\n";
  oss << "  Inference:    " << stages_.inferenceHook->Name() << "\n";
  oss << "  Shader:       " << stages_.shaderComposer->Name() << "\n";
  oss << "  Input:        " << inputWidth_ << "x" << inputHeight_ << "\n";
  oss << "  Output:       " << outputWidth_ << "x" << outputHeight_ << "\n";
  return oss.str();
}

}  // namespace runtime
}  // namespace xuanjing
