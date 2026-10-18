# URI resolution edge cases

`valijson::internal::uri::resolveRelativeUri` resolves URI references against a base resolution scope. The existing tests in `tests/test_uri.cpp` cover 45 cases, including many of the normal and abnormal examples from RFC 3986, plus several Valijson-specific cases. All of those tests currently pass.

The RFC test suites are deliberately named `Rfc3986ImplementedNormalExamples` and `Rfc3986ImplementedAbnormalExamples`: they contain only a subset of the RFC examples. Several omitted examples expose behavior that is inconsistent with RFC 3986.

## Confirmed edge cases

| Base URI                             | Reference                     | RFC 3986 result                    | Current result                                     |
| ------------------------------------ | ----------------------------- | ---------------------------------- | -------------------------------------------------- |
| `http://a/b/c/d;p?q`                 | `g:h`                         | `g:h`                              | `http://a/b/c/g:h`                                 |
| `http://a/b/c/d;p?q`                 | `//g`                         | `http://g`                         | `http://a/g`                                       |
| `http://a/b/c/d;p?q`                 | `#s`                          | `http://a/b/c/d;p?q#s`             | `http://a/b/c/d;p#s`                               |
| `http://a/b/c/d;p?q`                 | `.`                           | `http://a/b/c/`                    | `http://a/b/c`                                     |
| `http://a/b/c/d;p?q`                 | `..`                          | `http://a/b/`                      | `http://a/b`                                       |
| `http://a/b/c/d;p?q`                 | `./g/.`                       | `http://a/b/c/g/`                  | `http://a/b/c/g`                                   |
| `http://a/b/c/d;p?q`                 | `g//h`                        | `http://a/b/c/g//h`                | `http://a/b/c/g/h`                                 |
| `http://a/b/c/d;p?q#old`             | empty                         | `http://a/b/c/d;p?q`               | `http://a/b/c/d;p?q#old`                           |
| `http://example.com?version=1`       | `schema.json`                 | `http://example.com/schema.json`   | `http://example.com?version=1/schema.json`         |
| `http://a/b/c/d;p?q`                 | `mailto:user@example.com`     | `mailto:user@example.com`          | `http://a/b/c/mailto:user@example.com`             |
| `urn:example:a`                      | `b`                           | `urn:b`                            | `b`                                                |

## Causes

### Scheme detection

`parseScheme` recognizes a scheme only when it is followed by `://`. RFC 3986 defines a scheme using the form `scheme:`, so absolute references such as `g:h`, `mailto:user@example.com`, `data:...`, and `file:/...` are not recognized as absolute. The special handling for URNs covers only one subset of valid non-`://` schemes.

The same limitation affects base URIs without an authority component. For example, resolving `b` against `urn:example:a` loses the `urn:` scheme.

### Network-path references

A reference beginning with `//` supplies a new authority while inheriting the base scheme. The implementation treats it as an absolute path and retains the old authority. It then collapses the two leading slashes during path normalization.

### Query and fragment handling

The base query is removed before fragment-only references are handled. Consequently, resolving `#s` against a base containing `?q` loses that query.

An empty reference returns the resolution scope immediately. Under RFC 3986, it inherits the base path and query but does not inherit the base fragment.

### Dot segments and empty path segments

Terminal `.` and `..` segments imply a trailing slash in the normalized path. The current segment-based normalization removes the segment but does not preserve that slash.

The normalization also discards empty path segments, changing paths such as `g//h` to `g/h`. Empty non-dot segments are significant and should be preserved.

There is also a boundary-safety issue when normalization produces an empty string from a path ending in `/`: the trailing-slash check indexes `normalisedPath[normalisedPath.size() - 1]` without first checking whether `normalisedPath` is empty.

### Authority extraction

When extracting an authority from a base URI, the implementation searches only for `/`. An authority can also be terminated by `?` or `#`. A base such as `http://example.com?version=1` therefore has its query incorrectly included in the authority.

## Recommended test coverage

The omitted RFC 3986 examples should be added first:

- `g:h`
- `//g`
- `#s`
- `.`
- `..`
- `./g/.`
- `http:g`, using strict RFC behavior

Additional regression coverage should include:

- Absolute references using `https://`, `mailto:`, `data:`, and `file:/`.
- Network-path references containing a path, query, and fragment.
- Empty references and fragment-only references against bases with existing queries and fragments.
- Empty query and fragment markers (`?` and `#`).
- Terminal `.` and `..` segments at different path depths.
- Repeated slashes in both base paths and references.
- Authority-only bases followed by a query or fragment.
- Authorities containing user information, ports, and IPv6 literals.
- Relative resolution scopes and non-hierarchical base URIs.
- Inputs that normalize to an empty path, exercised under sanitizers or a bounds-checked standard library.

## Absolute-reference normalization policy

RFC 3986 resolution removes dot segments from the path of an absolute reference. For example, resolving `http://x/a/../b` produces `http://x/b`. The helper currently returns recognized absolute references unchanged. Before adding this as a conformance test, decide whether the helper is intended to perform full RFC resolution or deliberately preserve absolute references verbatim.
