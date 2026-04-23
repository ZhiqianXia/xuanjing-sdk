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
| xuanjing-model | Raw render sequences, GT high-res sequences | Trained model weight package |

## Required Engine-Side Inputs

The following MUST be provided by the calling engine/renderer:

1. **Motion vectors** — per-pixel MV in screen space. Prerequisite for correct temporal reconstruction. Without MV, temporal accumulation degrades to single-frame super-resolution with significant quality reduction.
2. **Jitter offset** — sub-pixel camera jitter parameters used to accumulate high-frequency detail across frames.

## Stage A Static Replay Contract

Purpose:
- Use deterministic offline data to connect runtime, temporal, upscale, shader, and eval before any API-specific integration.
- Validate data contracts, scheduling, history lifecycle, and output visibility with fixed inputs.

Recommended progression:
1. Single-frame static input: validate file I/O and final output.
2. Repeated static sequence (4 frames): validate history lifecycle with zero motion.
3. Simple translated sequence: validate motion-vector semantics before real renderer hookup.

### Minimal required fields

| Field | Required | First-stage value | Notes |
|---|---|---|---|
| `low_res_color` | yes | one static image | 3D scene color only; UI excluded |
| `motion_vectors` | yes | all-zero image | same resolution as `low_res_color`; 2-channel float |
| `depth` | yes | constant image | same resolution as `low_res_color`; single-channel float |
| `frame_index` | yes | `0..N-1` | monotonic increment for replay ordering |
| `output_width` | yes | explicit target width | do not infer only from scale factor |
| `output_height` | yes | explicit target height | do not infer only from scale factor |
| `jitter_x`, `jitter_y` | yes | `0.0` | static replay keeps jitter off initially |
| `exposure` | yes | `1.0` | must stay positive |
| `has_prev_history` | yes | `false` on frame 0, `true` after | runtime owns actual history buffer |
| `ui_layer` | no | omitted | reserved from day one to preserve semantics |

### Semantic rules to freeze now

1. `low_res_color`
- Semantic: low-resolution scene color before HUD/UI composition.
- First-stage default: `RGBA8` or `RGBA16F`; pick one and keep it stable in the replay corpus.

2. `motion_vectors`
- Semantic: per-pixel screen-space motion vectors.
- First-stage default: all-zero field for static scenes.
- Follow-up task: freeze exact coordinate convention before translated sequence fixtures are added.

3. `depth`
- Semantic: primary scene depth aligned with `low_res_color`.
- First-stage default: constant single-channel float image.
- Follow-up task: freeze whether this is linear depth or device depth before real renderer hookup.

4. `prev_history`
- Semantic: previous-frame high-resolution output.
- Ownership: runtime-managed; offline fixtures only provide the `has_prev_history` state.

5. `ui_layer`
- Semantic: separate post-scene overlay path.
- First-stage default: omitted.
- Constraint: never bake UI into `low_res_color`.

### Suggested offline fixture layout

```text
samples/offline_replay/input/
      frame_0000_color.png
      frame_0000_mv.exr
      frame_0000_depth.exr
      frame_0000_meta.json
      frame_0001_color.png
      frame_0001_mv.exr
      frame_0001_depth.exr
      frame_0001_meta.json
```

Suggested `meta.json` fields:

```json
{
      "frame_index": 0,
      "output_width": 3840,
      "output_height": 2160,
      "jitter_x": 0.0,
      "jitter_y": 0.0,
      "exposure": 1.0,
      "has_prev_history": false
}
```

### Stage A validation rules

1. `low_res_color`, `motion_vectors`, and `depth` must all exist and share the same input resolution.
2. `output_width` and `output_height` must be explicit and non-zero.
3. `exposure` must be positive.
4. Frame 0 must set `has_prev_history=false`.
5. Repeated static sequence must produce deterministic output and stable debug artifacts across reruns.

## UI Compositing Constraint

HUD and UI layers (no MV, no depth) must be separated from the 3D scene before SDK processing and composited after the upscale/shader pass via xuanjing-shader. Failing to do so causes UI elements to be incorrectly reconstructed, producing ghosting and trailing artifacts.
