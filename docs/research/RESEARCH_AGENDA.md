# Research Agenda

## Target Venues

Primary:
- SIGGRAPH / SIGGRAPH Asia (Real-time rendering, video upscaling)
- ACM TOG (Transactions on Graphics)
- IEEE TPAMI (Transactions on Pattern Analysis and Machine Intelligence)
- IEEE TIP (Transactions on Image Processing)

Secondary:
- CVPR / ICCV / ECCV (video SR, temporal consistency)
- AAAI / NeurIPS (efficient neural inference)

---

## Core Research Directions

### Direction 1 — Temporally Stable Super-Resolution

Hypothesis: Combining confidence-aware history rejection with a lightweight diffusion refinement stage
achieves state-of-the-art temporal stability without increasing latency beyond acceptable real-time budgets.

Open Problems:
- Existing methods (DLSS 3, XeSS, FSR 3) trade short-term sharpness for temporal ghosting.
- No open benchmark for real-time temporal consistency in gaming workloads.

Claimed Novelty:
- Per-pixel history trust map predicted jointly with the upscale pass.
- Scene-adaptive rejection threshold conditioned on motion magnitude and depth discontinuity.

Baselines:
- Bicubic, ESRGAN, EDSR (image SR)
- EDVR, BasicVSR, FastDVD (video SR)
- DLSS 2/3 (commercial, black-box — use perceptual proxy metrics)
- FSR 2 (open, reproducible baseline)

---

### Direction 2 — Low-Latency Frame Generation

Hypothesis: A two-stage architecture (coarse warping + residual synthesis) achieves competitive quality
to single-pass frame interpolation at 40% fewer FLOPs.

Open Problems:
- Current frame generation methods fail on fast camera pans and large disocclusions.
- No public dataset for real-time frame synthesis evaluation.

Claimed Novelty:
- Occlusion-aware residual frame synthesis network trained on game-engine captured sequences.
- Confidence-gated enable/disable preventing artifacts on scene cuts (zero false-positive rate target).

Baselines:
- DAIN, RIFE, IFRNet (video frame interpolation)
- AMT (All-Pairs Multi-Field Transformer)

---

### Direction 3 — Efficient NPU/GPU Inference for Real-Time SR

Hypothesis: Model quantization co-designed with the image-space processing pipeline reduces
inference latency by 2× at equivalent perceptual quality compared to FP16 baseline.

Open Problems:
- Quantization-aware training for spatially varying quality requirements is under-explored.
- No public benchmark for end-to-end latency on domestic NPU architectures.

Claimed Novelty:
- Mixed-precision schedule determined online by spatial complexity map.
- Hardware-specific kernel fusion validated on xuanjing-tensor backend.

---

## Publication Roadmap

| Paper | Target Venue | Depends On | Estimated Submission |
|---|---|---|---|
| P1: Temporally Stable SR | SIGGRAPH 2027 | xuanjing-temporal, xuanjing-upscale, xuanjing-eval | Q4 2026 |
| P2: Low-Latency Frame Generation | CVPR 2027 | xuanjing-genframe, xuanjing-temporal | Q3 2026 |
| P3: Efficient NPU SR Inference | IEEE TIP 2027 | xuanjing-tensor, xuanjing-train | Q1 2027 |

---

## Reproducibility Commitments

- All published results must be reproducible from this repository within 24 hours on a reference platform.
- Dataset links, random seeds, training configs, and model checkpoints must be archived at paper submission.
- Inference code must compile and run with `cmake --preset release` with no additional steps.
