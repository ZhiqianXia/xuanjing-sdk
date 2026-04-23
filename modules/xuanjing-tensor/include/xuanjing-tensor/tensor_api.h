#pragma once

namespace xuanjing::tensor {

enum class Backend {
  kGPU,
  kNPU,
  kCPU,
};

bool RunInference(Backend backend);

}  // namespace xuanjing::tensor
