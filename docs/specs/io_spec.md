# I/O Specification

## Overview

```
Game Engine / Renderer
      │
      ▼ Input
┌─────────────────────────────────────┐
│  Low-resolution color buffer         │
│  Motion vectors (per-pixel MV)       │
│  Depth buffer                        │
│  Jitter offset / exposure params     │
│  Temporal history (previous frame)   │
└─────────────────────────────────────┘
      │
   [Xuanjing SDK Pipeline]
      │
      ▼ Output
┌─────────────────────────────────────┐
│  High-resolution upscaled color      │
│  Optional: generated interpolated frame │
└─────────────────────────────────────┘
      │
      ▼
   Composite / Display
```

## Per-Module I/O

| Module | Input | Output |
|---|---|---|
| xuanjing-temporal | Current frame MV, depth, history frame | Aligned history frame, occlusion mask, validity confidence map |
| xuanjing-upscale | Low-res color, aligned history, confidence map, quality preset | High-resolution reconstructed frame |
| xuanjing-genframe | Previous/current high-res frames, MV, depth, exposure | Synthesized intermediate interpolated frame |
| xuanjing-tensor | Inference graph, weights, low-precision input tensor | Inference result tensor |
| xuanjing-shader | Intermediate textures, color space params | Final display-ready HDR/SDR buffer |
| xuanjing-runtime | Engine API calls | Scheduled task queue dispatched to sub-modules |
| xuanjing-platform | Driver/device query | Hardware capability descriptor (supported formats, MV precision, etc.) |
| xuanjing-eval | Output frame + ground-truth reference frame | PSNR, SSIM, LPIPS, frame time, flicker rate |
| xuanjing-train | Raw render sequences, GT high-res sequences | Trained model weight package |

## Required Engine-Side Inputs

The following MUST be provided by the calling engine/renderer:

1. **Motion vectors** — per-pixel MV in screen space. Prerequisite for correct temporal reconstruction. Without MV, temporal accumulation degrades to single-frame super-resolution with significant quality reduction.
2. **Jitter offset** — sub-pixel camera jitter parameters used to accumulate high-frequency detail across frames.

## UI Compositing Constraint

HUD and UI layers (no MV, no depth) must be separated from the 3D scene before SDK processing and composited after the upscale/shader pass via xuanjing-shader. Failing to do so causes UI elements to be incorrectly reconstructed, producing ghosting and trailing artifacts.
