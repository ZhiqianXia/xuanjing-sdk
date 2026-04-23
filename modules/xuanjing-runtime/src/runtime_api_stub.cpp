#include "xuanjing-runtime/runtime_api.h"

namespace xuanjing {
namespace runtime {

namespace {

bool IsValidImage(const ImageView& image) {
  return image.data != nullptr && image.width > 0 && image.height > 0 &&
         image.rowStrideBytes > 0 && image.format != PixelFormat::kUnknown;
}

}  // namespace

bool Initialize(RuntimeContext& ctx) {
  ctx.deviceId = 0;
  return true;
}

bool ValidateFrameInput(const FrameInput& input) {
  if (!IsValidImage(input.lowResColor) || !IsValidImage(input.motionVectors) ||
      !IsValidImage(input.depth)) {
    return false;
  }

  if (input.lowResColor.width != input.motionVectors.width ||
      input.lowResColor.height != input.motionVectors.height ||
      input.lowResColor.width != input.depth.width ||
      input.lowResColor.height != input.depth.height) {
    return false;
  }

  if (input.metadata.outputWidth == 0 || input.metadata.outputHeight == 0 ||
      input.metadata.exposure <= 0.0F) {
    return false;
  }

  if (input.hasUiLayer && !IsValidImage(input.uiLayer)) {
    return false;
  }

  if (input.hasPrevHistory && !IsValidImage(input.prevHistory)) {
    return false;
  }

  return true;
}

bool DispatchFrame(RuntimeContext& ctx, const FrameInput& input, FrameOutput& output) {
  if (ctx.deviceId < 0 || !ValidateFrameInput(input)) {
    return false;
  }

  // Stage A stub behavior: forward the low-res input as the visible output so
  // the offline replay path can validate I/O, scheduling, and history handling.
  output.highResColor = input.lowResColor;
  output.generatedFrame = {};
  output.hasGeneratedFrame = false;
  return true;
}

}  // namespace runtime
}  // namespace xuanjing
