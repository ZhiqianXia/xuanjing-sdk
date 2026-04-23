# Changelog

All notable changes to this project will be documented in this file.

The format is based on Keep a Changelog,
and this project follows Semantic Versioning.

## [Unreleased]

---

### 2026-04-23 — Industrial + Research Uplift

#### Added
- `.clang-format`: Google-style C++ formatting config (column limit 100).
- `.clang-tidy`: Static analysis with bugprone, cppcoreguidelines, modernize, performance rules; bugprone and clang-analyzer checks are treated as errors.
- `CMakePresets.json`: Four build presets — dev, release, asan, coverage.
- `cmake/Sanitizers.cmake`: ASan / UBSan / TSan target-level CMake helpers.
- `cmake/Coverage.cmake`: gcov/lcov coverage instrumentation CMake helper.
- `cmake/CompilerWarnings.cmake`: Strict warning flag set with `-Werror`.
- `tools/lint.sh`: clang-format dry-run check + clang-tidy batch runner.
- `tools/coverage.sh`: Coverage build, ctest execution, and lcov HTML report generation.
- `.github/workflows/ci.yml`: CI pipeline — build+test (dev, asan), lint, coverage, changelog-check jobs.
- `docs/CODING_STANDARDS.md`: C++ naming conventions, file layout rules, API design rules, memory safety policy.
- `docs/research/RESEARCH_AGENDA.md`: Three research directions, target venues (SIGGRAPH/TOG/TPAMI/TIP/CVPR), baseline comparison lists, paper roadmap (P1 SIGGRAPH 2027, P2 CVPR 2027, P3 IEEE TIP 2027).
- `docs/research/EXPERIMENT_PROTOCOL.md`: Experiment environment spec, dataset versioning, mandatory metrics (PSNR/SSIM/LPIPS/tLPIPS/tOF), ablation design requirements, statistical rigor rules, run tracking schema, model release checklist.

#### Changed
- `CMakeLists.txt`: Added `cmake/` module path; included Sanitizers, Coverage, CompilerWarnings helpers.
- `CMakePresets.json`: Reformatted by code formatter (buildPresets inline objects expanded to multi-line; no semantic change).
- `docs/MILESTONES_M1_M2_M3.md`: Added Engineering Gates and Research Gates to all three milestones; added header links to research docs.

---

### 2026-04-23 — Project Scaffold

#### Added
- Initialized repository structure for the Xuanjing graphics upscaling and frame generation stack.
- Added module directories: xuanjing-runtime, xuanjing-upscale, xuanjing-genframe, xuanjing-temporal, xuanjing-tensor, xuanjing-shader, xuanjing-eval, xuanjing-train, xuanjing-platform.
- Added supporting directories: docs/architecture, docs/specs, samples, tools, tests/integration, tests/perf.
- Added per-module minimal scaffolding (README.md, CMakeLists.txt, include header placeholder, src stub).
- Added top-level CMake build system skeleton.
- Added root README.md with bootstrap build instructions and changelog discipline note.
- Added `docs/MILESTONES_M1_M2_M3.md`: M1/M2/M3 milestone plan.
- Added `docs/CHANGELOG_POLICY.md`: changelog process policy.
- Added `docs/specs/io_spec.md`: per-module I/O table, required engine-side inputs, and UI compositing constraint.

#### Changed
- Root README.md updated to require changelog updates for every repository change.

#### Fixed
- Removed duplicated declarations in module interface header placeholders introduced during scaffold generation recovery.
