#pragma once

#include "xuanjing-runtime/runtime_api.h"

namespace xuanjing {
namespace genframe {

struct FrameGenConfig {
  bool enableConfidenceGate = true;
  float blendAlpha = 0.5F;
};

class IFrameGenerator {
 public:
  virtual ~IFrameGenerator() = default;

  virtual bool Prepare(std::uint32_t width, std::uint32_t height, const FrameGenConfig& config) = 0;
  virtual bool Generate(const runtime::FrameInput& input, runtime::FrameOutput& output) = 0;
  virtual const char* Name() const = 0;
};

IFrameGenerator* CreateDisabledFrameGenerator();
IFrameGenerator* CreateSimpleBlendFrameGenerator();

}  // namespace genframe
}  // namespace xuanjing
