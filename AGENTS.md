# Agents

This project provides a C++ header-only JSON Schema validator that can be used with various different JSON parsers. It supports draft 7 of the JSON Schema specification.

The current branch targets wide C++17 compatibility.

## Architecture

- All library code lives in headers under `include/valijson`. There are no source files to compile; changes take effect in every consumer of the header.
- Support for each JSON parser is implemented as an *adapter* in `include/valijson/adapters` (rapidjson, nlohmann, jsoncpp, Qt, Boost.JSON, etc). Core validation logic in `validation_visitor.hpp` and `schema_parser.hpp` is adapter-agnostic and operates through the adapter interface.
- `examples/valijson_nlohmann_bundled.hpp` is a **generated** file, produced by `./bundle.sh nlohmann_json > examples/valijson_nlohmann_bundled.hpp`. Never edit it by hand. If you change anything under `include/valijson` that is part of the bundle, regenerate it as part of the same change — CI does not check that it is in sync.

## Coding guidelines

These rules are distilled from real bugs fixed in recent PRs. Follow them when modifying library code.

### Portability comes first

- The library must build on a wide range of compilers and standard libraries (old GCC/libstdc++, libc++, MSVC). Do not introduce a C++17 feature without checking it is supported everywhere — e.g. `std::from_chars` for floating-point types is missing from older standard libraries, which is why `internal/double_parser.hpp` exists with three fallback implementations.
- Never parse or format numbers in a locale-dependent way. Do not use `std::stod`, `strtod`, `atof`, or stream extraction without an imbued classic locale. Use `valijson::internal` helpers (e.g. `double_parser.hpp`) instead.

### Exceptions may be disabled

- Builds can set `VALIJSON_USE_EXCEPTIONS=0`. Never `throw` directly from library code — call `throwRuntimeError()` / `throwLogicError()` from `valijson/exceptions.hpp`, and wrap any `try`/`catch` blocks in `#if VALIJSON_USE_EXCEPTIONS`.
- When you do catch exceptions from standard library calls, handle *every* exception type the call can throw. A recent bug: `std::stoul` throws `std::invalid_argument` for non-numeric input **and** `std::out_of_range` for oversized numeric input; only the former was caught, so a 34-digit JSON Pointer array index escaped as an unhandled exception instead of the expected `std::runtime_error`.

### Numeric edge cases

- Casting a `double` to `int64_t` when the value is outside the representable range is undefined behaviour. Range-check first, and handle the out-of-range case in floating point (see the `multipleOf` integer check in `validation_visitor.hpp`).
- Handle non-finite values (NaN, Infinity). Underlying parsers differ: nlohmann/json serialises non-finite floats as `null`, so the nlohmann adapter deliberately treats them as null during validation to match serialised behaviour.
- Watch for overflow in any arithmetic on schema constraints or JSON values; JSON numbers are not bounded by C++ integer types.

### String edge cases

- JSON strings may contain embedded null bytes. Never rely on `strlen`, `c_str()` without a length, or any null-terminated assumption. `u8_strlen` in `utils/utf8_utils.hpp` counts embedded nulls correctly for length validation — keep that property.
- Guard any back-indexing such as `s[s.size() - 1]` with an emptiness check. A recent bug: `resolveRelativeUri()` indexed `normalisedPath[normalisedPath.size() - 1]` when the string could be empty, causing an out-of-bounds access.
- All string handling must be UTF-8 aware where the spec requires it (e.g. `minLength`/`maxLength` count code points, not bytes).

### Adapters

- Do not assume default template parameters of the underlying parser library. A recent bug: the rapidjson `AdapterTraits` specialisation hard-coded `rapidjson::Document`, which breaks when a consumer redefines `RAPIDJSON_DEFAULT_ALLOCATOR`. Derive types from the value type's own typedefs (e.g. `ValueType::AllocatorType`) instead of naming concrete instantiations.
- Behavioural fixes in one adapter often apply to the others. When fixing an adapter bug, check whether the sibling adapters in `include/valijson/adapters` share the same flaw.
- Adapters should present consistent validation semantics regardless of the underlying parser's quirks (see the non-finite float handling above).

### Testing

- Every bug fix must come with a regression test in `tests/` that fails without the fix. Follow the existing pattern: add the test alongside related cases in the relevant `test_*.cpp` file (e.g. the oversized JSON Pointer token test sits next to the out-of-bounds index test in `test_json_pointer.cpp`).
- Compile-time contracts can be tested with a small standalone executable and `static_assert` (see `tests/test_rapidjson_crt_allocator.cpp`, which sets `RAPIDJSON_DEFAULT_ALLOCATOR` before including the adapter). Register new test executables in `CMakeLists.txt`.
- Changes to core validation logic should be exercised against the JSON-Schema-Test-Suite runner, not just hand-written unit tests.

## Inspector

The [inspector](inspector) directory contains JSON Inspector, a small Qt GUI application for interactively experimenting with JSON Schemas and target documents. It is a self-contained CMake project (not part of the main build) that consumes the library headers via a relative include path and uses the `QtJsonAdapter`. Because it is not built by CI, it can break silently: when changing the public API, the Qt adapter, or header layout under `include/valijson`, check whether the inspector's sources in `inspector/src` are affected and update them so the project still builds (configure and build it separately from the `inspector` directory).

## Examples

A range of parser-specific examples are included in the [examples](examples) directory.

## Test Suite

A robust test suite is included in the `tests` directory. This includes a range of tests for individual parsers, and a framework for executing test cases from the [thirdparty/JSON-Schema-Test-Suite](JSON-Schema-Test-Suite) project.

## Build

The project is designed to be used with CMake. Configure with `-Dvalijson_BUILD_TESTS=1` to ensure that tests are built.

Configure with `-Dvalijson_BUILD_EXAMPLES=1` when examples are being worked on.

When running `scripts/test.sh` for individual requests, use the script's default `build` directory. It is not necessary to create temporary build directories unless the request specifically requires a separate build tree.
