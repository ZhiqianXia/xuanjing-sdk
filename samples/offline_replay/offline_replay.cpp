#include "xuanjing-runtime/runtime_api.h"

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace {

using xuanjing::runtime::ColorSpace;
using xuanjing::runtime::FrameInput;
using xuanjing::runtime::FrameOutput;
using xuanjing::runtime::ImageView;
using xuanjing::runtime::PixelFormat;
using xuanjing::runtime::RuntimeContext;

ImageView MakeImageView(const void* data, std::uint32_t width, std::uint32_t height,
                        std::size_t rowStrideBytes, PixelFormat format,
                        ColorSpace colorSpace) {
  ImageView image;
  image.data = data;
  image.width = width;
  image.height = height;
  image.rowStrideBytes = rowStrideBytes;
  image.format = format;
  image.colorSpace = colorSpace;
  return image;
}

std::vector<std::uint8_t> MakeStaticColorImage(std::uint32_t width,
                                               std::uint32_t height) {
  std::vector<std::uint8_t> pixels(width * height * 4, 0);
  for (std::uint32_t y = 0; y < height; ++y) {
    for (std::uint32_t x = 0; x < width; ++x) {
      const std::size_t index = static_cast<std::size_t>(y * width + x) * 4;
      pixels[index + 0] = static_cast<std::uint8_t>((x * 255) / width);
      pixels[index + 1] = static_cast<std::uint8_t>((y * 255) / height);
      pixels[index + 2] = 160;
      pixels[index + 3] = 255;
    }
  }
  return pixels;
}

bool WritePpm(const ImageView& image, const std::string& path) {
  if (image.format != PixelFormat::kRGBA8UNorm || image.data == nullptr) {
    return false;
  }

  std::ofstream file(path, std::ios::binary);
  if (!file) {
    return false;
  }

  file << "P6\n" << image.width << ' ' << image.height << "\n255\n";

  const auto* pixels = static_cast<const std::uint8_t*>(image.data);
  for (std::uint32_t y = 0; y < image.height; ++y) {
    const auto* row = pixels + static_cast<std::size_t>(y) * image.rowStrideBytes;
    for (std::uint32_t x = 0; x < image.width; ++x) {
      file.write(reinterpret_cast<const char*>(row + x * 4), 3);
    }
  }

  return static_cast<bool>(file);
}

std::string MakeFramePath(std::uint64_t frameIndex) {
  std::ostringstream path;
  path << "offline_replay_frame_" << std::setw(4) << std::setfill('0') << frameIndex
       << ".ppm";
  return path.str();
}

}  // namespace

int main() {
  constexpr std::uint32_t kInputWidth = 8;
  constexpr std::uint32_t kInputHeight = 8;
  constexpr std::uint32_t kOutputWidth = 16;
  constexpr std::uint32_t kOutputHeight = 16;
  constexpr std::uint64_t kFrameCount = 4;

  auto color = MakeStaticColorImage(kInputWidth, kInputHeight);
  std::vector<float> motionVectors(kInputWidth * kInputHeight * 2, 0.0F);
  std::vector<float> depth(kInputWidth * kInputHeight, 1.0F);

  RuntimeContext context;
  if (!xuanjing::runtime::Initialize(context)) {
    std::cerr << "failed to initialize runtime\n";
    return 1;
  }

  for (std::uint64_t frameIndex = 0; frameIndex < kFrameCount; ++frameIndex) {
    FrameInput input;
    input.lowResColor = MakeImageView(color.data(), kInputWidth, kInputHeight,
                                      kInputWidth * 4, PixelFormat::kRGBA8UNorm,
                                      ColorSpace::kSRGB);
    input.motionVectors = MakeImageView(motionVectors.data(), kInputWidth,
                                        kInputHeight, kInputWidth * sizeof(float) * 2,
                                        PixelFormat::kRG16Float, ColorSpace::kLinear);
    input.depth = MakeImageView(depth.data(), kInputWidth, kInputHeight,
                                kInputWidth * sizeof(float), PixelFormat::kR32Float,
                                ColorSpace::kLinear);
    input.metadata.frameIndex = frameIndex;
    input.metadata.outputWidth = kOutputWidth;
    input.metadata.outputHeight = kOutputHeight;
    input.metadata.jitterX = 0.0F;
    input.metadata.jitterY = 0.0F;
    input.metadata.exposure = 1.0F;
    input.hasPrevHistory = frameIndex > 0;
    if (input.hasPrevHistory) {
      input.prevHistory = input.lowResColor;
    }

    FrameOutput output;
    if (!xuanjing::runtime::DispatchFrame(context, input, output)) {
      std::cerr << "dispatch failed for frame " << frameIndex << '\n';
      return 1;
    }

    const std::string path = MakeFramePath(frameIndex);
    if (!WritePpm(output.highResColor, path)) {
      std::cerr << "failed to write output to " << path << '\n';
      return 1;
    }
  }

  std::cout << "offline replay completed for " << kFrameCount
            << " static frames\n";
  return 0;
}