# Xuanjing SDK — Software Architecture

## 1. 模块输入/输出速查表

| 模块 | 输入 | 输出 |
|---|---|---|
| **xuanjing-runtime** | Engine API 调用（初始化参数：设备 handle、分辨率、质量预设、功能开关）；每帧：低分辨率 color、MV、Depth、Jitter、Exposure、temporal history | 向子模块派发的任务队列；高分辨率最终帧（转交 shader） |
| **xuanjing-platform** | Driver/Device 查询（VkDevice / D3D12Device） | CapabilityProfile（支持格式、MV 精度、max texture size、feature flags） |
| **xuanjing-temporal** | 当前帧 MV、Depth；上一帧高分辨率 history | 对齐后的 history 帧、遮挡 mask、有效性 confidence map |
| **xuanjing-upscale** | 低分辨率 color、aligned history、confidence map、quality preset | 高分辨率重建帧 |
| **xuanjing-genframe** | 前后两帧高分辨率帧 + MV + Depth + Exposure；occlusion mask（来自 temporal） | 合成的插值帧；低置信度时 fallback 信号 |
| **xuanjing-shader** | 中间纹理（upscale/genframe 输出）、色彩空间参数、UI layer（独立合成） | 显示就绪的 HDR/SDR buffer |
| **xuanjing-compiler** | ComputeGraph（算子图）、TargetISA（kSPIRV/kDXIL/…） | CompiledKernel（SPIR-V words 或 DXIL bytes） |
| **xuanjing-tensor** | CompiledKernel、模型权重包、输入 tensor | 推理结果 tensor |
| **xuanjing-eval** | 输出帧 + ground-truth 参考帧 | PSNR、SSIM、LPIPS、帧时延、flicker rate；CI 质量报告 |
| **xuanjing-model** | 原始渲染序列 + GT 高分辨率序列；dataset 注册 | 训练模型权重包（含版本 + checksum + backend compat） |

> **UI 合成约束**：HUD/UI 层（无 MV、无 Depth）必须在 SDK 处理前与 3D 场景分离，在 xuanjing-shader 的最后步骤才合入。否则会产生 ghosting 和 trailing 伪影。

---

## 2. 总体分层

```
┌──────────────────────────────────────────────────────────────┐
│                    Engine / Application                       │
│          (Game Engine, Renderer, Research Harness)            │
└───────────────────────────┬──────────────────────────────────┘
                            │  Public C++ API
┌───────────────────────────▼──────────────────────────────────┐
│                    xuanjing-runtime                           │
│         Orchestration · Lifecycle · Pipeline Dispatch         │
└──┬──────────────┬────────────────┬───────────┬───────────────┘
   │              │                │           │
   ▼              ▼                ▼           ▼
temporal      upscale           shader       eval
   │              │                │
   └──────────────┴────────────────┘
                  │ ComputeGraph (High-Level IR)
                  ▼
          ┌───────────────┐
          │xuanjing-       │
          │  compiler      │
          │               │
          │  Graph IR      │
          │  Pass Manager  │
          │  SPIR-V codegen│◄── DXIL (planned)
          └───────┬───────┘
                  │ CompiledKernel (SPIR-V words / DXIL bytes)
                  ▼
          ┌───────────────┐
          │xuanjing-tensor │
          │               │
          │ Backend Select │
          │ JIT Cache      │
          │ Kernel Dispatch│
          └───────┬───────┘
                  │ hardware calls
                  ▼
          ┌───────────────┐
          │xuanjing-       │
          │  platform      │
          │               │
          │CapabilityProbe │
          │ DriverInterop  │
          └───────┬───────┘
                  │
        ┌─────────┼──────────┐
        ▼         ▼          ▼
    Vulkan      D3D12      CPU ref
  (NV/AMD/     (DX12/      (scalar
  国产 GPU)     WDDM)      fallback)
```

---

## 2. 模块职责

| 层次 | 模块 | 职责 |
|---|---|---|
| **Orchestration** | xuanjing-runtime | 接收引擎调用；管理 RuntimeContext 生命周期；按拓扑顺序驱动子模块 |
| **Algorithm** | xuanjing-temporal | 运动向量验证、历史重投影、遮挡检测、有效性 mask |
| **Algorithm** | xuanjing-upscale | 超分辨率重建；质量预设路由；消费 temporal 信号 |
| **Algorithm** | xuanjing-genframe | 帧生成与合成；置信度门控；自动 fallback |
| **Rendering** | xuanjing-shader | shader pipeline bootstrap；色彩空间转换；UI 合成入口 |
| **Compiler** | xuanjing-compiler | 接收高层 ComputeGraph；运行优化 Pass；生成 SPIR-V（主路）/ DXIL（规划） |
| **Execution** | xuanjing-tensor | 后端选择（GPU/NPU/CPU）；JIT 缓存；kernel dispatch |
| **Platform** | xuanjing-platform | 设备能力探测；Capability Profile；driver interop bootstrap |
| **Evaluation** | xuanjing-eval | PSNR/SSIM/LPIPS；帧时延；CI 质量报告 |
| **Training** | xuanjing-model | 数据集注册；训练循环；模型包导出（版本+checksum） |

---

## 3. 数据流（E2E 单帧）

`→` 运行时数据路径；`··>` 编译/评估路径（离线或 lazy）

