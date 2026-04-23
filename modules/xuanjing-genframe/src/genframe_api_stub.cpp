#include "xuanjing-genframe/genframe_api.h"

namespace xuanjing::genframe {

bool GenerateFrame(const FrameGenConfig& config) {
  return config.enable_confidence_gate;
}

}  // namespace xuanjing::genframe
