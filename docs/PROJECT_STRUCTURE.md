# Project Structure

This repository is organized as a module-based monorepo.

## Modules

- modules/xuanjing-runtime: Runtime API, resource scheduling, backend selection.
- modules/xuanjing-upscale: Super-resolution reconstruction pipeline.
- modules/xuanjing-genframe: Frame generation and synthesis logic.
- modules/xuanjing-temporal: Temporal buffers, reprojection, motion-vector handling.
- modules/xuanjing-tensor: NPU/GPU inference kernels and graph runtime.
- modules/xuanjing-shader: Rasterization and post-processing shader stack.
- modules/xuanjing-eval: Objective and subjective quality/performance evaluation.
- modules/xuanjing-train: Data, training, distillation, model packaging.
- modules/xuanjing-platform: Driver/platform adaptation and capability abstraction.

## Shared Directories

- docs/architecture: Architecture notes and diagrams.
- docs/specs: Module-level technical specifications.
- samples: Integration demos and reference apps.
- tools: Build, benchmark, and utility scripts.
- tests/integration: Cross-module integration tests.
- tests/perf: Performance and regression tests.
