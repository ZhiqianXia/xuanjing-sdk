# Module Development Goals & Acceptance Criteria (Initial)

## Purpose

This document defines initial development goals and acceptance criteria for each module before formal implementation.

Alignment references:
- docs/MILESTONES_M1_M2_M3.md
- docs/specs/io_spec.md

## Global Baseline (applies to all modules)

1. Build gate
- Module builds successfully under dev and release presets.
- No new compiler warnings in public headers.

2. API gate
- Public header compiles standalone.
- API behavior is deterministic for identical inputs.

3. Quality gate
- Unit tests cover normal path and at least 2 failure paths.
- Error handling returns explicit status/result, no silent failure.

4. CI gate
- Related jobs pass in CI: build, lint, asan (if enabled for the module).

5. Documentation gate
- README and header comments are updated for any API/behavior change.

## Development Strategy (Recommended)

Decision: build a minimal integration trunk first, then develop sub-modules in parallel around that trunk.

Current mode (solo developer)
1. Prioritize end-to-end input/output path before deep module optimization.
2. Prefer fast feedback loops over broad parallel work.
3. Optimize one bottleneck at a time after baseline is measurable.

Why this order
1. Early integration exposes interface and data-contract mismatches sooner.
2. Sub-module work can proceed in parallel with stable contracts and stub fallbacks.
3. CI can validate end-to-end health continuously instead of discovering breakage late.

Execution sequence
1. Step 1 - Minimal integration trunk
- runtime + platform + temporal + upscale + shader + eval (stub or baseline behavior accepted).
- Goal: one end-to-end frame path executes without crash, with clearly observable input/output artifacts.

2. Step 2 - Core M1 module hardening
- temporal, upscale, shader, runtime.
- Goal: improve quality/stability while preserving trunk API contract.

3. Step 3 - Capability and acceleration
- tensor + platform backend paths.
- Goal: backend selection, fallback, and performance uplift.

4. Step 4 - Advanced features
- genframe + eval extensions.
- Goal: frame generation and richer metrics without destabilizing M1 path.

5. Step 5 - Training loop and model iteration
- train + tensor packaging alignment.
- Goal: repeatable model update pipeline and compatibility checks.

Practical solo loop
1. Freeze minimal I/O contract.
2. Implement E2E path quickly with placeholders.
3. Add measurement hooks (quality + frame time).
4. Optimize highest-impact module only.
5. Repeat with latest paper-backed ideas.

Release rule
- No sub-module feature merges to mainline unless trunk integration tests remain green.

## Module Dependency Definition

Dependency legend
- Hard dependency: required for module to function in production path.
- Soft dependency: optional at runtime, but used for quality/performance uplift.

Top-level dependency graph
- xuanjing-runtime -> xuanjing-platform, xuanjing-temporal, xuanjing-upscale, xuanjing-shader, xuanjing-eval
- xuanjing-upscale -> xuanjing-temporal, xuanjing-tensor (soft)
- xuanjing-genframe -> xuanjing-temporal, xuanjing-tensor, xuanjing-shader
- xuanjing-temporal -> xuanjing-platform (soft)
- xuanjing-tensor -> xuanjing-platform
- xuanjing-shader -> xuanjing-platform
- xuanjing-eval -> xuanjing-runtime output path
- xuanjing-model -> xuanjing-tensor (model format/graph alignment), xuanjing-eval (metric alignment)

Per-module dependency table

| Module | Hard Depends On | Soft Depends On | Provides To |
|---|---|---|---|
| xuanjing-runtime | platform, temporal, upscale, shader | eval | all engine entrypoints |
| xuanjing-platform | none | none | runtime, tensor, shader, temporal |
| xuanjing-temporal | none | platform | upscale, genframe |
| xuanjing-upscale | temporal | tensor | runtime, shader |
| xuanjing-tensor | platform | none | upscale, genframe, train |
| xuanjing-shader | platform | none | runtime final output, genframe post-process |
| xuanjing-genframe | temporal, tensor | shader | runtime frame interpolation path |
| xuanjing-eval | none | runtime output hooks | runtime, train, CI reports |
| xuanjing-model | none | tensor, eval | tensor model package consumers |

