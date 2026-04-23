# 算法贡献指南 · Algorithm Contribution Guide

本文档说明如何在玄镜 SDK 的现有 pipeline 中接入新算法，以及如何在 benchmark 框架内测试性能与画质。

---

## 1. Pipeline 概览

当前 benchmark runner 执行的完整阶段链：

```
输入 FrameInput
  │
  ├─► [temporal]   历史重投影 / 遮挡过滤         ITemporalProcessor
  │
  ├─► [upscale]    超分辨率重建                  IUpscaler
  │
  ├─► [framegen]   帧生成 / 插帧合成              IFrameGenerator
  │
  ├─► [shader]     后处理 / UI 合成 / 色彩变换    IShaderComposer
  │
  └─► [tensor]     神经网络推理后端               IInferenceHook
        │
        └── 时延写入 FrameMetrics.gpuTimeMs

输出 FrameOutput + eval::BenchmarkReport
```

每个阶段都是独立的纯虚接口（策略模式），替换算法时**不需要修改 benchmark runner 或 pipeline
 代码**，只需要：1. 实现接口 → 2. 注册工厂函数 → 3. 在 benchmark runner 中切换工厂调用。

---

## 2. 各接入点详情

### 2.1 超分辨率算法（最常用接入点）

**接口文件**：`modules/xuanjing-upscale/include/xuanjing-upscale/upscale_api.h`

```cpp
class IUpscaler {
 public:
  virtual bool Prepare(uint32_t inputW, uint32_t inputH,
                       uint32_t outputW, uint32_t outputH,
                       QualityPreset preset) = 0;
  virtual bool Upscale(const FrameInput& input, FrameOutput& output) = 0;
  virtual const char* Name() const = 0;
};
```

**实现步骤**：

1. 在 `modules/xuanjing-upscale/src/` 下新建 `my_upscaler.cpp`：

```cpp
#include "xuanjing-upscale/upscale_api.h"

namespace xuanjing {
namespace upscale {

class MyUpscaler : public IUpscaler {
 public:
  bool Prepare(uint32_t iw, uint32_t ih, uint32_t ow, uint32_t oh,
               QualityPreset) override {
    // 初始化算法所需资源
    return true;
  }

  bool Upscale(const FrameInput& input, FrameOutput& output) override {
    // 实现你的算法，将结果写入 output.highResColor
    return true;
  }

  const char* Name() const override { return "my-upscaler-v1"; }
};

IUpscaler* CreateMyUpscaler() { return new MyUpscaler(); }

}  // namespace upscale
}  // namespace xuanjing
```

2. 在 `modules/xuanjing-upscale/include/xuanjing-upscale/upscale_api.h` 中声明工厂：
```cpp
IUpscaler* CreateMyUpscaler();
```

3. 在 `modules/xuanjing-upscale/CMakeLists.txt` 中添加源文件：
```cmake
add_library(xuanjing-upscale STATIC
  src/upscale_api_stub.cpp
  src/my_upscaler.cpp     # 新增
)
```

4. 在 `samples/benchmark_runner/benchmark_runner.cpp` 中注册：
```cpp
algorithms.emplace_back(upscale::CreateBilinearUpscaler());   // 基线
algorithms.emplace_back(upscale::CreateMyUpscaler());         // 新算法
```

---

### 2.2 帧生成算法

**接口文件**：`modules/xuanjing-genframe/include/xuanjing-genframe/genframe_api.h`

```cpp
class IFrameGenerator {
 public:
  virtual bool Prepare(uint32_t width, uint32_t height,
                       const FrameGenConfig& config) = 0;
  virtual bool Generate(const FrameInput& input, FrameOutput& output) = 0;
  virtual const char* Name() const = 0;
};
```

**典型场景**：实现基于光流的插帧、基于 warping 的合成、或基于扩散模型的帧预测。

注意：`Generate()` 接收的 `output.highResColor` 已经由 `IUpscaler` 写入当前高分辨率帧，
可以用它和 `input.prevHistory` 合成插帧，写入 `output.generatedFrame`。

---

### 2.3 时序处理算法

**接口文件**：`modules/xuanjing-temporal/include/xuanjing-temporal/temporal_api.h`

```cpp
class ITemporalProcessor {
 public:
  virtual bool Prepare(uint32_t width, uint32_t height,
                       const TemporalConfig& config) = 0;
  virtual bool Process(const FrameInput& input, TemporalResult& result) = 0;
  virtual const char* Name() const = 0;
};
```

**典型场景**：实现更精确的运动向量验证、遮挡检测（mesh/point cloud 重投影）、历史颜色
补偿。`result.reprojectedHistory` 和 `result.validityMask` 会自动传入后续 upscale 阶段。

---

### 2.4 神经网络推理后端

**接口文件**：`modules/xuanjing-tensor/include/xuanjing-tensor/tensor_api.h`

```cpp
class IInferenceHook {
 public:
  virtual bool Initialize(Backend backend) = 0;
  virtual bool Run(const InferenceRequest& request, InferenceResult& result) = 0;
  virtual const char* Name() const = 0;
};
```

