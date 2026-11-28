#ifdef _MSC_VER
#pragma warning(disable : 4706)
#include <picojson.h>
#pragma warning(default : 4706)
#else
#include <picojson.h>
#endif

#include <algorithm>
#include <cctype>
#include <iterator>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#ifdef VALIJSON_BUILD_JSON11_ADAPTER
#include <valijson/adapters/json11_adapter.hpp>
#include <valijson/utils/json11_utils.hpp>
#endif
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/adapters/picojson_adapter.hpp>
#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/exceptions.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>
#include <valijson/utils/nlohmann_json_utils.hpp>
#include <valijson/utils/picojson_utils.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#ifdef VALIJSON_BUILD_POCO_ADAPTER
#include <valijson/adapters/poco_json_adapter.hpp>
#include <valijson/utils/poco_json_utils.hpp>
#endif

#define TEST_SUITE_DIR "../thirdparty/JSON-Schema-Test-Suite/validation/tests/"

#define LOCAL_TEST_SUITE_DIR "../tests/data/test_suite/"

using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::adapters::AdapterTraits;

namespace {

//
// Dialects
// ------------------------------------------------------------------------------------------------
//
// Dialects are identified by the numbers used in the 'compatibility' property
// of test cases: 3, 4, 6 and 7 for drafts 3 to 7, and the year for date-based
// releases, such as 2019 for draft 2019-09 and 2020 for draft 2020-12.
//
// To run the test suite for another dialect, add an entry for it here, along
// with any meta-schemas it needs (see below) and known failures.
//

struct Dialect
{
    /// Number used in the 'compatibility' property of test cases
    int version;

    /// Prefix for test names
    const char *name;

    /// Parser version used for schemas of the dialect
    SchemaParser::Version parserVersion;

