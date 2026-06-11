# OSS-Fuzz Issues for Valijson

This list was compiled from the public OSS-Fuzz Issue Tracker search for `valijson` and the corresponding public issue pages on 2026-06-11.

Source listing: <https://issues.oss-fuzz.com/issues?q=valijson>

Some C++ symbols are truncated in the Issue Tracker's displayed title or crash-state text. Follow the issue links for the full tracker entry, detailed report, and reproducer links where available.

## Summary

- Open issues still needing attention: 8
- Issues verified since the previous snapshot: 6

## Open Issues

| Issue                                                     | Status | Priority | Finding                         | Engine / job                         | Crash state                                                                                                                                                             |
| --------------------------------------------------------- | ------ | -------- | ------------------------------- | ------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [490886677](https://issues.oss-fuzz.com/issues/490886677) | New    | P2       | Stack-overflow                  | honggfuzz / honggfuzz_asan_valijson  | `fuzzer`                                                                                                                                                                |
| [444637784](https://issues.oss-fuzz.com/issues/444637784) | New    | P2       | Timeout (exceeds 60 secs)       | libFuzzer / libfuzzer_ubsan_valijson | `fuzzer`                                                                                                                                                                |
| [444619048](https://issues.oss-fuzz.com/issues/444619048) | New    | P2       | Stack-overflow                  | honggfuzz / honggfuzz_asan_valijson  | `std::__1::__wrap_iter<char const*> std::__1::basic_regex<char, std::__1::regex_t` / `std::__1::__wrap_iter<char const*> std::__1::basic_regex<char, std::__1::regex_t` |
| [444583510](https://issues.oss-fuzz.com/issues/444583510) | New    | P2       | Stack-overflow                  | libFuzzer / libfuzzer_msan_valijson  | `std::__1::__match_char<char>::~__match_char`                                                                                                                           |
| [441210575](https://issues.oss-fuzz.com/issues/441210575) | New    | P2       | Stack-overflow                  | libFuzzer / libfuzzer_msan_valijson  | `std::__1::__begin_marked_subexpression<char>::~__begin_marked_subexpression` / `std::__1::__empty_state<char>::~__empty_state`                                         |
| [428754690](https://issues.oss-fuzz.com/issues/428754690) | New    | P2       | Stack-overflow                  | libFuzzer / libfuzzer_msan_valijson  | `std::__1::__owns_two_states<char>::~__owns_two_states` / `std::__1::__alternate<char>::~__alternate`                                                                   |
| [427531814](https://issues.oss-fuzz.com/issues/427531814) | New    | P2       | Stack-overflow                  | honggfuzz / honggfuzz_asan_valijson  | `std::__1::__r_anchor_multiline<char>::~__r_anchor_multiline`                                                                                                           |
| [422513162](https://issues.oss-fuzz.com/issues/422513162) | New    | P2       | Out-of-memory (exceeds 2560 MB) | libFuzzer / libfuzzer_ubsan_valijson | `fuzzer`                                                                                                                                                                |

## Verified Issues

The Issue Tracker currently reports 35 Valijson issues as verified. The issues below were previously listed as unverified in this file and are now verified.

| Issue                                                     | Status   | Priority | Finding        | Engine / job                         | Crash state                                                                                                                                                             |
| --------------------------------------------------------- | -------- | -------- | -------------- | ------------------------------------ | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [484281247](https://issues.oss-fuzz.com/issues/484281247) | Verified | P2       | Stack-overflow | libFuzzer / libfuzzer_ubsan_valijson | `HandleDynamicTypeCacheMiss` / `valijson::ValidationVisitor<valijson::adapters::GenericRapidJsonAdapter<rapidjso`                                                       |
| [473895988](https://issues.oss-fuzz.com/issues/473895988) | Verified | P2       | Stack-overflow | libFuzzer / libfuzzer_ubsan_valijson | `std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<ch` / `rapidjson::GenericMemberIterator<false, rapidjson::UTF8<char>, rapidjson::Memory` |
| [462677906](https://issues.oss-fuzz.com/issues/462677906) | Verified | P2       | Stack-overflow | libFuzzer / libfuzzer_msan_valijson  | `valijson::adapters::GenericRapidJsonAdapter<rapidjson::GenericValue<rapidjson::U`                                                                                      |
| [445208680](https://issues.oss-fuzz.com/issues/445208680) | Verified | P2       | Stack-overflow | libFuzzer / libfuzzer_msan_valijson  | `valijson::constraints::OneOfConstraint valijson::SchemaParser::makeOneOfConstrai` / `void valijson::SchemaParser::populateSchema<valijson::adapters::GenericRapidJson` |
| [379425187](https://issues.oss-fuzz.com/issues/379425187) | Verified | P2       | Stack-overflow | libFuzzer / libfuzzer_ubsan_valijson | `void valijson::SchemaParser::populateSchema<valijson::adapters::GenericRapidJson` / `valijson::Subschema const* valijson::SchemaParser::makeOrReuseSchema<valijson::a` |
| [377356089](https://issues.oss-fuzz.com/issues/377356089) | Verified | P2       | Stack-overflow | libFuzzer / libfuzzer_ubsan_valijson | `valijson::ValidationVisitor<valijson::adapters::GenericRapidJsonAdapter<rapidjso` / `valijson::Subschema::apply`                                                       |

## Obsolete Issues

| Issue                                                   | Status   | Priority | Finding | Engine / job                         | Crash state |
| ------------------------------------------------------- | -------- | -------- | ------- | ------------------------------------ | ----------- |
| [42519248](https://issues.oss-fuzz.com/issues/42519248) | Obsolete | P2       | Abrt    | libFuzzer / libfuzzer_ubsan_valijson | `NULL`      |
