#include "xuanjing-runtime/runtime_api.h"

namespace xuanjing::runtime {

bool Initialize(RuntimeContext& ctx) {
  ctx.device_id = 0;
  return true;
}

}  // namespace xuanjing::runtime
