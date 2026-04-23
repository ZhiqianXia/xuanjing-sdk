# Experiment Protocol

This document defines mandatory procedures for all experiments reported in publications.
All experiments must follow this protocol before results can be included in a paper draft.

---

## 1. Environment Specification

Every experiment log must record:
- Host OS, kernel version
- GPU/NPU model, driver version, firmware version
- CUDA/ROCm/vendor SDK version
- CMake preset used
- Git commit SHA (must be a tagged release or a clean checkout)
- Python version and `requirements.txt` hash (for training runs)

---

## 2. Datasets

### 2.1 Super-Resolution Evaluation
| Dataset | Resolution | Usage |
|---|---|---|
| Vimeo-90K | Various | Standard video SR benchmark |
| Vid4 | 480p | Classical temporal SR benchmark |
| REDS | 1080p | Motion blur / fast motion |
| Xuanjing-GameSeq (internal) | 1080p→4K | Game-engine rendered, ground-truth available |

### 2.2 Frame Generation Evaluation
| Dataset | FPS | Usage |
|---|---|---|
| GoPro | 240fps→60fps | Occlusion, motion blur |
| Middlebury | Various | Standard interpolation benchmark |
| Xuanjing-GameFG (internal) | 30fps→120fps | Game-specific fast pans, scene cuts |

### 2.3 Data Versioning
- All dataset splits are frozen in `tools/dataset_splits/` as SHA-256 manifests.
- Any split change requires a new version tag and a changelog entry.

---

## 3. Metrics

### Quantitative (mandatory for all image/video quality claims)
- PSNR (dB) — Y channel, YCbCr
- SSIM
- LPIPS (VGG backbone, version must be recorded)
- tLPIPS — temporal LPIPS (frame-to-frame consistency)
- tOF — temporal optical flow consistency score

### Performance (mandatory for all real-time/latency claims)
- GPU/NPU inference time (ms) — P50, P95, P99
- End-to-end latency (ms) — input submission to output ready
- Peak VRAM usage (MB)
- FLOPs and parameter count

### Per-artifact (for any claim about specific failure modes)
- Ghosting rate: fraction of frames with tLPIPS > threshold in fast-motion clips
- Scene-cut artifact rate: fraction of scene-cut transitions with visible artifact

---

## 4. Ablation Study Requirements

Each paper must include ablation covering:
1. Proposed component removed vs. present (isolate each novel contribution)
2. Comparison to nearest open-source baseline at matched FLOPs budget
3. Quality-latency Pareto curve across at least 3 operating points

Ablation results must be reproducible from a single config file under `xuanjing-model/configs/ablations/`.

---

## 5. Statistical Rigor

- Report mean ± standard deviation over at least 3 independent runs.
- For human perceptual studies: minimum 20 observers, MOS protocol (ITU-T P.910).
- Use Wilcoxon signed-rank test when claiming statistical superiority.

---

## 6. Experiment Tracking

All training and evaluation runs are tracked with a structured log under `xuanjing-model/runs/`.
Required fields per run:
```
run_id:       <uuid>
date:         <ISO 8601>
git_sha:      <full sha>
config_file:  <relative path>
dataset:      <name + version>
metrics:      <dict>
hardware:     <dict>
notes:        <free text>
```

---

## 7. Model Release Checklist

Before submitting a paper or releasing a model:
- [ ] All eval metrics re-run on reference hardware and match paper tables
- [ ] Inference code compiles with `cmake --preset release`
- [ ] Model weights archived with SHA-256 checksum
- [ ] Config file commited and tagged
- [ ] CHANGELOG.md updated with model version entry
