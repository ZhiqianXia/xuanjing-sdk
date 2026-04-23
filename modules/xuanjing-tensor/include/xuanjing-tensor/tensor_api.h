#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <string>

namespace xuanjing {
namespace tensor {

enum class Backend {
  kGPU,
  kNPU,
  kCPU,
};

enum class DataType {
  kFloat32,
  kFloat16,
  kUInt8,
  kInt32,
};

// Named tensor input for multi-input ONNX graphs.
struct NamedTensor {
  std::string name;          // e.g., "input", "motion_vectors", "depth"
  const void* data = nullptr;
  std::size_t bytes = 0;
  DataType dtype = DataType::kFloat32;
  std::vector<std::uint32_t> shape;  // e.g., {1, 3, 256, 256} for NCHW
};

// Per-output tensor result with name and timing.
struct OutputTensor {
  std::string name;
  void* data = nullptr;
  std::size_t bytes = 0;
  DataType dtype = DataType::kFloat32;
  std::vector<std::uint32_t> shape;
};

// Legacy single-input request (backward compat).
struct InferenceRequest {
  const void* input = nullptr;
  std::size_t inputBytes = 0;
  Backend backend = Backend::kCPU;
};

// Multi-input request for ONNX models.
struct InferenceRequestEx {
  std::vector<NamedTensor> inputs;
  Backend backend = Backend::kCPU;
  bool useMultiInput = false;  // When true, use inputs[] instead of single input
};

struct SessionConfig {
  Backend backend = Backend::kCPU;
  const char* modelId = nullptr;
};

struct InferenceResult {
  bool success = false;
  double latencyMs = 0.0;
  double uploadMs = 0.0;
  double computeMs = 0.0;
  double downloadMs = 0.0;
  Backend backend = Backend::kCPU;
  std::vector<OutputTensor> outputs;
};

class IInferenceHook {
 public:
  virtual ~IInferenceHook() = default;

  virtual bool Initialize(const SessionConfig& config) = 0;
  virtual bool Run(const InferenceRequest& request, InferenceResult& result) = 0;
  
  // Multi-input variant (new).
  virtual bool RunEx(const InferenceRequestEx& request, InferenceResult& result) {
    // Default implementation: unsupported.
    result.success = false;
    return false;
  }
  
  virtual const char* ActiveModelId() const = 0;
  virtual const char* Name() const = 0;
};

IInferenceHook* CreateCpuReferenceInferenceHook();
IInferenceHook* CreateMockGpuInferenceHook();

}  // namespace tensor
}  // namespace xuanjing
