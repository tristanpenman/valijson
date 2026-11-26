#include <string>

#include <gtest/gtest.h>

#include <valijson/internal/uri_template.hpp>

using valijson::internal::uri_template::isUriTemplate;

TEST(TestUriTemplate, AcceptsExpressionsWithEachOperator)
{
    EXPECT_TRUE(isUriTemplate("{var}"));
    for (const char op : std::string("+#./;?&=,!@|")) {
        EXPECT_TRUE(isUriTemplate(std::string("{") + op + "var}")) << op;
    }
}

TEST(TestUriTemplate, AcceptsLiterals)
{
    EXPECT_TRUE(isUriTemplate(""));
    EXPECT_TRUE(isUriTemplate("http://example.com/dictionary"));
    EXPECT_TRUE(isUriTemplate("a%41b"));
    EXPECT_TRUE(isUriTemplate("a'b"));
    EXPECT_TRUE(isUriTemplate("a\xc2\xa0" "b"));         // U+00A0
    EXPECT_TRUE(isUriTemplate("a\xf0\x9f\x98\x80" "b")); // U+1F600
    EXPECT_TRUE(isUriTemplate("a\xee\x80\x80" "b"));     // U+E000, private use
    EXPECT_TRUE(isUriTemplate("a\xf3\xb0\x80\x80" "b")); // U+F0000, private use
}

TEST(TestUriTemplate, AcceptsModifiers)
{
    EXPECT_TRUE(isUriTemplate("{var*}"));
    EXPECT_TRUE(isUriTemplate("{var:1}"));
    EXPECT_TRUE(isUriTemplate("{var:1000}"));
    EXPECT_TRUE(isUriTemplate("{var:9999,list*}"));
}

TEST(TestUriTemplate, AcceptsTemplates)
{
    EXPECT_TRUE(isUriTemplate("http://example.com/dictionary/{term:1}/{term}"));
    EXPECT_TRUE(isUriTemplate("dictionary/{term:1}/{term}"));
    EXPECT_TRUE(isUriTemplate("{/path*}{?x,y}{&z}{#fragment}"));
}

TEST(TestUriTemplate, AcceptsVariableNames)
{
    EXPECT_TRUE(isUriTemplate("{a.b}"));
    EXPECT_TRUE(isUriTemplate("{a_b.c1}"));
    EXPECT_TRUE(isUriTemplate("{%41}"));
    EXPECT_TRUE(isUriTemplate("{x,y}"));
}

TEST(TestUriTemplate, RejectsInvalidExpressions)
{
    EXPECT_FALSE(isUriTemplate("{}"));
    EXPECT_FALSE(isUriTemplate("{+}"));
    EXPECT_FALSE(isUriTemplate("{a,,b}"));
    EXPECT_FALSE(isUriTemplate("{a,}"));
    EXPECT_FALSE(isUriTemplate("{,+var}"));
    EXPECT_FALSE(isUriTemplate("{var=def}"));
    EXPECT_FALSE(isUriTemplate("{var"));
    EXPECT_FALSE(isUriTemplate("http://example.com/dictionary/{term:1}/{term"));
    EXPECT_FALSE(isUriTemplate("{a{b}}"));
}

TEST(TestUriTemplate, RejectsInvalidLiterals)
{
    EXPECT_FALSE(isUriTemplate("foo}bar"));
    EXPECT_FALSE(isUriTemplate("a b"));
    EXPECT_FALSE(isUriTemplate("a\x7f" "b"));
    EXPECT_FALSE(isUriTemplate(std::string("a\0b", 3)));
    EXPECT_FALSE(isUriTemplate("a\"b"));
    EXPECT_FALSE(isUriTemplate("a<b"));
    EXPECT_FALSE(isUriTemplate("a%"));
    EXPECT_FALSE(isUriTemplate("a%4"));
    EXPECT_FALSE(isUriTemplate("a%GG"));
}

TEST(TestUriTemplate, RejectsInvalidModifiers)
{
    EXPECT_FALSE(isUriTemplate("{v:0}"));
    EXPECT_FALSE(isUriTemplate("{v:01}"));
    EXPECT_FALSE(isUriTemplate("{v:10000}"));
    EXPECT_FALSE(isUriTemplate("{v:}"));
    EXPECT_FALSE(isUriTemplate("{var:1*}"));
    EXPECT_FALSE(isUriTemplate("{var**}"));
}

TEST(TestUriTemplate, RejectsInvalidUtf8AndCharacters)
{
    EXPECT_FALSE(isUriTemplate("a\x80" "b"));             // continuation byte
    EXPECT_FALSE(isUriTemplate("a\xc2"));                 // truncated
    EXPECT_FALSE(isUriTemplate("a\xc0\xa0" "b"));         // overlong
    EXPECT_FALSE(isUriTemplate("a\xed\xa0\x80" "b"));     // surrogate
    EXPECT_FALSE(isUriTemplate("a\xc2\x9f" "b"));         // U+009F, control
    EXPECT_FALSE(isUriTemplate("a\xef\xbf\xbe" "b"));     // U+FFFE
    EXPECT_FALSE(isUriTemplate("a\xf0\x9f\xbf\xbe" "b")); // U+1FFFE
}

TEST(TestUriTemplate, RejectsInvalidVariableNames)
{
    EXPECT_FALSE(isUriTemplate("{a.}"));
    EXPECT_FALSE(isUriTemplate("{.a.}"));
    EXPECT_FALSE(isUriTemplate("{a..b}"));
    EXPECT_FALSE(isUriTemplate("{%}"));
    EXPECT_FALSE(isUriTemplate("{%4}"));
    EXPECT_FALSE(isUriTemplate("{%GG}"));
    EXPECT_FALSE(isUriTemplate("{a-b}"));
}
