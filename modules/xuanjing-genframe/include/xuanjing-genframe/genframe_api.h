#pragma once

namespace xuanjing::genframe {

struct FrameGenConfig {
  bool enable_confidence_gate = true;
};

bool GenerateFrame(const FrameGenConfig& config);

}  // namespace xuanjing::genframe
