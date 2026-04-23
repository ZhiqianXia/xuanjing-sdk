---
name: cpp17-recommended-syntax
description: "Use when: writing or reviewing C++17 code style, recommending modern C++17 syntax, refactoring legacy C++ to C++17 idioms, choosing optional/variant/string_view/filesystem/structured bindings/if constexpr/emplace and safer ownership patterns."
---

# C++17 Recommended Syntax Guide

This skill provides practical, team-friendly C++17 syntax recommendations for production code.

## Goals

- Improve readability and maintainability.
- Reduce common memory and lifetime bugs.
- Prefer zero-cost abstractions and standard-library tools.
- Keep APIs explicit and unsurprising.

## Core Recommendations

### 1) Prefer automatic type deduction with intent

- Use auto when the initializer already makes the type obvious.
- Keep explicit types for public API boundaries and complex numeric types.
- Use const auto for read-only local values.

Preferred:
- const auto count = items.size();

Avoid:
- auto x = 0; where the intended width and signedness are unclear.

### 2) Prefer uniform initialization, avoid narrowing

- Use brace initialization for objects and aggregates.
- Let the compiler reject implicit narrowing conversions.

Preferred:
- std::vector<int> v{1, 2, 3};
- int port{8080};

### 3) Use structured bindings for tuple-like returns

- Use structured bindings to improve clarity when decomposing pairs and tuples.
- Prefer meaningful variable names when unpacking map iteration.

Preferred:
- for (const auto& [name, score] : leaderboard) { ... }

### 4) Use if with initializer and switch with initializer

- Keep temporary lookup variables scoped to the condition.
- Avoid leaking helper variables into outer scope.

Preferred:
- if (auto it = cache.find(key); it != cache.end()) { ... }

### 5) Use if constexpr for compile-time branching

- Replace heavy SFINAE branches when logic can be selected at compile time.
- Keep template code readable and type-safe.

Preferred:
- if constexpr (std::is_integral_v<T>) { ... } else { ... }

### 6) Prefer std::optional for nullable return values

- Use std::optional<T> instead of sentinel values such as -1 or empty strings.
- Use explicit handling for missing values.

Preferred:
- std::optional<User> FindUser(Id id);

### 7) Prefer std::variant for closed type alternatives

- Use std::variant for tagged unions instead of manual enum + union patterns.
- Use std::visit for exhaustive handling.

Preferred:
- using Token = std::variant<NumberToken, NameToken, EndToken>;

### 8) Prefer std::string_view for read-only string parameters

- Use std::string_view for non-owning read-only APIs.
- Do not store std::string_view beyond the lifetime of the source data.

Preferred:
- bool StartsWith(std::string_view text, std::string_view prefix);

### 9) Prefer std::filesystem for file/path operations

- Use std::filesystem::path, exists, create_directories, canonical, and directory iterators.
- Avoid ad-hoc string concatenation for paths.

### 10) Prefer smart pointers and clear ownership

- Use std::unique_ptr for exclusive ownership.
- Use std::shared_ptr only when ownership is genuinely shared.
- Pass raw pointers or references for non-owning access.

Preferred:
- std::unique_ptr<Foo> CreateFoo();
- void Render(const Scene& scene);

### 11) Prefer emplace only when it adds value

- Use emplace_back when constructing in-place avoids temporary objects.
- Use push_back when passing an existing object is clearer.

### 12) Use attributes for intent communication

- Use [[nodiscard]] for important return values.
- Use [[maybe_unused]] for conditionally used variables in debug or feature-gated code.

## API Design Rules for C++17

- Accept lightweight views for inputs (string_view, span-like abstractions if available).
- Return value objects or optional/variant for explicit outcomes.
- Keep exception policy consistent across module boundaries.
- Mark single-argument constructors explicit unless implicit conversion is desired.
- Mark move operations noexcept when valid.

## Common Modernization Refactors

- Replace raw new and delete with make_unique and RAII ownership.
- Replace magic sentinel return values with optional.
- Replace manual tagged unions with variant.
- Replace global helper temporaries with if-initializer scope.
- Replace macro-based type branches with if constexpr.

## Do and Avoid

Do:
- Keep variable scope minimal.
- Prefer standard library facilities over custom utilities.
- Use const aggressively for immutable values.
- Write small, composable functions.

Avoid:
- Returning dangling string_view or references.
- Overusing auto when it hides meaning.
- Using shared_ptr as default ownership model.
- Introducing template metaprogramming complexity without measurable benefit.

## Review Checklist

- Is ownership explicit?
- Is lifetime safe for all non-owning views?
- Is nullable state modeled with optional?
- Are alternatives modeled with variant?
- Are temporary variables scoped tightly with if/switch initializers?
- Are conversions explicit and narrowing prevented?
- Is this still C++17 compatible (no C++20-only syntax)?

## Compatibility Note

This skill targets C++17 only. Avoid C++20 features such as concepts, ranges, and designated initializers unless the project standard is upgraded.
