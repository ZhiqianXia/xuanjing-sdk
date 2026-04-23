#pragma once

namespace xuanjing::runtime {

struct RuntimeContext {
  int device_id = -1;
};

bool Initialize(RuntimeContext& ctx);

}  // namespace xuanjing::runtime
