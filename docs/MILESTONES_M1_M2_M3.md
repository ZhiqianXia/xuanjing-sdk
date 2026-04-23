# Milestones (M1/M2/M3)

> This document tracks both **industrial SDK delivery** and **research publication** goals in parallel.
> See docs/research/RESEARCH_AGENDA.md for paper-specific targets.
> See docs/research/EXPERIMENT_PROTOCOL.md for experiment and reproducibility requirements.

---

## M1 - Usable Baseline (Temporal + Upscale)

Goal: Deliver stable temporal reconstruction and super-resolution in real-time scenes.

Scope:
- xuanjing-temporal: Motion-vector validation, reprojection, history rejection.
- xuanjing-upscale: Core SR path with quality/performance presets.
- xuanjing-runtime: Basic API and pipeline orchestration.
- xuanjing-shader: Pre/post process chain required for SR.
- xuanjing-eval: Baseline quality and performance metrics.

Exit Criteria:
- Stable 60 FPS in target demo scenes.
- Visible quality gain over native low-resolution rendering.
- Major ghosting/flicker cases controlled in common camera motions.

Engineering Gates (M1):
- All modules build clean under `cmake --preset dev` with `-Werror`.
- CI pipeline passing: build, asan, lint, changelog-check jobs.
- clang-format and clang-tidy clean on all public headers.
- Unit test coverage ≥ 60% for xuanjing-temporal and xuanjing-upscale.

Research Gates (M1):
- PSNR/SSIM/LPIPS/tLPIPS evaluated on Vimeo-90K and Vid4 per EXPERIMENT_PROTOCOL.md.
- FSR 2 open-source baseline comparison completed and results logged.
- Ablation: history rejection ON vs. OFF — quantified quality delta.
- Draft for P1 (Temporally Stable SR) introduction and related work sections.

## M2 - Enhanced Experience (Frame Generation)

Goal: Add frame generation with controllable latency and robust fallback.

Scope:
- xuanjing-genframe: Frame synthesis and confidence gating.
- xuanjing-temporal: Occlusion handling improvements for interpolation.
- xuanjing-tensor: Runtime acceleration path for inference kernels.
- xuanjing-platform: Capability detection and backend fallback.
- xuanjing-eval: Latency/jitter/visual-artifact tracking for FG path.

Exit Criteria:
- High-refresh scenarios show clear smoothness uplift.
- End-to-end latency remains within agreed budget.
- Automatic fallback works for scene cuts/unstable vectors.

Engineering Gates (M2):
- ASan clean on full pipeline end-to-end test.
- Coverage ≥ 70% across temporal + upscale + genframe modules.
- Performance regression suite running in CI (P95 frame time tracked per commit).
- ABI versioning policy enforced (no silent ABI breaks after M1 tag).

Research Gates (M2):
- Frame generation eval on GoPro and Xuanjing-GameFG datasets.
- Comparison to RIFE and IFRNet at matched latency budget.
- Ablation: coarse warp + residual synthesis vs. single-pass.
- P2 (Frame Generation) technical sections drafted; submission target confirmed.

## M3 - Productization (Multi-Platform + Tooling)

Goal: Scale from demo pipeline to production-ready SDK delivery.

Scope:
- Multi-platform adaptation and driver cooperation paths.
- Model/data/training iteration loop for sustained quality upgrades.
- Integration packages and sample apps for engine teams.
- CI-driven integration/perf regression suite.

Exit Criteria:
- Cross-title integration guide available and verified.
- Automated quality/perf regression baseline established.
- Release process and versioning policy in place.

Engineering Gates (M3):
- Full release build passes on ≥ 2 domestic GPU/NPU platforms.
- Integration test suite covers ≥ 3 sample scenes end-to-end.
- Public API documented with Doxygen; docs published.
- SBOM (Software Bill of Materials) generated for each release tag.

Research Gates (M3):
- All three papers (P1/P2/P3) submitted or conditionally accepted.
- Reproducibility checkpoint: fresh-clone-to-results in < 24 h on reference platform.
- Model weights and configs archived with DOI (Zenodo or equivalent).
- Internal benchmark results independently verified by a second researcher.

