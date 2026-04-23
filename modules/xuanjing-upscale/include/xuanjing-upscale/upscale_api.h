#pragma once

#include "xuanjing-runtime/runtime_api.h"

#include <string>

namespace xuanjing {
namespace upscale {

using runtime::FrameInput;
using runtime::FrameOutput;

enum class QualityPreset {
  kQuality,
  kBalanced,
  kPerformance,
};

// IUpscaler — algorithm strategy interface.
// Each upscale algorithm implements this interface.
// The same pipeline can compare algorithms by swapping the pointer.
class IUpscaler {
public:
  virtual ~IUpscaler() = default;

  // One-time preparation. Called once after initialization.
  virtual bool Prepare(std::uint32_t inputWidth, std::uint32_t inputHeight,
                       std::uint32_t outputWidth, std::uint32_t outputHeight,
                       QualityPreset preset) = 0;

  // Per-frame upscale. Writes result into output.highResColor.
  // The output ImageView should reuse the implementation's internal buffer.
  virtual bool Upscale(const FrameInput& input, FrameOutput& output) = 0;

  // Human-readable name, e.g. "bilinear", "fsr1", "xuanjing-v0".
  virtual const char* Name() const = 0;
};

// Factory helpers — each algorithm registers here.
IUpscaler* CreateBilinearUpscaler();   // CPU-reference, always available
IUpscaler* CreatePassthroughUpscaler(); // identity (input = output), for baseline

}  // namespace upscale
}  // namespace xuanjing
