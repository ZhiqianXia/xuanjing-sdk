#include <cstdint>
#include <vector>

#include "xuanjing-shader/shader_api.h"

namespace xuanjing {
namespace shader {

class PassthroughShaderComposer : public IShaderComposer {
 public:
  bool Prepare(std::uint32_t, std::uint32_t, const ShaderComposeConfig&) override { return true; }

  bool Compose(const runtime::FrameInput&, runtime::FrameOutput&) override { return true; }

  const char* Name() const override { return "shader-passthrough"; }
};

class SimpleUiShaderComposer : public IShaderComposer {
 public:
  bool Prepare(std::uint32_t width, std::uint32_t height,
               const ShaderComposeConfig& config) override {
    width_ = width;
    height_ = height;
    enableUiComposite_ = config.enableUiComposite;
    colorBuffer_.resize(static_cast<std::size_t>(width_) * height_ * 4U, 0U);
    return true;
  }

  bool Compose(const runtime::FrameInput& input, runtime::FrameOutput& output) override {
    if (output.highResColor.data == nullptr || output.highResColor.width != width_
        || output.highResColor.height != height_) {
      return true;
    }

    for (std::uint32_t y = 0; y < height_; ++y) {
      const auto* src = static_cast<const std::uint8_t*>(output.highResColor.data)
                        + y * output.highResColor.rowStrideBytes;
      auto* dst = colorBuffer_.data() + static_cast<std::size_t>(y) * width_ * 4U;
      for (std::uint32_t x = 0; x < width_; ++x) {
        dst[x * 4 + 0] = src[x * 4 + 0];
        dst[x * 4 + 1] = src[x * 4 + 1];
        dst[x * 4 + 2] = src[x * 4 + 2];
        dst[x * 4 + 3] = src[x * 4 + 3];
      }
    }

    if (enableUiComposite_ && input.hasUiLayer && input.uiLayer.data != nullptr
        && input.uiLayer.width == width_ && input.uiLayer.height == height_) {
      for (std::uint32_t y = 0; y < height_; ++y) {
        const auto* ui =
            static_cast<const std::uint8_t*>(input.uiLayer.data) + y * input.uiLayer.rowStrideBytes;
        auto* dst = colorBuffer_.data() + static_cast<std::size_t>(y) * width_ * 4U;
        for (std::uint32_t x = 0; x < width_; ++x) {
          const float alpha = static_cast<float>(ui[x * 4 + 3]) / 255.0F;
          for (int c = 0; c < 3; ++c) {
            const float v = (1.0F - alpha) * static_cast<float>(dst[x * 4 + c])
                            + alpha * static_cast<float>(ui[x * 4 + c]);
            dst[x * 4 + c] = static_cast<std::uint8_t>(v + 0.5F);
          }
          dst[x * 4 + 3] = 255U;
        }
      }
    }

    outputView_.data = colorBuffer_.data();
    outputView_.width = width_;
    outputView_.height = height_;
    outputView_.rowStrideBytes = static_cast<std::size_t>(width_) * 4U;
    outputView_.format = runtime::PixelFormat::kRGBA8UNorm;
    outputView_.colorSpace = output.highResColor.colorSpace;
    output.highResColor = outputView_;
    return true;
  }

  const char* Name() const override { return "shader-simple-ui"; }

 private:
  std::uint32_t width_ = 0;
  std::uint32_t height_ = 0;
  bool enableUiComposite_ = true;
  std::vector<std::uint8_t> colorBuffer_;
  runtime::ImageView outputView_{};
};

IShaderComposer* CreatePassthroughShaderComposer() { return new PassthroughShaderComposer(); }

IShaderComposer* CreateSimpleUiShaderComposer() { return new SimpleUiShaderComposer(); }

}  // namespace shader
}  // namespace xuanjing
