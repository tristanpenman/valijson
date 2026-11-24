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
using valijson::internal::hasRepeatedQuantifier;
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

TEST(TestRegex, AcceptsLongAlternations)
{
    // Each alternative is a short sequence, however many there are
    EXPECT_FALSE(isRegexTooComplex("^(" + repeat("abc|", 1500) + "abc)$"));
}

TEST(TestRegex, AcceptsNonGreedyQuantifiers)
{
    EXPECT_FALSE(hasRepeatedQuantifier("a+?"));
    EXPECT_FALSE(hasRepeatedQuantifier("a*?"));
    EXPECT_FALSE(hasRepeatedQuantifier("a??"));
    EXPECT_FALSE(hasRepeatedQuantifier("a{2}?"));
    EXPECT_FALSE(hasRepeatedQuantifier("a{2,}?"));
    EXPECT_FALSE(hasRepeatedQuantifier("a{2,3}?b+?"));
}

TEST(TestRegex, AcceptsQuantifiedGroups)
{
    EXPECT_FALSE(hasRepeatedQuantifier("(?:ab)+"));
    EXPECT_FALSE(hasRepeatedQuantifier("(?=a)b*"));
    EXPECT_FALSE(hasRepeatedQuantifier("(a+)?"));
}

TEST(TestRegex, AcceptsTypicalPatterns)
{
    EXPECT_FALSE(isRegexTooComplex(""));
    EXPECT_FALSE(isRegexTooComplex("^[a-z0-9-]+$"));
    EXPECT_FALSE(isRegexTooComplex(
        "^(([Cc][Tt][Rr][Ll]|[Ss][Hh][Ii][Ff][Tt]|[Aa][Ll][Tt])(\\+)?){0,3}"));
    EXPECT_FALSE(isRegexTooComplex("^(\\([0-9]{3}\\))?[0-9]{3}-[0-9]{4}$"));
}

TEST(TestRegex, CountsEscapesAndCharacterClassesAsSingleTerms)
{
    EXPECT_FALSE(isRegexTooComplex(repeat("[a-z]", 1500)));
    EXPECT_FALSE(isRegexTooComplex(repeat("\\d", 1500)));
    EXPECT_FALSE(isRegexTooComplex(repeat("[\\]()|]", 1000)));
    EXPECT_TRUE(isRegexTooComplex(repeat("[a-z]", 1500) + repeat("a", 600)));
}

TEST(TestRegex, IgnoresBracesThatAreNotQuantifiers)
{
    // In ECMA 262 (Annex B), a brace that does not start a quantifier is a
    // literal character
    EXPECT_FALSE(hasRepeatedQuantifier("a{b}+"));
    EXPECT_FALSE(hasRepeatedQuantifier("a{,2}+"));
    EXPECT_FALSE(hasRepeatedQuantifier("a{2"));
}

TEST(TestRegex, IgnoresEscapedAndCharacterClassQuantifiers)
{
    EXPECT_FALSE(hasRepeatedQuantifier("\\++"));
    EXPECT_FALSE(hasRepeatedQuantifier("a\\*+"));
    EXPECT_FALSE(hasRepeatedQuantifier("[+*?]+"));
    EXPECT_FALSE(hasRepeatedQuantifier("[]+]+"));
}

TEST(TestRegex, IgnoresUnbalancedParentheses)
{
    // Left for the regular expression engine to reject
    EXPECT_FALSE(isRegexTooComplex(repeat(")", 100)));
}

TEST(TestRegex, RejectsDeeplyNestedGroups)
{
    EXPECT_FALSE(isRegexTooComplex(repeat("(", 128) + repeat(")", 128)));
    EXPECT_TRUE(isRegexTooComplex(repeat("(", 129) + repeat(")", 129)));
    EXPECT_TRUE(isRegexTooComplex(repeat("(?:", 129) + repeat(")", 129)));
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

TEST(TestRegex, RejectsRepeatedQuantifiers)
{
    EXPECT_TRUE(hasRepeatedQuantifier("s+++"));
    EXPECT_TRUE(hasRepeatedQuantifier("s**"));
    EXPECT_TRUE(hasRepeatedQuantifier("s+*"));
    EXPECT_TRUE(hasRepeatedQuantifier("s{2}{3}"));
    EXPECT_TRUE(hasRepeatedQuantifier("s{1,2}+"));
    EXPECT_TRUE(hasRepeatedQuantifier("s+??"));
    EXPECT_TRUE(hasRepeatedQuantifier("(a)*+"));
}

// The limits only apply to std::regex
#if VALIJSON_USE_EXCEPTIONS && \
    !(defined(VALIJSON_USE_BOOST_REGEX) && VALIJSON_USE_BOOST_REGEX)

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

TEST(TestRegex, RejectsRepeatedQuantifiersInPattern)
{
    Schema schema;
    valijson::constraints::PatternConstraint constraint;
    constraint.setPattern(std::string("s+++v"));
    schema.addConstraint(constraint);

    Validator validator;
    EXPECT_THROW(validator.validate(schema, StdStringAdapter(std::string(30, 's')), nullptr),
                 std::runtime_error);
}

TEST(TestRegex, RejectsRepeatedQuantifiersInRegexFormat)
{
    // libstdc++ accepts "s+++", but matching with it takes exponential time
    Schema schema;
    valijson::constraints::FormatConstraint constraint;
    constraint.setFormat("regex");
    schema.addConstraint(constraint);

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, StdStringAdapter("s+?"), nullptr));
    EXPECT_FALSE(validator.validate(schema, StdStringAdapter("s+++"), nullptr));
}

#endif