Dependency management rules
1. No circular hard dependencies are allowed.
2. Cross-module API changes require updates in both io_spec and this dependency section.
3. Soft dependencies must always have a documented fallback path.
4. Runtime is orchestration-only and must not absorb algorithm logic from sub-modules.

## Topological Development Schedule (Draft)

Assumption: solo-first execution. Use dependency-ordered stages as primary plan; week mapping is a reference, not a hard commitment.

Topological waves
1. Wave 0 (foundation)
- xuanjing-platform
- xuanjing-eval (baseline report skeleton)

2. Wave 1 (core path)
- xuanjing-temporal
- xuanjing-shader
- xuanjing-runtime

3. Wave 2 (quality/performance)
- xuanjing-upscale
- xuanjing-tensor

4. Wave 3 (advanced feature)
- xuanjing-genframe

5. Wave 4 (model lifecycle)
- xuanjing-model

Week-by-week schedule

| Week | Primary Focus | Modules | Entry Criteria | Exit Criteria |
|---|---|---|---|---|
| W1 | Contract freeze v0 | platform, runtime, temporal, upscale, shader | io_spec reviewed | API signatures + buffer contracts frozen and documented |
| W2 | Minimal trunk bring-up | runtime, platform, temporal, shader, eval | W1 contracts frozen | end-to-end pipeline runs on stub data without crash |
| W3 | Temporal hardening | temporal, runtime, eval | W2 trunk green in CI | reprojection/validity baseline tests pass |
| W4 | Upscale baseline | upscale, temporal, shader | W3 temporal outputs stable | preset routing + output contract tests pass |
| W5 | Backend acceleration path | tensor, platform, upscale | W4 upscale path stable | backend select/fallback tests pass |
| W6 | Frame generation v0 | genframe, temporal, tensor, shader | W5 tensor path integrated | confidence-gated FG path runs with fallback |
| W7 | Training skeleton + packaging | train, tensor, eval | W5 backend metadata stable | dataset register + model metadata validation pass |
| W8 | Integration hardening + release candidate | all M1 modules | W2-W7 module gates green | integration/perf smoke tests green; M1 candidate tag ready |

Stage-oriented fallback (recommended for solo execution)
1. Stage A - E2E I/O visibility
- Deliverable: deterministic input capture and output dump/visualization pipeline.

Stage A detailed task checklist

Task A1 - Freeze minimal I/O contract v0
- Scope: low-res color, motion vectors, depth, jitter, exposure, history, output color.
- Output artifact: io_spec update with tensor/buffer shape, format, coordinate convention, and frame index semantics.
- Done when: all required fields are marked mandatory/optional and example payload is documented.

Task A2 - Create deterministic test fixture set
- Scope: static frame, camera pan, fast motion, scene cut.
- Output artifact: reproducible fixture manifest and fixed-seed generation notes.
- Done when: rerun produces identical checksums for fixture inputs.

Task A3 - Build minimal pipeline runner (stub-friendly)
- Scope: runtime -> temporal -> upscale -> shader path with current placeholders.
- Output artifact: one CLI/sample entry that consumes fixture input and writes output.
- Done when: command runs end-to-end without crash for all Stage A fixtures.

Task A4 - Implement unified frame dump utility
- Scope: dump selected intermediate buffers and final output each frame.
- Output artifact: deterministic file naming convention with frame_id + stage name.
- Done when: at least 5 key buffers are dumpable on demand.

Task A5 - Add quick visualization path
- Scope: convert dumped buffers into viewable images or a lightweight HTML viewer.
- Output artifact: before/after and stage-by-stage visual inspection outputs.
- Done when: developer can visually confirm input/output alignment in < 2 minutes.

Task A6 - Add E2E sanity validators
- Scope: resolution checks, NaN/Inf checks, range checks, format consistency checks.
- Output artifact: validation report (pass/fail + reason) per run.
- Done when: malformed input fails fast with explicit diagnostics.

Task A7 - Add baseline timing probes
- Scope: stage-level timing for runtime, temporal, upscale, shader.
- Output artifact: per-frame timing CSV/JSON summary.
- Done when: timing output is stable across repeated runs and consumable by scripts.

Task A8 - Add Stage A smoke tests to CI
- Scope: one deterministic E2E smoke case + one malformed-input negative case.
- Output artifact: CI job output showing E2E execution and validator status.
- Done when: CI can detect contract breakage from any module change.

