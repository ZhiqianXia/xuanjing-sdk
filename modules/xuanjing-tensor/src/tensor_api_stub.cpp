#include "xuanjing-tensor/tensor_api.h"

namespace xuanjing {
namespace tensor {

class CpuReferenceInferenceHook : public IInferenceHook {
 public:
	bool Initialize(Backend backend) override {
		backend_ = backend;
		return true;
	}

	bool Run(const InferenceRequest& request, InferenceResult& result) override {
		result.backend = backend_;
		result.success = request.input != nullptr || request.inputBytes == 0;
		result.latencyMs = 0.2;
		return result.success;
	}

	const char* Name() const override { return "tensor-cpu-reference"; }

 private:
	Backend backend_ = Backend::kCPU;
};

class MockGpuInferenceHook : public IInferenceHook {
 public:
	bool Initialize(Backend backend) override {
		backend_ = backend;
		return backend == Backend::kGPU || backend == Backend::kNPU;
	}

	bool Run(const InferenceRequest& request, InferenceResult& result) override {
		result.backend = backend_;
		result.success = request.input != nullptr || request.inputBytes == 0;
		result.latencyMs = (backend_ == Backend::kGPU) ? 0.08 : 0.12;
		return result.success;
	}

	const char* Name() const override { return "tensor-mock-gpu"; }

 private:
	Backend backend_ = Backend::kGPU;
};

IInferenceHook* CreateCpuReferenceInferenceHook() {
	return new CpuReferenceInferenceHook();
}

IInferenceHook* CreateMockGpuInferenceHook() {
	return new MockGpuInferenceHook();
}

}  // namespace tensor
}  // namespace xuanjing
