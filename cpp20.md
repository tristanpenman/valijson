# Transitioning Valijson to C++20

Notes for moving Valijson to modern C++20 baseline while keeping the parser-adapter architecture.

## Goals

Better diagnostics, fewer string allocations during object traversal, clearer callback code, and safer validation result handling.

Specifically:

- Require C++20 throughout the public headers, tests, examples, and build files.
- Improve template diagnostics for adapter authors.
- Let adapters pass library-native string references without unnecessary copies.
- Keep the existing parser-adapter model intact.

## Ideas

### Concepts

Enforce adapter requirements with concepts.

- `BasicAdapter` uses the Curiously Recurring Template Pattern (CRTP), but its expectations for `AdapterType`, `ArrayType`, `ObjectType`, and `ValueType` are mostly comments today.
- Add concepts for capabilities such as iterable array/object views, optional accessors like `getArrayOptional`, scalar accessors, object lookup, and key iteration.
- Use those concepts on core templates and adapter helpers so errors point at the missing adapter capability instead of failing deep in template instantiation.
- Constrained overloads could also support extra adapter behaviour without expanding the baseline adapter contract.

### Strings

Use `std::string_view` for object keys.

- Object comparison and lookup paths use `const std::string &`, which forces some adapters to allocate temporary strings.
- RapidJSON and Boost.JSON can expose borrowed string references; `std::string_view` lets those adapters forward native key references.
- Switch callbacks, lookup helpers, and key comparison APIs to `std::string_view`.
- C++20 gives `std::string_view` good constexpr and hashing support, so it fits non-owning key references.

### Lambdas

Replace `std::bind` whre possible.

- Validation dispatch uses `std::bind` to connect visitor callbacks.
- Prefer capturing lambdas for clearer captures and data flow.
- Use `std::bind_front` only where partial application is clearer than a lambda.
- This removes old `std::bind` placeholder and forwarding surprises from callback code.

### Safety

Mark boolean-returning validation APIs `[[nodiscard]]`.

- Core entry points such as `ValidatorT::validate` return success or failure, but callers can ignore the result today.
- `[[nodiscard]]` makes accidental omissions visible at compile time.

## Migration Strategy

1. Update build configuration, documentation, and CI to require C++20.
2. Convert object-key APIs from `std::string` references to `std::string_view`.
3. Replace `std::bind` usage with lambdas or `std::bind_front`.
4. Add `[[nodiscard]]` to boolean-returning validation APIs.
5. Introduce adapter concepts, starting with internal constraints
6. Expand to public extension points once the names and diagnostics are stable.
