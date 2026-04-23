#pragma once

#include <cstddef>

namespace xuanjing {
namespace tensor {

enum class Backend {
  kGPU,
  kNPU,
  kCPU,
};

struct InferenceRequest {
  const void* input = nullptr;
  std::size_t inputBytes = 0;
  Backend backend = Backend::kCPU;
};

struct InferenceResult {
  bool success = false;
  double latencyMs = 0.0;
  Backend backend = Backend::kCPU;
};

class IInferenceHook {
 public:
  virtual ~IInferenceHook() = default;

  virtual bool Initialize(Backend backend) = 0;
  virtual bool Run(const InferenceRequest& request, InferenceResult& result) = 0;
  virtual const char* Name() const = 0;
};

IInferenceHook* CreateCpuReferenceInferenceHook();
IInferenceHook* CreateMockGpuInferenceHook();

}  // namespace tensor
}  // namespace xuanjing
