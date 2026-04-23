#include <algorithm>
#include <cstdint>
#include <vector>

#include "xuanjing-genframe/genframe_api.h"

namespace xuanjing {
namespace genframe {

class DisabledFrameGenerator : public IFrameGenerator {
 public:
  bool Prepare(std::uint32_t, std::uint32_t, const FrameGenConfig&) override { return true; }

  bool Generate(const runtime::FrameInput&, runtime::FrameOutput& output) override {
    output.generatedFrame = {};
    output.hasGeneratedFrame = false;
    return true;
  }

  const char* Name() const override { return "disabled"; }
};

class SimpleBlendFrameGenerator : public IFrameGenerator {
 public:
  bool Prepare(std::uint32_t width, std::uint32_t height, const FrameGenConfig& config) override {
    width_ = width;
    height_ = height;
    blendAlpha_ = std::clamp(config.blendAlpha, 0.0F, 1.0F);
    enableConfidenceGate_ = config.enableConfidenceGate;
    buffer_.resize(static_cast<std::size_t>(width_) * height_ * 4U);
    return true;
  }

  bool Generate(const runtime::FrameInput& input, runtime::FrameOutput& output) override {
    if (!input.hasPrevHistory || input.prevHistory.data == nullptr
        || output.highResColor.data == nullptr) {
      output.generatedFrame = {};
      output.hasGeneratedFrame = false;
      return true;
    }

    const std::uint32_t width = output.highResColor.width;
    const std::uint32_t height = output.highResColor.height;
    if (width == 0 || height == 0 || width != width_ || height != height_) {
      output.generatedFrame = {};
      output.hasGeneratedFrame = false;
      return true;
    }

    if (enableConfidenceGate_ && input.motionVectors.data != nullptr) {
      const auto* mv = static_cast<const std::uint8_t*>(input.motionVectors.data);
      std::uint64_t sum = 0;
      const std::size_t count =
          static_cast<std::size_t>(input.motionVectors.width) * input.motionVectors.height;
      for (std::size_t i = 0; i < count; ++i) {
        sum += mv[i * 4];
      }
      const float avgMotion = static_cast<float>(sum) / static_cast<float>(count);
      if (avgMotion > 220.0F) {
        output.generatedFrame = {};
        output.hasGeneratedFrame = false;
        return true;
      }
    }

    const auto* prev = static_cast<const std::uint8_t*>(input.prevHistory.data);
    const auto* curr = static_cast<const std::uint8_t*>(output.highResColor.data);

    for (std::uint32_t y = 0; y < height_; ++y) {
      const auto* prevRow = prev + y * input.prevHistory.rowStrideBytes;
      const auto* currRow = curr + y * output.highResColor.rowStrideBytes;
      auto* dstRow = buffer_.data() + static_cast<std::size_t>(y) * width_ * 4U;
      for (std::uint32_t x = 0; x < width_; ++x) {
        for (int c = 0; c < 4; ++c) {
          const float v = (1.0F - blendAlpha_) * static_cast<float>(prevRow[x * 4 + c])
                          + blendAlpha_ * static_cast<float>(currRow[x * 4 + c]);
          dstRow[x * 4 + c] = static_cast<std::uint8_t>(v + 0.5F);
        }
      }
    }

    generatedView_.data = buffer_.data();
    generatedView_.width = width_;
    generatedView_.height = height_;
    generatedView_.rowStrideBytes = static_cast<std::size_t>(width_) * 4U;
    generatedView_.format = runtime::PixelFormat::kRGBA8UNorm;
    generatedView_.colorSpace = output.highResColor.colorSpace;
    output.generatedFrame = generatedView_;
    output.hasGeneratedFrame = true;
    return true;
  }

  const char* Name() const override { return "simple-blend"; }

 private:
  std::uint32_t width_ = 0;
  std::uint32_t height_ = 0;
  float blendAlpha_ = 0.5F;
  bool enableConfidenceGate_ = true;
  std::vector<std::uint8_t> buffer_;
  runtime::ImageView generatedView_{};
};

IFrameGenerator* CreateDisabledFrameGenerator() { return new DisabledFrameGenerator(); }

IFrameGenerator* CreateSimpleBlendFrameGenerator() { return new SimpleBlendFrameGenerator(); }

}  // namespace genframe
}  // namespace xuanjing
