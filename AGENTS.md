# Agents

This project provides a C++ header-only JSON Schema validator that can be used with various different JSON parsers. It supports draft 7 of the JSON Schema specification.

The current branch targets wide C++17 compatibility.

## Examples

A range of parser-specific examples are included in the [examples](examples) directory.

## Test Suite

A robust test suite is included in the `tests` directory. This includes a range of tests for individual parsers, and a framework for executing test cases from the [thirdparty/JSON-Schema-Test-Suite](JSON-Schema-Test-Suite) project.

## Build

The project is designed to be used with CMake. Configure with `-Dvalijson_BUILD_TESTS=1` to ensure that tests are built.

Configure with `-Dvalijson_BUILD_EXAMPLES=1` when examples are being worked on.

When running `scripts/test.sh` for individual requests, use the script's default `build` directory. It is not necessary to create temporary build directories unless the request specifically requires a separate build tree.
