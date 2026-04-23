#pragma once

#include <cstddef>
#include <cstdint>

namespace xuanjing {
namespace runtime {

enum class PixelFormat {
  kUnknown,
  kRGBA8UNorm,
  kRGBA16Float,
  kRG16Float,
  kR32Float,
};

enum class ColorSpace {
  kUnknown,
  kLinear,
  kSRGB,
};

struct ImageView {
  const void* data = nullptr;
  std::uint32_t width = 0;
  std::uint32_t height = 0;
  std::size_t rowStrideBytes = 0;
  PixelFormat format = PixelFormat::kUnknown;
  ColorSpace colorSpace = ColorSpace::kUnknown;
};

struct FrameMetadata {
  std::uint64_t frameIndex = 0;
  std::uint32_t outputWidth = 0;
  std::uint32_t outputHeight = 0;
  float jitterX = 0.0F;
  float jitterY = 0.0F;
  float exposure = 1.0F;
};

struct FrameInput {
  ImageView lowResColor;
  ImageView motionVectors;
  ImageView depth;
  ImageView uiLayer;
  ImageView prevHistory;
  FrameMetadata metadata;
  bool hasUiLayer = false;
  bool hasPrevHistory = false;
};

struct FrameOutput {
  ImageView highResColor;
  ImageView generatedFrame;
  bool hasGeneratedFrame = false;
};

struct RuntimeContext {
  int deviceId = -1;
};

bool Initialize(RuntimeContext& ctx);
bool ValidateFrameInput(const FrameInput& input);
bool DispatchFrame(RuntimeContext& ctx, const FrameInput& input, FrameOutput& output);

}  // namespace runtime
}  // namespace xuanjing