```
Engine / Renderer
  │  [初始化] device handle · 分辨率 · 质量预设 · 功能开关
  │  [每  帧] low-res color · MV · depth · jitter · exposure · prev HR history
  ▼
xuanjing-runtime  ──────────────────────────────────────────────────────
  │  device handle                xuanjing-platform
  │  ─────────────────────────►   IN  VkDevice / D3D12Device
  │  ◄─────────────────────────   OUT CapabilityProfile
  │                                   (格式 · MV 精度 · feature flags)
  │
  ├──[MV · depth · prev HR history]────────────────────────────────────►
  │                                   xuanjing-temporal
  │                                   IN  current MV, depth
  │                                   IN  prev HR history frame
  │  ◄──[aligned history]────────     OUT aligned history frame
  │  ◄──[occlusion mask]─────────     OUT occlusion mask
  │  ◄──[confidence map]─────────     OUT validity confidence map
  │
  ├──[low-res color · quality preset]──────────────────────────────────►
  │  [aligned history · occlusion mask · confidence map]─────────────  ►
  │                                   xuanjing-upscale
  │                                   IN  低分辨率 color
  │                                   IN  aligned history · confidence map
  │                                   IN  quality preset
  │   ··> ComputeGraph ··>            OUT 高分辨率重建帧
  │        xuanjing-compiler               ··> 推理结果 tensor
  │        IN  ComputeGraph · TargetISA     (via xuanjing-tensor)
  │        OUT CompiledKernel (SPIR-V)
  │         ··> xuanjing-tensor
  │             IN  CompiledKernel · 模型权重包 · input tensor
  │             OUT 推理结果 tensor ··> upscale
  │
  ├──[HR 重建帧]───────────────────────────────────────────────────────►
  │                                   xuanjing-shader
  │                                   IN  HR 重建帧 / genframe 输出
  │                                   IN  色彩空间参数
  │                                   IN  UI layer（独立，最后合入）
  │                                   OUT HDR / SDR display-ready buffer
  │
  ▼
Display / Composite
  ··> [output frame] ··> xuanjing-eval
                         IN  输出帧 + GT 参考帧
                         OUT PSNR · SSIM · LPIPS · 帧时延 · flicker rate
                             CI 质量报告
```

> **注意**：UI/HUD layer 必须在进入 SDK 前从 3D 场景分离，由 xuanjing-shader 在最末步骤单独合入，否则产生 ghosting 伪影。

---

## 4. IR 层设计（xuanjing-compiler 内部）

```
算法模块 (upscale / temporal / shader)
  │  描述算子图
  ▼
[ High-Level IR ]
  ComputeGraph — OpNode (SSA) — TensorDesc
  │
  ▼
[ Pass Manager ]
  1. DeadNodeElimPass   — 删除无用节点
  2. OpFusionPass       — 融合相邻 elementwise 算子
  3. LayoutNormPass     — 统一 tensor layout
  4. (可扩展 Pass)
  │
  ▼
[ Low-Level IR / KernelSpec ]
  tile shape · wave size · shared memory scope
  │
  ├──[主路]──► SPIR-V Codegen  →  xuanjing-tensor (JIT Cache → GPU dispatch)
  │
  └──[规划]··► DXIL Codegen   ··> xuanjing-tensor (D3D12 backend)
```

---

## 5. 帧生成路径（M2 — xuanjing-genframe）

```
prev HR frame + MV + depth + exposure
current low-res color + MV
  │
  ▼
xuanjing-temporal
  IN  current MV · depth · prev HR frame
  OUT occlusion mask (供帧生成使用)
  │
  ▼ occlusion mask
xuanjing-genframe
  IN  prev/curr HR frame · MV · depth · exposure · occlusion mask
  OUT 合成插值帧
      [low confidence / scene-cut → fallback 信号，不输出插值帧]
  │
  ▼
xuanjing-shader
  IN  genframe 输出（或 fallback 转 upscale 输出）
  IN  UI layer（独立合入）
  OUT HDR / SDR buffer → Display（高刷新率）
```

---

## 6. 训练与模型更新路径

```
Dataset
  原始渲染序列 + GT 高分辨率序列
  │
  ▼
xuanjing-model
  IN  dataset 注册 · 原始渲染序列 · GT 高分辨率序列
  IN  质量反馈 (来自 xuanjing-eval，可选)
  OUT 模型权重包
      (weights · version · backend compat · checksum)
  │
  ▼
xuanjing-tensor
  IN  模型权重包
  OUT 已加载模型，供推理路径调用
```

---

## 7. 模块依赖矩阵（Hard / Soft）

```
xuanjing-platform   ← 无依赖（根节点）
xuanjing-compiler   ← platform
xuanjing-tensor     ← platform, compiler
xuanjing-temporal   ← (soft) platform
xuanjing-upscale    ← temporal, (soft) tensor/compiler
xuanjing-shader     ← platform
xuanjing-genframe   ← temporal, tensor, shader
xuanjing-eval       ← 无硬依赖（挂载 runtime 输出）
xuanjing-model      ← (soft) tensor, eval
xuanjing-runtime    ← platform, temporal, upscale, shader, (soft) eval
```

---

## 8. 关键决策记录

| 决策 | 选择 | 理由 |
|---|---|---|
| 图形 IR 格式 | SPIR-V 主路，DXIL 兼容规划 | 跨 Vendor 覆盖最广；Vulkan/OpenCL/Metal 均可消费 SPIR-V |
| IR 所有权 | 独立 xuanjing-compiler 模块 | 算法模块不感知 vendor；backend 可独立替换 |
| 后端选择策略 | Runtime 询问 platform，compiler/tensor 执行 | 单一能力来源，避免 vendor 逻辑散落 |
| 帧生成 fallback | 置信度门 + 自动降级 | 场景切换/低质量运动向量时保稳定性优先 |
| CPU Reference Backend | 始终存在 | 用于正确性对拍和 CI 无 GPU 环境 |
| 训练与推理解耦 | train 输出模型包，tensor 加载 | 允许独立迭代模型而不改推理路径 |