**典型场景**：接入 Vulkan compute 推理、国产 GPU/NPU 专属 SDK。每帧推理时延自动写入
`FrameMetrics.gpuTimeMs` 并汇总到 `BenchmarkReport.meanGpuTimeMs`。

---

### 2.5 后处理 / Shader 合成

**接口文件**：`modules/xuanjing-shader/include/xuanjing-shader/shader_api.h`

```cpp
class IShaderComposer {
 public:
  virtual bool Prepare(uint32_t width, uint32_t height,
                       const ShaderComposeConfig& config) = 0;
  virtual bool Compose(const FrameInput& input, FrameOutput& output) = 0;
  virtual const char* Name() const = 0;
};
```

**典型场景**：接入 tone mapping、film grain、色彩空间转换（HDR/SDR）、UI alpha 混合。

---

## 3. 如何在 benchmark runner 中对比新旧算法

**只需修改 `samples/benchmark_runner/benchmark_runner.cpp` 中的注册表**，每次跑出同一数据集的 PSNR / 时延对比：

```cpp
// ---- 超分算法注册表 ----------------------------------------
std::vector<std::unique_ptr<upscale::IUpscaler>> algorithms;
algorithms.emplace_back(upscale::CreatePassthroughUpscaler()); // 性能下界
algorithms.emplace_back(upscale::CreateBilinearUpscaler());    // 质量基线
algorithms.emplace_back(upscale::CreateMyUpscaler());          // 你的算法
// -----------------------------------------------------------

// ---- 帧生成算法（一次选一个）-------------------------------
std::unique_ptr<genframe::IFrameGenerator> frameGenerator(
    genframe::CreateSimpleBlendFrameGenerator());              // 默认基线
    // genframe::CreateMyFrameGenerator());                   // 切换新算法
// -----------------------------------------------------------

// ---- 时序处理 ----------------------------------------------
std::unique_ptr<temporal::ITemporalProcessor> temporalProcessor(
    temporal::CreateSimpleTemporalProcessor());                // 默认基线
// -----------------------------------------------------------

// ---- 推理后端 ----------------------------------------------
std::unique_ptr<tensor::IInferenceHook> inferenceHook(
    tensor::CreateCpuReferenceInferenceHook());                // CPU 默认
    // tensor::CreateMockGpuInferenceHook());                 // 切换 GPU
// -----------------------------------------------------------
```

每个算法会输出：
- `benchmark_<name>.json` — 单算法详细报告
- `benchmark_comparison.json` — 所有算法横向对比

---

## 4. 输出指标说明

| 字段 | 单位 | 说明 |
|---|---|---|
| `mean_psnr_db` | dB | 与 ground truth 的均值 PSNR，越高越好 |
| `min_psnr_db` | dB | 最低帧质量，反映最差画面稳定性 |
| `max_psnr_db` | dB | 最高帧质量 |
| `mean_cpu_time_ms` | ms | CPU 端每帧平均时延（含 upscale+framegen+shader） |
| `mean_gpu_time_ms` | ms | 推理后端（tensor hook）每帧平均时延 |

控制台同时输出差异摘要：
```
bilinear              PSNR  33.87 dB  cpu   0.05 ms
my-upscaler-v1        PSNR  38.12 dB  cpu   1.20 ms
delta:  PSNR +4.25 dB  speedup 0.04x
```

---

## 5. 新算法验收门控

每个新算法合入 mainline 前，需满足以下最低标准（参考
`docs/specs/module_goals_acceptance.md`）：

1. **构建门**：在 `dev` 和 `release` 预设下均无编译警告  
2. **接口门**：实现对应 `IXxx` 接口全部纯虚函数  
3. **质量门**：PSNR ≥ bilinear 基线（`benchmark_bilinear.json` 的 `mean_psnr_db`），  
   或在 PR 描述中给出有充分理由的质量/性能权衡说明  
4. **CI 门**：相关模块的构建和 ASAN 作业全部通过  
5. **文档门**：PR 描述中包含参考论文/方法说明，及 benchmark_comparison.json 附件  

---

## 6. 相关文件索引

| 文件 | 说明 |
|---|---|
| `modules/xuanjing-upscale/include/xuanjing-upscale/upscale_api.h` | IUpscaler 接口 |
| `modules/xuanjing-genframe/include/xuanjing-genframe/genframe_api.h` | IFrameGenerator 接口 |
| `modules/xuanjing-temporal/include/xuanjing-temporal/temporal_api.h` | ITemporalProcessor 接口 |
| `modules/xuanjing-shader/include/xuanjing-shader/shader_api.h` | IShaderComposer 接口 |
| `modules/xuanjing-tensor/include/xuanjing-tensor/tensor_api.h` | IInferenceHook 接口 |
| `samples/benchmark_runner/benchmark_runner.cpp` | 算法注册与运行入口 |
| `modules/xuanjing-runtime/include/xuanjing-runtime/runtime_api.h` | FrameInput / FrameOutput 数据结构 |
| `docs/specs/io_spec.md` | 完整 I/O 合约文档 |
| `docs/specs/module_goals_acceptance.md` | 验收标准 |
