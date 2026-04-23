#pragma once

namespace xuanjing::upscale {

enum class QualityPreset {
  kQuality,
  kBalanced,
  kPerformance,
};

bool RunUpscale(QualityPreset preset);

}  // namespace xuanjing::upscale