Task A9 - Define Stage A debugging playbook
- Scope: known failure signatures and first-response checks.
- Output artifact: concise troubleshooting section linked from docs.
- Done when: common failures can be triaged in < 15 minutes.

Task A10 - Stage A checkpoint review
- Scope: verify all above artifacts and decide Stage B entry.
- Output artifact: one checkpoint note with metrics snapshot and open issues.
- Done when: Stage B entry criteria are explicitly satisfied.

Stage A entry criteria
1. io_spec baseline exists and is reviewed.
2. current modules build in dev preset.
3. at least one callable pipeline entry exists.

Stage A exit criteria
1. Deterministic fixture -> deterministic output path is proven.
2. Intermediate + final outputs are visible and inspectable.
3. E2E sanity validators pass for nominal inputs and fail correctly for malformed inputs.
4. CI contains at least one Stage A smoke test.

Stage A anti-goals (avoid during this phase)
1. Do not chase peak quality/performance yet.
2. Do not introduce large architectural refactors.
3. Do not optimize multiple modules in parallel before baseline visibility is stable.

2. Stage B - Baseline quality/performance instrumentation
- Deliverable: reproducible PSNR/SSIM/frame-time baseline report.

3. Stage C - Module-by-module optimization by impact
- Deliverable: each iteration improves one bottleneck with before/after metrics.

4. Stage D - Research-driven upgrades
- Deliverable: paper-inspired changes behind feature flags with regression checks.

Critical path
1. platform -> runtime -> temporal -> upscale -> shader -> integration test green.
2. platform -> tensor -> upscale/genframe acceleration.
3. tensor -> train model package compatibility.

Parallelization rules
1. eval can progress in parallel from W1 and continuously attach to outputs.
2. genframe starts only after temporal signal quality and tensor runtime are stable.
3. train should not block M1 release; it is gated by model package contract only.

Schedule governance
1. Weekly gate review each Friday: contract breakages, CI status, perf drift.
2. No wave promotion unless previous wave exit criteria are met.
3. If critical path slips by > 1 week, de-scope non-critical soft-dependency features first.

## Long-Term Ecosystem Goal

Vision
- Build an LLVM-like open technical community around graphics, GPU, compilers, and algorithms, serving both experimentation and production engineering.

Ecosystem principles
1. Modular architecture with stable public contracts and replaceable implementations.
2. Research-to-engineering path: every major optimization has reproducible metrics and fallback.
3. Open contribution workflow: design docs, RFCs, benchmarks, and CI as merge gates.
4. Multi-discipline collaboration: rendering, compiler/runtime, kernel optimization, and ML jointly evolve.

Near-term actions toward the vision
1. Keep module boundaries strict and documented.
2. Add benchmark corpus and reproducibility scripts early.
3. Introduce RFC template before major cross-module changes.
4. Version APIs and model formats from the start.

## Module Plans

## xuanjing-runtime

Initial goals
1. Provide stable RuntimeContext initialization and lifecycle management.
2. Implement orchestration entry that dispatches temporal -> upscale -> shader in correct order.
3. Add backend and capability handoff from xuanjing-platform.

Acceptance criteria
1. Initialize(RuntimeContext&) returns success on supported device and failure with reason on unsupported device.
2. Pipeline dispatch produces identical output metadata across repeated runs of same input.
3. End-to-end call path integrates at least temporal + upscale + shader stubs without crash.
4. Runtime unit tests cover context creation, invalid device id, and repeated init/shutdown.

## xuanjing-upscale

Initial goals
1. Implement quality preset routing: kQuality, kBalanced, kPerformance.
2. Build a baseline super-resolution path consuming low-res color + temporal signals.
3. Expose runtime-tunable parameters needed by engine integration.

Acceptance criteria
1. RunUpscale(QualityPreset) supports all enum values and rejects invalid preset mapping.
2. Output resolution matches requested upscale ratio and output format contract.
3. On benchmark scenes, quality preset changes produce expected cost/quality ordering.
4. Unit tests cover preset routing, invalid input dimensions, and deterministic output checksum in fixed seed mode.

## xuanjing-genframe

Initial goals
1. Implement frame generation entry with confidence gate behavior.
2. Support fallback to non-generated frame when confidence is low.
3. Expose gating threshold as configurable runtime parameter.

