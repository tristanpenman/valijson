#include <string>

#include <gtest/gtest.h>

#include <valijson/adapters/std_string_adapter.hpp>
#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/internal/regex.hpp>
#include <valijson/schema.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::Validator;
using valijson::adapters::StdStringAdapter;
using valijson::internal::isRegexTooComplex;

namespace {

std::string repeat(const std::string &s, size_t count)
{
    std::string result;
    for (size_t i = 0; i < count; i++) {
        result += s;
    }
    return result;
}

} // namespace

TEST(TestRegex, AcceptsTypicalPatterns)
{
    EXPECT_FALSE(isRegexTooComplex(""));
    EXPECT_FALSE(isRegexTooComplex("^[a-z0-9-]+$"));
    EXPECT_FALSE(isRegexTooComplex(
        "^(([Cc][Tt][Rr][Ll]|[Ss][Hh][Ii][Ff][Tt]|[Aa][Ll][Tt])(\\+)?){0,3}"));
    EXPECT_FALSE(isRegexTooComplex("^(\\([0-9]{3}\\))?[0-9]{3}-[0-9]{4}$"));
}

TEST(TestRegex, AcceptsLongAlternations)
{
    // Each alternative is a short sequence, however many there are
    EXPECT_FALSE(isRegexTooComplex("^(" + repeat("abc|", 1500) + "abc)$"));
}

TEST(TestRegex, CountsEscapesAndCharacterClassesAsSingleTerms)
{
    EXPECT_FALSE(isRegexTooComplex(repeat("[a-z]", 1500)));
    EXPECT_FALSE(isRegexTooComplex(repeat("\\d", 1500)));
    EXPECT_FALSE(isRegexTooComplex(repeat("[\\]()|]", 1000)));
    EXPECT_TRUE(isRegexTooComplex(repeat("[a-z]", 1500) + repeat("a", 600)));
}

TEST(TestRegex, RejectsLongPatterns)
{
    EXPECT_FALSE(isRegexTooComplex(repeat("a|", 4095) + "a"));
    EXPECT_TRUE(isRegexTooComplex(repeat("a|", 4096) + "a"));
}

TEST(TestRegex, RejectsLongSequences)
{
    EXPECT_FALSE(isRegexTooComplex(repeat("a", 2048)));
    EXPECT_TRUE(isRegexTooComplex(repeat("a", 2049)));
}

TEST(TestRegex, RejectsDeeplyNestedGroups)
{
    EXPECT_FALSE(isRegexTooComplex(repeat("(", 128) + repeat(")", 128)));
    EXPECT_TRUE(isRegexTooComplex(repeat("(", 129) + repeat(")", 129)));
    EXPECT_TRUE(isRegexTooComplex(repeat("(?:", 129) + repeat(")", 129)));
}

TEST(TestRegex, IgnoresUnbalancedParentheses)
{
    // Left for the regular expression engine to reject
    EXPECT_FALSE(isRegexTooComplex(repeat(")", 100)));
}

// The limits only apply to std::regex
#if VALIJSON_USE_EXCEPTIONS && \
    !(defined(VALIJSON_USE_BOOST_REGEX) && VALIJSON_USE_BOOST_REGEX)

TEST(TestRegex, RejectsComplexRegexFormat)
{
    // A string that is too complex to be compiled is not a valid regular
    // expression, rather than causing a stack overflow
    Schema schema;
    valijson::constraints::FormatConstraint constraint;
    constraint.setFormat("regex");
    schema.addConstraint(constraint);

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, StdStringAdapter("(a)"), nullptr));
    EXPECT_FALSE(validator.validate(
        schema, StdStringAdapter(repeat("(", 30000) + repeat(")", 30000)),
        nullptr));
}

TEST(TestRegex, RejectsComplexPattern)
{
    Schema schema;
    valijson::constraints::PatternConstraint constraint;
    constraint.setPattern(repeat("(", 30000) + repeat(")", 30000));
    schema.addConstraint(constraint);

    Validator validator;
    EXPECT_THROW(validator.validate(schema, StdStringAdapter("a"), nullptr),
                 std::runtime_error);
}

#endif
