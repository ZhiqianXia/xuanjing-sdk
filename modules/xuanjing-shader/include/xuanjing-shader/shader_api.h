#pragma once

#include "xuanjing-runtime/runtime_api.h"
#include <vector>
#include <string>

namespace xuanjing {
namespace shader {

struct ShaderComposeConfig {
  bool enableUiComposite = true;
  bool enableProfiling = false;   // Track GPU time
  bool enableDebugOutput = false; // For debugging invalid shader
};

// Performance metrics for shader execution.
struct ShaderMetrics {
  double uploadTimeMs = 0.0;
  double computeTimeMs = 0.0;
  double downloadTimeMs = 0.0;
  double totalTimeMs = 0.0;
  std::uint32_t pixelsProcessed = 0;
};

class IShaderComposer {
 public:
  virtual ~IShaderComposer() = default;

  virtual bool Prepare(std::uint32_t width, std::uint32_t height,
                       const ShaderComposeConfig& config) = 0;
  
  virtual bool Compose(const runtime::FrameInput& input, runtime::FrameOutput& output) = 0;
  
  // Get last composition metrics (if profiling enabled).
  virtual const ShaderMetrics* GetLastMetrics() const {
    return nullptr;
  }
  
  virtual const char* Name() const = 0;
};

IShaderComposer* CreatePassthroughShaderComposer();
IShaderComposer* CreateSimpleUiShaderComposer();

}  // namespace shader
}  // namespace xuanjing
