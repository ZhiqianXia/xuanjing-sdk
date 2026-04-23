#include "xuanjing-upscale/upscale_api.h"

#include <vector>
#include <cstring>

namespace xuanjing {
namespace upscale {

// ---------------------------------------------------------------------------
// PassthroughUpscaler
// ---------------------------------------------------------------------------
class PassthroughUpscaler : public IUpscaler {
public:
  bool Prepare(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t,
               QualityPreset) override {
    return true;
  }

  bool Upscale(const FrameInput& input, FrameOutput& output) override {
    // Identity: output = input (same resolution; no scaling performed)
    output.highResColor = input.lowResColor;
    output.hasGeneratedFrame = false;
    return true;
  }

  const char* Name() const override { return "passthrough"; }
};

// ---------------------------------------------------------------------------
// BilinearUpscaler
// ---------------------------------------------------------------------------
class BilinearUpscaler : public IUpscaler {
public:
  bool Prepare(std::uint32_t inputW, std::uint32_t inputH,
               std::uint32_t outputW, std::uint32_t outputH,
               QualityPreset) override {
    outputW_ = outputW;
    outputH_ = outputH;
    buffer_.resize(outputW_ * outputH_ * 4);  // RGBA8
    return true;
  }

  bool Upscale(const FrameInput& input, FrameOutput& output) override {
    const auto& src = input.lowResColor;
    if (src.data == nullptr) return false;

    const std::uint32_t srcW = src.width;
    const std::uint32_t srcH = src.height;

    for (std::uint32_t y = 0; y < outputH_; ++y) {
      for (std::uint32_t x = 0; x < outputW_; ++x) {
        float u = (x + 0.5f) * static_cast<float>(srcW) /
                  static_cast<float>(outputW_);
        float v = (y + 0.5f) * static_cast<float>(srcH) /
                  static_cast<float>(outputH_);

        const std::uint32_t x0 = static_cast<std::uint32_t>(u);
        const std::uint32_t y0 = static_cast<std::uint32_t>(v);
        const std::uint32_t x1 = (x0 + 1 < srcW) ? x0 + 1 : x0;
        const std::uint32_t y1 = (y0 + 1 < srcH) ? y0 + 1 : y0;

        const float fx = u - static_cast<float>(x0);
        const float fy = v - static_cast<float>(y0);

        // Fetch 4 source pixels (RGBA8)
        const auto* row0 =
            static_cast<const std::uint8_t*>(src.data) + y0 * src.rowStrideBytes;
        const auto* row1 =
            static_cast<const std::uint8_t*>(src.data) + y1 * src.rowStrideBytes;

        std::uint8_t* dst = buffer_.data() + (y * outputW_ + x) * 4;
        for (int c = 0; c < 4; ++c) {
          float p = (1.f - fy) * ((1.f - fx) * row0[x0 * 4 + c] +
                                  fx * row0[x1 * 4 + c]) +
                    fy * ((1.f - fx) * row1[x0 * 4 + c] +
                          fx * row1[x1 * 4 + c]);
          dst[c] = static_cast<std::uint8_t>(p + 0.5f);
        }
      }
    }

    view_.data = buffer_.data();
    view_.width = outputW_;
    view_.height = outputH_;
    view_.rowStrideBytes = outputW_ * 4;
    view_.format = runtime::PixelFormat::kRGBA8UNorm;
    view_.colorSpace = src.colorSpace;

    output.highResColor = view_;
    output.hasGeneratedFrame = false;
    return true;
  }

  const char* Name() const override { return "bilinear"; }

private:
  std::uint32_t outputW_ = 0;
  std::uint32_t outputH_ = 0;
  std::vector<std::uint8_t> buffer_;
  runtime::ImageView view_{};
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------
IUpscaler* CreatePassthroughUpscaler() { return new PassthroughUpscaler(); }
IUpscaler* CreateBilinearUpscaler()    { return new BilinearUpscaler(); }

}  // namespace upscale
}  // namespace xuanjing
