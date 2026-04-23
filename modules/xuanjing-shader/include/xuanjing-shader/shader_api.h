#pragma once

#include "xuanjing-runtime/runtime_api.h"

namespace xuanjing {
namespace shader {

struct ShaderComposeConfig {
  bool enableUiComposite = true;
};

class IShaderComposer {
 public:
  virtual ~IShaderComposer() = default;

  virtual bool Prepare(std::uint32_t width, std::uint32_t height,
                       const ShaderComposeConfig& config) = 0;
  virtual bool Compose(const runtime::FrameInput& input, runtime::FrameOutput& output) = 0;
  virtual const char* Name() const = 0;
};

IShaderComposer* CreatePassthroughShaderComposer();
IShaderComposer* CreateSimpleUiShaderComposer();

}  // namespace shader
}  // namespace xuanjing
