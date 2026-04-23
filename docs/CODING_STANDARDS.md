# C++ Coding Standards

## Style

- Formatting is enforced by `.clang-format` (Google Style, column limit 100).
- Static analysis is enforced by `.clang-tidy` (bugprone + cppcoreguidelines + modernize).
- Run before every commit:
  ```
  tools/lint.sh
  ```

## Naming Conventions

| Symbol | Convention | Example |
|---|---|---|
| Namespace | lower_case | `xuanjing::upscale` |
| Class / Struct | CamelCase | `RuntimeContext` |
| Function | CamelCase | `RunUpscale()` |
| Variable / Parameter | lower_case | `quality_preset` |
| Member variable | lower_case with `_` suffix | `device_id_` |
| Constant / Enum value | kCamelCase | `kQuality` |
| Macro | UPPER_CASE | `XUANJING_CHECK(...)` |

## File Layout

Each module follows:
```
modules/<name>/
  include/<name>/   # public headers only
  src/              # implementation files
  CMakeLists.txt
  README.md
```

## API Design Rules

- Public headers must not expose internal implementation types.
- All public APIs must have explicit error return (bool or result type). Exceptions are forbidden in public interface.
- Use `const&` for input-only parameters larger than a pointer size.
- Avoid raw pointers in public API; use `std::span`, `std::string_view`, or explicit lifetime documentation.

## Dependencies

- No implicit transitive headers. Each `.cpp` includes its own direct dependencies.
- System and third-party includes use angle brackets `<>`. Project includes use quoted paths `""`.

## Testing

- Unit tests live adjacent to `src/` under `tests/unit/<module>/`.
- Each new public function must have at least one positive and one negative unit test before merging.
- Integration tests under `tests/integration/` cover cross-module interactions.

## Memory Safety

- All builds run with AddressSanitizer in CI (`cmake --preset asan`).
- UndefinedBehaviorSanitizer is enabled for debug builds.
- No manual `new`/`delete` in production code. Use RAII and smart pointers.
