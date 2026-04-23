#include "xuanjing-tensor/tensor_api.h"

#include "xuanjing-model/model_api.h"

#include <string>

namespace xuanjing {
namespace tensor {

class CpuReferenceInferenceHook : public IInferenceHook {
 public:
  bool Initialize(const SessionConfig& config) override {
    backend_ = config.backend;

    activeModelId_.clear();
    activeModelPath_.clear();

    if (config.modelId == nullptr || config.modelId[0] == '\0') {
      return true;
    }

    const char* modelPath = nullptr;
    model::ModelFormat format = model::ModelFormat::kONNX;
    if (!model::ResolveModelPath(config.modelId, &modelPath, &format, nullptr)) {
      return false;
    }
    if (format != model::ModelFormat::kONNX || modelPath == nullptr) {
      return false;
    }

    activeModelId_ = config.modelId;
    activeModelPath_ = modelPath;
    return true;
  }

  bool Run(const InferenceRequest& request, InferenceResult& result) override {
    result.backend = backend_;
    result.success = request.input != nullptr || request.inputBytes == 0;
    result.latencyMs = 0.2;
    return result.success;
  }

  const char* ActiveModelId() const override {
    return activeModelId_.empty() ? nullptr : activeModelId_.c_str();
  }

  const char* Name() const override { return "tensor-cpu-reference"; }

 private:
  Backend backend_ = Backend::kCPU;
  std::string activeModelId_;
  std::string activeModelPath_;
};

class MockGpuInferenceHook : public IInferenceHook {
 public:
  bool Initialize(const SessionConfig& config) override {
    backend_ = config.backend;
    activeModelId_.clear();

    if (!(backend_ == Backend::kGPU || backend_ == Backend::kNPU)) {
      return false;
    }

    if (config.modelId == nullptr || config.modelId[0] == '\0') {
      return true;
    }

    const char* modelPath = nullptr;
    model::ModelFormat format = model::ModelFormat::kONNX;
    if (!model::ResolveModelPath(config.modelId, &modelPath, &format, nullptr)) {
      return false;
    }
    if (format != model::ModelFormat::kONNX || modelPath == nullptr) {
      return false;
    }

    activeModelId_ = config.modelId;
    return true;
  }

  bool Run(const InferenceRequest& request, InferenceResult& result) override {
    result.backend = backend_;
    result.success = request.input != nullptr || request.inputBytes == 0;
    result.latencyMs = (backend_ == Backend::kGPU) ? 0.08 : 0.12;
    return result.success;
  }

  const char* ActiveModelId() const override {
    return activeModelId_.empty() ? nullptr : activeModelId_.c_str();
  }

  const char* Name() const override { return "tensor-mock-gpu"; }

 private:
  Backend backend_ = Backend::kGPU;
  std::string activeModelId_;
};

IInferenceHook* CreateCpuReferenceInferenceHook() { return new CpuReferenceInferenceHook(); }

IInferenceHook* CreateMockGpuInferenceHook() { return new MockGpuInferenceHook(); }

}  // namespace tensor
}  // namespace xuanjing