Acceptance criteria
1. GenerateFrame(const FrameGenConfig&) returns generated frame only when gate condition passes.
2. Scene-cut and invalid motion-vector cases trigger fallback path.
3. Confidence gate toggle changes behavior in controlled test clip as expected.
4. Tests cover gate on/off, low-confidence fallback, and config boundary values.

## xuanjing-temporal

Initial goals
1. Implement history reprojection from motion vectors and depth.
2. Add history validity mask and rejection strategy for disocclusion.
3. Provide outputs required by upscale module (aligned history + confidence).

Acceptance criteria
1. ReprojectHistory() handles valid and invalid motion vectors without artifact explosion.
2. Disocclusion regions are rejected according to documented threshold policy.
3. Temporal output tensor/buffer layout matches io_spec contract.
4. Tests include static camera, fast pan, and scene-cut clips with expected validity behavior.

## xuanjing-tensor

Initial goals
1. Implement backend selection path for GPU/NPU/CPU.
2. Provide baseline inference dispatch and graph execution wrapper.
3. Add backend fallback policy when target backend is unavailable.

Acceptance criteria
1. RunInference(Backend) executes on requested backend when available.
2. If backend is unavailable, fallback policy is triggered and logged.
3. Inference result shape/type is validated before returning success.
4. Tests cover backend available/unavailable matrix and invalid model/runtime state.

## xuanjing-shader

Initial goals
1. Bootstrap shader pipeline and required resource binding.
2. Implement post-process chain entry for color-space and final compose.
3. Support UI compositing rule from io_spec (3D content first, UI after SDK pass).

Acceptance criteria
1. BootstrapShaderPipeline() succeeds with supported formats and fails fast with explicit reason otherwise.
2. Shader pass order is validated by integration tests and debug markers.
3. Final output meets expected color-space metadata in SDR/HDR test modes.
4. Tests cover missing resource bindings, unsupported format, and normal rendering path.

## xuanjing-eval

Initial goals
1. Provide baseline quality score API and runtime metric collection.
2. Add metric adapters for PSNR/SSIM/LPIPS placeholders according to research plan.
3. Emit structured evaluation report for CI/perf tracking.

Acceptance criteria
1. ComputeQualityScore() returns reproducible value for fixed input pair.
2. Metric pipeline handles missing reference input with explicit error code.
3. Evaluation report schema is stable and parseable by tools.
4. Tests cover nominal metric computation, missing GT frame, and invalid resolution mismatch.

## xuanjing-model

Initial goals
1. Implement dataset registration with validation and dedup behavior.
2. Define model package metadata contract for export/import.
3. Build minimal train-loop skeleton that can be invoked in CI dry-run mode.

Acceptance criteria
1. RegisterDataset(const char*) rejects null/empty path and accepts valid dataset root.
2. Duplicate dataset registration is idempotent and traceable in logs.
3. Exported model metadata contains version, backend compatibility, and checksum fields.
4. Tests cover invalid path, duplicate registration, and metadata schema validation.

## xuanjing-platform

Initial goals
1. Implement platform capability probing and normalized capability descriptor.
2. Provide driver interop initialization hooks for runtime/tensor/shader modules.
3. Add capability cache to avoid repeated expensive probing.

Acceptance criteria
1. ProbeCapabilities() returns deterministic descriptor for same device/driver state.
2. Unsupported capability combinations are reported with actionable diagnostics.
3. Capability descriptor fields satisfy io_spec requirements (format support, MV precision, etc.).
4. Tests cover supported device, partially supported device, and unsupported device paths.

## Cross-Module Integration Acceptance (Initial)

1. Basic pipeline integration
- runtime + temporal + upscale + shader executes end-to-end in sample app without crash.

2. Data contract validation
- Buffer format, resolution, motion-vector conventions follow docs/specs/io_spec.md.

3. Performance sanity
- Debug build runs complete pipeline in test scene with stable frame progression and no deadlock.

4. Regression baseline
- At least one integration test and one perf smoke test are present and runnable in CI.

## Definition of Done for Initial Development Phase

A module is considered initially complete when all conditions below are true:
1. Its module-level acceptance criteria pass.
2. Global baseline gates pass.
3. Required documentation is updated.
4. No blocking known issues remain for M1 scope integration.