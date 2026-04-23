#include "xuanjing-tensor/tensor_api.h"

namespace xuanjing::tensor {

bool RunInference(Backend backend) { return backend != Backend::kCPU; }

}  // namespace xuanjing::tensor
