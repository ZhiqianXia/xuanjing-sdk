#pragma once

#include "runtime_api.h"
#include "xuanjing-temporal/temporal_api.h"
#include "xuanjing-upscale/upscale_api.h"
#include "xuanjing-shader/shader_api.h"
#include "xuanjing-tensor/tensor_api.h"

#include <memory>
#include <string>

namespace xuanjing {
namespace runtime {

/**
 * PipelineOrchestrator — orchestrates the full super-resolution pipeline:
 *   Input → Temporal Alignment → Inference → Shader Composition → Output
 *
 * Workflow:
 *   1. Create orchestrator with all stages (temporal, upscaler, shader, inference)
 *   2. Call ProcessFrame() for each input frame
 *   3. Access metrics/results from the orchestrator state
 */
struct PipelineStages {
  temporal::ITemporalProcessor* temporalProcessor = nullptr;
  upscale::IUpscaler* upscaler = nullptr;
  shader::IShaderComposer* shaderComposer = nullptr;
  tensor::IInferenceHook* inferenceHook = nullptr;
};

struct PipelineMetrics {
  // Per-stage timings (ms)
  double temporalTimeMs = 0.0;
  double inferenceTimeMs = 0.0;
  double shaderTimeMs = 0.0;
  double totalTimeMs = 0.0;
  
  // Frame info
  std::uint64_t frameIndex = 0;
  std::uint32_t inputWidth = 0;
  std::uint32_t inputHeight = 0;
  std::uint32_t outputWidth = 0;
  std::uint32_t outputHeight = 0;
};

class PipelineOrchestrator {
 public:
  PipelineOrchestrator(const PipelineStages& stages);
  ~PipelineOrchestrator();

  // One-time initialization. Call this before ProcessFrame().
  bool Initialize(std::uint32_t inputWidth, std::uint32_t inputHeight,
                  std::uint32_t outputWidth, std::uint32_t outputHeight);

  // Process a single frame through the entire pipeline.
  bool ProcessFrame(const FrameInput& input, FrameOutput& output);

  // Retrieve metrics from last ProcessFrame().
  const PipelineMetrics& GetLastMetrics() const {
    return lastMetrics_;
  }

  // Human-readable pipeline description.
  std::string DescribePipeline() const;

 private:
  PipelineStages stages_;
  PipelineMetrics lastMetrics_;
  std::uint32_t inputWidth_ = 0;
  std::uint32_t inputHeight_ = 0;
  std::uint32_t outputWidth_ = 0;
  std::uint32_t outputHeight_ = 0;
  bool initialized_ = false;
};

}  // namespace runtime
}  // namespace xuanjing