    /// Whether 'format' is treated as an assertion. Up to draft 7, test cases
    /// tagged 'format-assertion' apply when it is, and test cases tagged
    /// 'format-annotation' apply when it is not.
    bool formatAssertion;
};

// clang-format off
constexpr Dialect kDialects[] = {
    { 3, "Draft3", SchemaParser::kDraft3, true },
    { 4, "Draft4", SchemaParser::kDraft4, true },
    { 6, "Draft6", SchemaParser::kDraft6, true },
    { 7, "Draft7", SchemaParser::kDraft7, true },
};
// clang-format on

//
// Meta-schemas
// ------------------------------------------------------------------------------------------------
//
// Test cases provide the schemas they reference through the 'externalSchemas'
// property, with the exception of meta-schemas, which are loaded from files.
//

// clang-format off
const std::map<std::string, std::string> kMetaSchemas = {
    { "http://json-schema.org/draft-03/schema", "../doc/schema/draft-03.json" },
    { "http://json-schema.org/draft-04/schema", "../doc/schema/draft-04.json" },
    { "http://json-schema.org/draft-06/schema", "../doc/schema/draft-06.json" },
    { "http://json-schema.org/draft-07/schema", "../doc/schema/draft-07.json" },
};
// clang-format on

//
// Test suite files
// ------------------------------------------------------------------------------------------------
//
// Each file contains test cases for multiple dialects. The 'compatibility'
// property of each test case determines which dialects it applies to, so every
// file is run for every supported dialect. Files without any test cases for a
// dialect are reported as skipped.
//

// clang-format off
constexpr const char *kTestSuiteFiles[] = {
    "additionalProperties.json",
    "allOf.json",
    "anchor-legacy.json",
    "anchor.json",
    "anyOf.json",
    "boolean_schema.json",
    "const.json",
    "contains.json",
    "content.json",
    "default.json",
    "dependentRequired.json",
    "dependentSchemas.json",
    "disallow.json",
    "dynamicRef.json",
    "enum.json",
    "extends.json",
    "format-color.json",
    "format-date-time.json",
    "format-date.json",
    "format-duration.json",
    "format-email.json",
    "format-host-name.json",
    "format-hostname.json",
    "format-idn-email.json",
    "format-idn-hostname.json",
    "format-ip-address.json",
    "format-ipv4.json",
    "format-ipv6.json",
    "format-iri-reference.json",
    "format-iri.json",
    "format-json-pointer.json",
    "format-regex.json",
    "format-relative-json-pointer.json",
    "format-time.json",
    "format-unknown.json",
    "format-uri-reference.json",
    "format-uri-template.json",
    "format-uri.json",
    "format-uuid.json",
    "id-legacy.json",
    "id.json",
    "if-then-else.json",
    "infinite-loop-detection-legacy.json",
    "infinite-loop-detection.json",
    "items.json",
    "maxItems.json",
    "maxLength.json",
    "maxProperties.json",
    "maximum.json",
    "minItems.json",
    "minLength.json",
    "minProperties.json",
    "minimum.json",
    "multipleOf.json",
    "not.json",
    "oneOf.json",
    "pattern.json",
    "patternProperties.json",
    "properties.json",
    "propertyNames.json",
    "recursiveRef.json",
    "ref-legacy.json",
    "ref.json",
    "required.json",
    "type.json",
    "unevaluatedItems.json",
    "unevaluatedProperties.json",
    "uniqueItems.json",
    "vocabulary.json",
    "optional/bignum.json",
    "optional/content.json",
    "optional/cross-draft.json",
    "optional/dependencies-compatibility.json",
    "optional/dynamicRef.json",
    "optional/ecmascript-regex.json",
    "optional/float-overflow.json",
    "optional/format-assertion.json",
    "optional/format-regex.json",
    "optional/format-time.json",
    "optional/non-bmp-regex.json",
    "optional/non-schemas.json",
    "optional/zero-terminated-floats.json",
};
// clang-format on

//
// Local test files
// ------------------------------------------------------------------------------------------------
//
// Test cases that are not part of the JSON-Schema-Test-Suite, such as
// regression tests for bugs in Valijson. They are in the same format as the
// test suite, in tests/data/test_suite, and are run in the same way.
//

const std::vector<const char *> kLocalTestFiles = {
    "draft6-draft7-features.json",
};

//
// Known failures
// ------------------------------------------------------------------------------------------------
//
// Test cases that exercise features which have not been implemented yet. A null
// test case refers to the whole file, and a null test to the whole test case.
//
// Known failures are still run, and must fail for at least one adapter, so that
// entries are removed once the features they describe are implemented. Test
// cases that would crash the test runner are not run at all.
//
// Every entry must refer to a file, test case and test that apply to the
// dialect, so that entries do not outlive the test cases they describe.
//

struct KnownFailure
{
    int draft;
    const char *file;
    const char *testCase;
    const char *test;
};

// clang-format off
constexpr KnownFailure kKnownFailures[] = {
    // Draft 3 keywords and formats that are not supported. Only the tests of
    // 'disallow' that require a value to be rejected fail.
    { 3, "format-color.json", nullptr, nullptr },
    { 3, "format-host-name.json", nullptr, nullptr },
    { 3, "format-ip-address.json", nullptr, nullptr },
    { 3, "disallow.json", "disallow", "disallowed" },
    { 3, "disallow.json", "multiple disallow", "mismatch" },
    { 3, "disallow.json", "multiple disallow", "other mismatch" },
    { 3, "disallow.json", "multiple disallow subschema", "mismatch" },
    { 3, "disallow.json", "multiple disallow subschema", "other mismatch" },

    // The Draft 3 'time' format does not have a time offset, but validation
    // requires one by default
    { 3, "format-time.json", "validation of time strings (hh:mm:ss)", "a valid time string" },

    // Formats that are not supported
    { 3, "format-email.json", nullptr, nullptr },
    { 4, "format-email.json", nullptr, nullptr },
    { 6, "format-email.json", nullptr, nullptr },
    { 7, "format-email.json", nullptr, nullptr },
    { 7, "format-idn-email.json", nullptr, nullptr },
    { 7, "format-idn-hostname.json", nullptr, nullptr },
    { 7, "format-iri-reference.json", nullptr, nullptr },
    { 7, "format-iri.json", nullptr, nullptr },
    { 3, "format-uri.json", nullptr, nullptr },
    { 4, "format-uri.json", nullptr, nullptr },
    { 6, "format-uri.json", nullptr, nullptr },
    { 7, "format-uri.json", nullptr, nullptr },
    { 6, "format-uri-reference.json", nullptr, nullptr },
    { 7, "format-uri-reference.json", nullptr, nullptr },
    { 6, "format-uri-template.json", nullptr, nullptr },
    { 7, "format-uri-template.json", nullptr, nullptr },

    // A-labels are only checked against the syntax of RFC 1123, and are not
    // decoded to check them against the rules of IDNA2008
    { 7, "format-hostname.json", "validation of A-label (punycode) host names", nullptr },

    // Plain-name fragments are not resolved in remote documents
    { 3, "anchor-legacy.json", "anchor within remote ref (id)", nullptr },
    { 4, "anchor-legacy.json", "anchor within remote ref (id)", nullptr },
    { 6, "anchor-legacy.json", "anchor within remote ref", nullptr },
    { 7, "anchor-legacy.json", "anchor within remote ref", nullptr },

    // Identifiers in non-schema locations are treated as schema identifiers
    { 4, "optional/non-schemas.json", "$id inside an enum is not a real identifier (id/definitions)", nullptr },
    { 4, "optional/non-schemas.json", "anchor inside an enum is not a real identifier (id)", nullptr },
    { 6, "optional/non-schemas.json", "$id inside an enum is not a real identifier ($id/definitions)", nullptr },
    { 7, "optional/non-schemas.json", "$id inside an enum is not a real identifier ($id/definitions)", nullptr },
    { 6, "optional/non-schemas.json", "$id inside an unknown keyword is not a real identifier", nullptr },
    { 7, "optional/non-schemas.json", "$id inside an unknown keyword is not a real identifier", nullptr },
    { 6, "optional/non-schemas.json", "anchor inside an enum is not a real identifier ($id)", nullptr },
    { 7, "optional/non-schemas.json", "anchor inside an enum is not a real identifier ($id)", nullptr },

    // Regular expressions are evaluated by std::regex, which does not
    // implement ECMA 262 with Unicode semantics
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO (as \\u escape) matches" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO matches (unlike e.g. Python)" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\S matches everything but whitespace", "Non-whitespace matches" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\W matches everything but ascii letters", "latin-1 e-acute matches (unlike e.g. Python)" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "EM SPACE matches (Space_Separator)" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "latin-1 non-breaking-space matches" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "paragraph separator matches (line terminator)" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "zero-width whitespace matches" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO (as \\u escape) matches" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO matches (unlike e.g. Python)" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\S matches everything but whitespace", "Non-whitespace matches" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\W matches everything but ascii letters", "latin-1 e-acute matches (unlike e.g. Python)" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "EM SPACE matches (Space_Separator)" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "latin-1 non-breaking-space matches" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "paragraph separator matches (line terminator)" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "zero-width whitespace matches" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO (as \\u escape) matches" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO (as \\u escape) matches" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO matches (unlike e.g. Python)" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\D matches everything but ascii digits", "NKO DIGIT ZERO matches (unlike e.g. Python)" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\S matches everything but whitespace", "Non-whitespace matches" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\S matches everything but whitespace", "Non-whitespace matches" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\W matches everything but ascii letters", "latin-1 e-acute matches (unlike e.g. Python)" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\W matches everything but ascii letters", "latin-1 e-acute matches (unlike e.g. Python)" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "EM SPACE matches (Space_Separator)" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "EM SPACE matches (Space_Separator)" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "latin-1 non-breaking-space matches" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "latin-1 non-breaking-space matches" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "paragraph separator matches (line terminator)" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "paragraph separator matches (line terminator)" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "zero-width whitespace matches" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 \\s matches whitespace", "zero-width whitespace matches" },
    { 3, "optional/format-regex.json", "\\a is not an ECMA 262 control escape", nullptr },
    { 3, "optional/format-regex.json", "ECMA 262 lookbehind is valid, including variable width", nullptr },
    { 7, "optional/format-regex.json", "\\a is not an ECMA 262 control escape", nullptr },
    { 7, "optional/format-regex.json", "ECMA 262 lookbehind is valid, including variable width", nullptr },
#if defined(VALIJSON_USE_BOOST_REGEX) && VALIJSON_USE_BOOST_REGEX
    // Boost.Regex, which uses its Perl syntax, rejects empty character
    // classes, and accepts inline flags and comments
    { 3, "optional/format-regex.json", "ECMA 262 character classes and escapes", "an empty character class is valid ECMA 262" },
    { 3, "optional/format-regex.json", "ECMA 262 character classes and escapes", "a negated empty character class is valid ECMA 262" },
    { 3, "optional/format-regex.json", "Python-specific regular expression syntax is not valid ECMA 262", "inline comment group (?#...) is not ECMA 262" },
    { 3, "optional/format-regex.json", "global inline flag groups are not valid ECMA 262", "a single global inline flag (?i)" },
    { 3, "optional/format-regex.json", "global inline flag groups are not valid ECMA 262", "multiple global inline flags (?ims)" },
    { 7, "optional/format-regex.json", "ECMA 262 character classes and escapes", "an empty character class is valid ECMA 262" },
    { 7, "optional/format-regex.json", "ECMA 262 character classes and escapes", "a negated empty character class is valid ECMA 262" },
    { 7, "optional/format-regex.json", "Python-specific regular expression syntax is not valid ECMA 262", "inline comment group (?#...) is not ECMA 262" },
    { 7, "optional/format-regex.json", "global inline flag groups are not valid ECMA 262", "a single global inline flag (?i)" },
    { 7, "optional/format-regex.json", "global inline flag groups are not valid ECMA 262", "multiple global inline flags (?ims)" },
#else
    // std::regex does not support control escapes such as \cA, or named groups
    { 3, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and lower letter", "matches" },
    { 3, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and upper letter", "matches" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and lower letter", "matches" },
    { 4, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and upper letter", "matches" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and lower letter", "matches" },
    { 6, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and upper letter", "matches" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and lower letter", "matches" },
    { 7, "optional/ecmascript-regex.json", "ECMA 262 regex escapes control codes with \\c and upper letter", "matches" },
    { 3, "optional/format-regex.json", "ECMA 262 named groups and backreferences are valid", nullptr },
    { 7, "optional/format-regex.json", "ECMA 262 named groups and backreferences are valid", nullptr },
#endif
    { 3, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches empty" },
    { 3, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches two" },
    { 3, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: patternProperties", "doesn't match two" },
    { 4, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches empty" },
    { 4, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches two" },
    { 4, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: patternProperties", "doesn't match two" },
    { 6, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches empty" },
    { 7, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches empty" },
    { 6, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches two" },
    { 7, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: pattern", "matches two" },
    { 6, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: patternProperties", "doesn't match two" },
    { 7, "optional/non-bmp-regex.json", "Proper UTF-16 surrogate pair handling: patternProperties", "doesn't match two" },

    // Numbers are represented as doubles
    { 3, "optional/bignum.json", "enum comparison with high precision", "a slightly larger decimal is invalid" },
    { 3, "optional/bignum.json", "enum comparison with high precision", "a slightly smaller decimal is invalid" },
    { 3, "optional/bignum.json", "float comparison with high precision (boolean)", "below the exclusive maximum is valid" },
    { 3, "optional/bignum.json", "float comparison with high precision on negative numbers (boolean)", "above the exclusive minimum is valid" },
    { 3, "optional/bignum.json", "integer", nullptr },
    { 3, "optional/bignum.json", "maximum integer comparison", "just above the maximum is invalid" },
    { 3, "optional/bignum.json", "minimum integer comparison", "just below the minimum is invalid" },
    { 3, "optional/bignum.json", "uniqueItems comparison with high precision", "arrays containing distinct decimals are unique" },
    { 3, "optional/bignum.json", "uniqueItems comparison with high precision", "distinct high-precision decimals are unique" },
    { 3, "optional/bignum.json", "uniqueItems comparison with high precision", "objects containing distinct decimals are unique" },
    { 4, "optional/bignum.json", "enum comparison with high precision", "a slightly larger decimal is invalid" },
    { 4, "optional/bignum.json", "enum comparison with high precision", "a slightly smaller decimal is invalid" },
    { 4, "optional/bignum.json", "float comparison with high precision (boolean)", "below the exclusive maximum is valid" },
    { 4, "optional/bignum.json", "float comparison with high precision on negative numbers (boolean)", "above the exclusive minimum is valid" },
    { 4, "optional/bignum.json", "integer", nullptr },
    { 4, "optional/bignum.json", "maximum integer comparison", "just above the maximum is invalid" },
    { 4, "optional/bignum.json", "minimum integer comparison", "just below the minimum is invalid" },
    { 4, "optional/bignum.json", "uniqueItems comparison with high precision", "arrays containing distinct decimals are unique" },
    { 4, "optional/bignum.json", "uniqueItems comparison with high precision", "distinct high-precision decimals are unique" },
    { 4, "optional/bignum.json", "uniqueItems comparison with high precision", "objects containing distinct decimals are unique" },
    { 6, "optional/bignum.json", "const comparison with high precision", "a slightly larger decimal is invalid" },
    { 7, "optional/bignum.json", "const comparison with high precision", "a slightly larger decimal is invalid" },
    { 6, "optional/bignum.json", "const comparison with high precision", "a slightly smaller decimal is invalid" },
    { 7, "optional/bignum.json", "const comparison with high precision", "a slightly smaller decimal is invalid" },
    { 6, "optional/bignum.json", "enum comparison with high precision", "a slightly larger decimal is invalid" },
    { 7, "optional/bignum.json", "enum comparison with high precision", "a slightly larger decimal is invalid" },
    { 6, "optional/bignum.json", "enum comparison with high precision", "a slightly smaller decimal is invalid" },
    { 7, "optional/bignum.json", "enum comparison with high precision", "a slightly smaller decimal is invalid" },
    { 6, "optional/bignum.json", "float comparison with high precision", "below the exclusive maximum is valid" },
    { 7, "optional/bignum.json", "float comparison with high precision", "below the exclusive maximum is valid" },
    { 6, "optional/bignum.json", "float comparison with high precision on negative numbers", "above the exclusive minimum is valid" },
    { 7, "optional/bignum.json", "float comparison with high precision on negative numbers", "above the exclusive minimum is valid" },
    { 6, "optional/bignum.json", "integer", nullptr },
    { 7, "optional/bignum.json", "integer", nullptr },
    { 6, "optional/bignum.json", "maximum integer comparison", "just above the maximum is invalid" },
    { 7, "optional/bignum.json", "maximum integer comparison", "just above the maximum is invalid" },
    { 6, "optional/bignum.json", "minimum integer comparison", "just below the minimum is invalid" },
    { 7, "optional/bignum.json", "minimum integer comparison", "just below the minimum is invalid" },
    { 6, "optional/bignum.json", "uniqueItems comparison with high precision", "arrays containing distinct decimals are unique" },
    { 7, "optional/bignum.json", "uniqueItems comparison with high precision", "arrays containing distinct decimals are unique" },
    { 6, "optional/bignum.json", "uniqueItems comparison with high precision", "distinct high-precision decimals are unique" },
    { 7, "optional/bignum.json", "uniqueItems comparison with high precision", "distinct high-precision decimals are unique" },
    { 6, "optional/bignum.json", "uniqueItems comparison with high precision", "objects containing distinct decimals are unique" },
    { 7, "optional/bignum.json", "uniqueItems comparison with high precision", "objects containing distinct decimals are unique" },
    { 3, "optional/zero-terminated-floats.json", nullptr, nullptr },
    { 4, "optional/zero-terminated-floats.json", nullptr, nullptr },

    // 'contentEncoding' and 'contentMediaType' are not validated
    { 7, "optional/content.json", "validation of binary string-encoding", "an invalid base64 string (% is not a valid character)" },
    { 7, "optional/content.json", "validation of binary-encoded media type documents", "a validly-encoded invalid JSON document" },
    { 7, "optional/content.json", "validation of binary-encoded media type documents", "an invalid base64 string that is valid JSON" },
    { 7, "optional/content.json", "validation of string-encoded content based on media type", "an invalid JSON document" },

    // Embedded schemas are always processed using the dialect of the parser
    { 7, "optional/cross-draft.json", "refs to future drafts are processed as future drafts (2019)", "missing bar is invalid" },
    { 7, "optional/cross-draft.json", "refs to future drafts are processed as future drafts (2020)", "first item not a string is invalid" },
};
// clang-format on

struct CrashingTestCase
{
    int draft;
    const char *file;
    const char *testCase;
};

// clang-format off
constexpr CrashingTestCase kCrashingTestCases[] = {
    // Identifiers using URI schemes without an authority component are
    // resolved as relative references, resulting in unbounded recursion
    { 3, "id-legacy.json", "simple tag URI base URI with $ref via the tag URI (id)" },
    { 3, "id-legacy.json", "simple base URI using an arbitrary URI scheme with $ref to that URI (id)" },
    { 4, "id-legacy.json", "simple tag URI base URI with $ref via the tag URI (id)" },
    { 4, "id-legacy.json", "simple base URI using an arbitrary URI scheme with $ref to that URI (id)" },
    { 6, "id.json", "simple tag URI base URI with $ref via the tag URI" },
    { 6, "id.json", "simple base URI using an arbitrary URI scheme with $ref to that URI" },
    { 7, "id.json", "simple tag URI base URI with $ref via the tag URI" },
    { 7, "id.json", "simple base URI using an arbitrary URI scheme with $ref to that URI" },
};
// clang-format on

//
// Unparseable files
// ------------------------------------------------------------------------------------------------
//
// Test suite files that the parser behind an adapter cannot parse, so that the
// file is not run with that adapter. Loading these files must still fail, so
// that entries are removed once the parser supports them.
//

struct UnparseableFile
{
    /// Name of the adapter, as returned by AdapterTraits::adapterName()
    const char *adapter;
    const char *file;
};

// clang-format off
constexpr UnparseableFile kUnparseableFiles[] = {
    // Poco::JSON rejects some integers that do not fit in a 64-bit integer,
    // such as -18446744073709551616, with "Not a valid unsigned integer"
    { "PocoJsonAdapter", "optional/bignum.json" },
};
// clang-format on

//
// Helpers
// ------------------------------------------------------------------------------------------------
//

/**
 * Determine whether a test case applies to a dialect, using the algorithm
 * described in the README of the test suite
 */
bool isCompatible(const std::string &compatibility, int draft)
{
    bool compatible = true;

    std::stringstream ss(compatibility);
    std::string constraint;
    while (std::getline(ss, constraint, ',')) {
        std::string op;
        size_t pos = 0;
        while (pos < constraint.size() &&
               (constraint[pos] == '<' || constraint[pos] == '=')) {
            op += constraint[pos++];
        }
        const int version = std::stoi(constraint.substr(pos));

        if (op.empty()) {
            compatible = draft >= version;
        } else if (op == "<=") {
            compatible = draft <= version;
        } else if (op == "=") {
            compatible = draft == version;
        } else {
            ADD_FAILURE() << "Unknown compatibility operator '" << op << "'";
            return false;
        }

        if (draft <= version) {
            break;
        }
    }

    return compatible;
}

/**
 * Determine whether the behaviour described by the tags of a test case is
 * provided for a dialect
 */
bool supportsTags(const nlohmann::json &testCase, const Dialect &dialect)
{
    const auto tags = testCase.find("tags");
    if (tags == testCase.end()) {
        return true;
    }

    return std::all_of(tags->begin(), tags->end(),
                       [&dialect](const nlohmann::json &tag) {
                           if (tag == "format-assertion") {
                               return dialect.formatAssertion;
                           }
                           if (tag == "format-annotation") {
                               return !dialect.formatAssertion;
                           }
                           return false;
                       });
}

/**
 * Find the known failure for a test, or for a whole test case if the test is
 * null
 */
const KnownFailure *findKnownFailure(int draft, const std::string &file,
                                     const std::string &testCase,
                                     const std::string *test)
{
    for (const KnownFailure &knownFailure : kKnownFailures) {
        if (knownFailure.draft != draft || file != knownFailure.file) {
            continue;
        }
        if (knownFailure.testCase == nullptr) {
            return &knownFailure;
        }
        if (testCase != knownFailure.testCase) {
            continue;
        }
        if (knownFailure.test == nullptr ||
            (test != nullptr && *test == knownFailure.test)) {
            return &knownFailure;
        }
    }

    return nullptr;
}

bool isCrashingTestCase(int draft, const std::string &file,
                        const std::string &testCase)
{
    return std::any_of(std::begin(kCrashingTestCases),
                       std::end(kCrashingTestCases),
                       [&](const CrashingTestCase &crashing) {
                           return crashing.draft == draft &&
                                  file == crashing.file &&
                                  testCase == crashing.testCase;
                       });
}

bool isUnparseableFile(const std::string &adapter, const std::string &file)
{
    return std::any_of(
        std::begin(kUnparseableFiles), std::end(kUnparseableFiles),
        [&](const UnparseableFile &unparseable) {
            return adapter == unparseable.adapter && file == unparseable.file;
        });
}

/**
 * Information about a test case that applies to the dialect being tested
 */
struct TestCaseInfo
{
    /// Index of the test case in the file
    size_t index = 0;

    std::string description;

    /// Descriptions of the tests in the test case
    std::set<std::string> tests;

    /// External schemas, as JSON text, keyed by retrieval URI
    std::map<std::string, std::string> externalSchemas;
};

//
// Parsing of external schemas
// ------------------------------------------------------------------------------------------------
//

#ifdef VALIJSON_BUILD_JSON11_ADAPTER
bool parseJson(const std::string &text, json11::Json &document)
{
    std::string err;
    document = json11::Json::parse(text, err);
    return err.empty();
}
#endif

bool parseJson(const std::string &text, Json::Value &document)
{
    Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
    std::string err;
    return reader->parse(text.data(), text.data() + text.size(), &document,
                         &err);
}

template <typename Encoding, typename Allocator>
bool parseJson(const std::string &text,
               rapidjson::GenericDocument<Encoding, Allocator> &document)
{
    document.template Parse<rapidjson::kParseIterativeFlag>(text.c_str());
    return !document.HasParseError();
}

bool parseJson(const std::string &text, picojson::value &document)
{
    return picojson::parse(document, text).empty();
}

bool parseJson(const std::string &text, nlohmann::json &document)
{
    document = nlohmann::json::parse(text, nullptr, false);
    return !document.is_discarded();
}

#ifdef VALIJSON_BUILD_POCO_ADAPTER
bool parseJson(const std::string &text, Poco::Dynamic::Var &document)
{
    document = Poco::JSON::Parser().parse(text);
    return true;
}
#endif

//
// Test runner
// ------------------------------------------------------------------------------------------------
//

struct TestSuiteFile
{
    const Dialect *dialect;
    const char *file;

    /// Whether the file is a local test file, rather than part of the test
    /// suite
    bool local;
};

void PrintTo(const TestSuiteFile &testSuiteFile, std::ostream *os)
{
    *os << testSuiteFile.dialect->name << ", "
        << (testSuiteFile.local ? "local " : "") << testSuiteFile.file;
}

std::vector<TestSuiteFile> allTestSuiteFiles()
{
    std::vector<TestSuiteFile> result;
    for (const Dialect &dialect : kDialects) {
        std::transform(std::begin(kTestSuiteFiles), std::end(kTestSuiteFiles),
                       std::back_inserter(result),
                       [&dialect](const char *file) {
                           return TestSuiteFile{&dialect, file, false};
                       });
        std::transform(kLocalTestFiles.begin(), kLocalTestFiles.end(),
                       std::back_inserter(result),
                       [&dialect](const char *file) {
                           return TestSuiteFile{&dialect, file, true};
                       });
    }

    return result;
}

std::string testSuiteFileName(const testing::TestParamInfo<TestSuiteFile> &info)
{
    // Remove the '.json' extension, and replace characters that are not
    // permitted in test names
    const std::string file(info.param.file);
    std::string name = std::string(info.param.dialect->name) +
                       (info.param.local ? "_local_" : "_");
    std::transform(
        file.begin(), file.end() - 5, std::back_inserter(name), [](char c) {
            return std::isalnum(static_cast<unsigned char>(c)) ? c : '_';
        });

    return name;
}

class TestValidator : public testing::TestWithParam<TestSuiteFile>
{
  protected:
    /**
     * Collect information about the test cases in a file that apply to a
     * dialect
     */
    static std::vector<TestCaseInfo> loadTestCases(const std::string &path,
                                                   const Dialect &dialect)
    {
        std::vector<TestCaseInfo> result;

        nlohmann::json document;
        if (!valijson::utils::loadDocument(path, document)) {
            ADD_FAILURE() << "Failed to load test suite file '" << path << "'";
            return result;
        }

        const nlohmann::json &testCases = document["tests"];
        for (size_t index = 0; index < testCases.size(); index++) {
            const nlohmann::json &testCase = testCases[index];

            const auto compatibility = testCase.find("compatibility");
            if (compatibility != testCase.end() &&
                !isCompatible(compatibility->get<std::string>(),
                              dialect.version)) {
                continue;
            }

            if (!supportsTags(testCase, dialect)) {
                continue;
            }

            TestCaseInfo info;
            info.index = index;
            info.description = testCase["description"].get<std::string>();
            for (const auto &test : testCase["tests"]) {
                info.tests.insert(test["description"].get<std::string>());
            }

            const auto externalSchemas = testCase.find("externalSchemas");
            if (externalSchemas != testCase.end()) {
                for (auto itr = externalSchemas->begin();
                     itr != externalSchemas->end(); ++itr) {
                    info.externalSchemas[itr.key()] = itr.value().dump();
                }
            }

            result.push_back(info);
        }

        return result;
    }

    /**
     * Ensure that every known failure and crashing test case refers to a test
     * case and test that apply to the dialect
     */
    static void checkKnownFailures(const Dialect &dialect,
                                   const std::string &file,
                                   const std::vector<TestCaseInfo> &testCases)
    {
        const auto applies = [&testCases](const char *testCase,
                                          const char *test) {
            return std::any_of(testCases.begin(), testCases.end(),
                               [testCase, test](const TestCaseInfo &info) {
                                   return info.description == testCase &&
                                          (test == nullptr ||
                                           info.tests.count(test) > 0);
                               });
        };

        for (const KnownFailure &knownFailure : kKnownFailures) {
            if (knownFailure.draft == dialect.version &&
                file == knownFailure.file) {
                if (testCases.empty()) {
                    ADD_FAILURE() << "Known failure for '" << file
                                  << "' does not apply to " << dialect.name;
                } else if (knownFailure.testCase != nullptr) {
                    EXPECT_TRUE(
                        applies(knownFailure.testCase, knownFailure.test))
                        << "Known failure for test case '"
                        << knownFailure.testCase << "'"
                        << (knownFailure.test != nullptr
                                ? std::string(" and test '") +
                                      knownFailure.test + "'"
                                : std::string())
                        << " does not apply to " << dialect.name << " in '"
                        << file << "'";
                }
            }
        }

        for (const CrashingTestCase &crashing : kCrashingTestCases) {
            if (crashing.draft == dialect.version && file == crashing.file) {
                EXPECT_TRUE(applies(crashing.testCase, nullptr))
                    << "Crashing test case '" << crashing.testCase
                    << "' does not apply to " << dialect.name << " in '" << file
                    << "'";
            }
        }
    }

    /**
     * Ensure that every known failure for a file failed for at least one
     * adapter, so that entries are removed once they no longer fail
     */
    static void
    checkKnownFailuresFailed(const Dialect &dialect, const std::string &file,
                             const std::set<const KnownFailure *> &failed)
    {
        for (const KnownFailure &knownFailure : kKnownFailures) {
            if (knownFailure.draft == dialect.version &&
                file == knownFailure.file && failed.count(&knownFailure) == 0) {
                ADD_FAILURE()
                    << "Known failure for "
                    << (knownFailure.testCase != nullptr
                            ? std::string("test case '") +
                                  knownFailure.testCase + "'"
                            : "the whole file")
                    << (knownFailure.test != nullptr
                            ? std::string(" and test '") + knownFailure.test +
                                  "'"
                            : std::string())
                    << " in '" << file << "' passes for " << dialect.name
                    << "; remove it from the list of known failures";
            }
        }
    }

    template <typename AdapterType>
    static void processTestCases(const std::string &path,
                                 const Dialect &dialect,
                                 const std::string &file,
                                 const std::vector<TestCaseInfo> &testCases,
                                 std::set<const KnownFailure *> &failed)
    {
        typedef typename AdapterTraits<AdapterType>::DocumentType DocumentType;

        DocumentType document;
        const std::string adapter = AdapterTraits<AdapterType>::adapterName();
        if (isUnparseableFile(adapter, file)) {
            EXPECT_FALSE(valijson::utils::loadDocument(path, document))
                << "'" << file << "' can be parsed for adapter '" << adapter
                << "'; remove it from the list of unparseable files";
            return;
        }

        ASSERT_TRUE(valijson::utils::loadDocument(path, document));
        const AdapterType root(document);
        ASSERT_TRUE(root.isObject());
        const typename AdapterType::Object rootObject = root.getObject();
        const auto testCasesItr = rootObject.find("tests");
        ASSERT_NE(rootObject.end(), testCasesItr);

        size_t index = 0;
        auto info = testCases.begin();
        for (const AdapterType testCase : testCasesItr->second.getArray()) {
            if (info == testCases.end()) {
                break;
            }
            if (index++ == info->index) {
                processTestCase<AdapterType>(testCase, dialect, file, *info++,
                                             failed);
            }
        }
    }

    template <typename AdapterType>
    static void processTestCase(const AdapterType &testCase,
                                const Dialect &dialect, const std::string &file,
                                const TestCaseInfo &info,
                                std::set<const KnownFailure *> &failed)
    {
        typedef typename AdapterTraits<AdapterType>::DocumentType DocumentType;

        if (isCrashingTestCase(dialect.version, file, info.description)) {
            return;
        }

        const std::string context =
            "' of test case '" + info.description + "' in '" + file + "' for " +
            dialect.name + " with adapter '" +
            AdapterTraits<AdapterType>::adapterName() + "'";

        const std::map<std::string, std::string> &externalSchemas =
            info.externalSchemas;
        const auto fetchDocument =
            [&externalSchemas](const std::string &uri) -> const DocumentType * {
            auto *document = new DocumentType();

            const auto external = externalSchemas.find(uri);
            const auto metaSchema = kMetaSchemas.find(uri);
            bool loaded = false;
            if (external != externalSchemas.end()) {
                loaded = parseJson(external->second, *document);
            } else if (metaSchema != kMetaSchemas.end()) {
                loaded = valijson::utils::loadDocument(metaSchema->second,
                                                       *document);
            } else {
                delete document;
                valijson::throwRuntimeError("Attempt fetchDoc of " + uri);
            }

            if (!loaded) {
                delete document;
                valijson::throwRuntimeError("Failed fetchDoc of " + uri);
            }

            return document;
        };
        const auto freeDocument = [](const DocumentType *document) {
            delete document;
        };

        const typename AdapterType::Object object = testCase.getObject();

        Schema schema;
        SchemaParser parser(dialect.parserVersion);
#if VALIJSON_USE_EXCEPTIONS
        try {
#endif
            parser.populateSchema(object.find("schema")->second, schema,
                                  fetchDocument, freeDocument);
#if VALIJSON_USE_EXCEPTIONS
        } catch (const std::exception &e) {
            const KnownFailure *knownFailure = findKnownFailure(
                dialect.version, file, info.description, nullptr);
            if (knownFailure) {
                failed.insert(knownFailure);
            } else {
                ADD_FAILURE()
                    << "Exception thrown while parsing schema with message '"
                    << e.what() << context;
            }
            return;
        }
#endif

        const AdapterType tests = object.find("tests")->second;
        Validator validator(tests.hasStrictTypes() ? Validator::kStrongTypes
                                                   : Validator::kWeakTypes);

        for (const AdapterType test : tests.getArray()) {
            const typename AdapterType::Object testObject = test.getObject();
            const std::string description =
                testObject.find("description")->second.getString();
            const bool shouldValidate =
                testObject.find("valid")->second.getBool();
            const AdapterType data = testObject.find("data")->second;

            const KnownFailure *knownFailure = findKnownFailure(
                dialect.version, file, info.description, &description);

            bool valid = !shouldValidate;
            bool validWithResults = !shouldValidate;
#if VALIJSON_USE_EXCEPTIONS
            try {
#endif
                valid = validator.validate(schema, data, nullptr);

                // Validation that collects all errors must reach the same
                // result as validation that stops at the first error
                valijson::ValidationResults results;
                validWithResults = validator.validate(schema, data, &results);
#if VALIJSON_USE_EXCEPTIONS
            } catch (const std::exception &e) {
                if (knownFailure) {
                    failed.insert(knownFailure);
                } else {
                    ADD_FAILURE()
                        << "Exception thrown with message '" << e.what()
                        << "' in '" << description << context;
                }
                continue;
            }
#endif

            if (knownFailure) {
                if (valid != shouldValidate ||
                    validWithResults != shouldValidate) {
                    failed.insert(knownFailure);
                }
                continue;
            }

            EXPECT_EQ(shouldValidate, valid)
                << "Failed while testing validate() function in '"
                << description << context;
            EXPECT_EQ(shouldValidate, validWithResults)
                << "Failed while testing validate() function with results in '"
                << description << context;
        }
    }
};

} // namespace

TEST_P(TestValidator, TestSuiteFile)
{
    const Dialect &dialect = *GetParam().dialect;
    const std::string file = GetParam().file;
    const std::string path =
        (GetParam().local ? LOCAL_TEST_SUITE_DIR : TEST_SUITE_DIR) + file;

    const std::vector<TestCaseInfo> testCases = loadTestCases(path, dialect);
    checkKnownFailures(dialect, file, testCases);
    if (testCases.empty()) {
        GTEST_SKIP() << "No test cases for " << dialect.name;
    }

    std::set<const KnownFailure *> failed;
#ifdef VALIJSON_BUILD_JSON11_ADAPTER
    processTestCases<valijson::adapters::Json11Adapter>(path, dialect, file,
                                                        testCases, failed);
#endif
    processTestCases<valijson::adapters::JsonCppAdapter>(path, dialect, file,
                                                         testCases, failed);
    processTestCases<valijson::adapters::RapidJsonAdapter>(path, dialect, file,
                                                           testCases, failed);
    processTestCases<valijson::adapters::PicoJsonAdapter>(path, dialect, file,
                                                          testCases, failed);
    processTestCases<valijson::adapters::NlohmannJsonAdapter>(
        path, dialect, file, testCases, failed);
#ifdef VALIJSON_BUILD_POCO_ADAPTER
    processTestCases<valijson::adapters::PocoJsonAdapter>(path, dialect, file,
                                                          testCases, failed);
#endif

    checkKnownFailuresFailed(dialect, file, failed);
}

INSTANTIATE_TEST_SUITE_P(JsonSchemaTestSuite, TestValidator,
                         testing::ValuesIn(allTestSuiteFiles()),
                         testSuiteFileName);
