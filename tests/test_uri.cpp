#include <string>

#include <gtest/gtest.h>

#include <valijson/internal/uri.hpp>

using valijson::internal::uri::isUri;
using valijson::internal::uri::isUriReference;
using valijson::internal::uri::resolveRelativeUri;

namespace {

struct UriResolutionTestCase
{
    std::string baseUri;
    std::string relativeUri;
    std::string expectedUri;
};

class TestUriResolution
    : public testing::TestWithParam<UriResolutionTestCase>
{
};

TEST_P(TestUriResolution, ResolvesRelativeUri)
{
    const UriResolutionTestCase &testCase = GetParam();

    EXPECT_EQ(
            testCase.expectedUri,
            resolveRelativeUri(testCase.baseUri, testCase.relativeUri));
}

}  // namespace

INSTANTIATE_TEST_SUITE_P(
        Rfc3986ImplementedNormalExamples,
        TestUriResolution,
        testing::Values(
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g", "http://a/b/c/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "./g", "http://a/b/c/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g/", "http://a/b/c/g/"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "/g", "http://a/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "?y", "http://a/b/c/d;p?y"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g?y", "http://a/b/c/g?y"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g#s", "http://a/b/c/g#s"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g?y#s", "http://a/b/c/g?y#s"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", ";x", "http://a/b/c/;x"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g;x", "http://a/b/c/g;x"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g;x?y#s", "http://a/b/c/g;x?y#s"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "", "http://a/b/c/d;p?q"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "./", "http://a/b/c/"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "../", "http://a/b/"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "../g", "http://a/b/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "../..", "http://a/"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "../../", "http://a/"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "../../g", "http://a/g"}));

INSTANTIATE_TEST_SUITE_P(
        Rfc3986ImplementedAbnormalExamples,
        TestUriResolution,
        testing::Values(
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "../../../g", "http://a/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "../../../../g", "http://a/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "/./g", "http://a/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "/../g", "http://a/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g.", "http://a/b/c/g."},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", ".g", "http://a/b/c/.g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g..", "http://a/b/c/g.."},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "..g", "http://a/b/c/..g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "./../g", "http://a/b/g"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g/./h", "http://a/b/c/g/h"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g/../h", "http://a/b/c/h"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g;x=1/./y", "http://a/b/c/g;x=1/y"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g;x=1/../y", "http://a/b/c/y"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g?y/./x", "http://a/b/c/g?y/./x"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g?y/../x", "http://a/b/c/g?y/../x"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g#s/./x", "http://a/b/c/g#s/./x"},
                UriResolutionTestCase{
                        "http://a/b/c/d;p?q", "g#s/../x", "http://a/b/c/g#s/../x"}));

INSTANTIATE_TEST_SUITE_P(
        ValijsonUriResolution,
        TestUriResolution,
        testing::Values(
                UriResolutionTestCase{
                        "http://example.com/schema.json",
                        "#/definitions/id",
                        "http://example.com/schema.json#/definitions/id"},
                UriResolutionTestCase{
                        "http://example.com/schemas/root.json?version=1#root",
                        "?version=2",
                        "http://example.com/schemas/root.json?version=2"},
                UriResolutionTestCase{
                        "http://example.com/schemas/root.json?version=1#root",
                        "child.json#/definitions/name",
                        "http://example.com/schemas/child.json#/definitions/name"},
                UriResolutionTestCase{
                        "http://example.com/schemas/",
                        "../common/types.json",
                        "http://example.com/common/types.json"},
                UriResolutionTestCase{
                        "http://example.com",
                        "schema.json",
                        "http://example.com/schema.json"},
                UriResolutionTestCase{
                        "urn:mvn:example.schema.common:status:1.1.0",
                        "",
                        "urn:mvn:example.schema.common:status:1.1.0"},
                UriResolutionTestCase{
                        "http://example.com/schema.json",
                        "urn:mvn:example.schema.common:status:1.1.0",
                        "urn:mvn:example.schema.common:status:1.1.0"},
                UriResolutionTestCase{
                        "schemas/",
                        "child.json",
                        "schemas/child.json"},
                UriResolutionTestCase{
                        "json-schemas/schema-B.json",
                        "schema-C.json",
                        "json-schemas/schema-C.json"},
                UriResolutionTestCase{
                        "schema-B.json",
                        "schema-C.json",
                        "schema-C.json"}));

// Regression test for an out-of-bounds read in normalised path handling.
INSTANTIATE_TEST_SUITE_P(
        RelativeScopeCollapsingToEmptyPath,
        TestUriResolution,
        testing::Values(
                UriResolutionTestCase{"foo", "./", "/"},
                UriResolutionTestCase{"a/b", "../", "/"},
                UriResolutionTestCase{"dir/file.json", "../", "/"}));

TEST(UriSyntax, AcceptsUris)
{
    EXPECT_TRUE(isUri("http://foo.bar/?baz=qux#quux"));
    EXPECT_TRUE(isUri("http://foo.com/blah_(wikipedia)_blah#cite-1"));
    EXPECT_TRUE(isUri("http://foo.bar/?q=Test%20URL-encoded%20stuff"));
    EXPECT_TRUE(isUri("http://-.~_!$&'()*+,;=:%40:80%2f::::::@example.com"));
    EXPECT_TRUE(isUri("http://user@example.com:8080/path"));
    EXPECT_TRUE(isUri("http://example.com:/path"));
    EXPECT_TRUE(isUri("http://999.999.999.999/"));
    EXPECT_TRUE(isUri("ldap://[2001:db8::7]/c=GB?objectClass?one"));
    EXPECT_TRUE(isUri("http://[v1.fe80::a+en1]/"));
    EXPECT_TRUE(isUri("file:///etc/hosts"));
    EXPECT_TRUE(isUri("mailto:John.Doe@example.com"));
    EXPECT_TRUE(isUri("tel:+1-816-555-1212"));
    EXPECT_TRUE(isUri("urn:oasis:names:specification:docbook:dtd:xml:4.1.2"));
    EXPECT_TRUE(isUri("a:"));
}

TEST(UriSyntax, RejectsInvalidSchemes)
{
    EXPECT_FALSE(isUri(""));
    EXPECT_FALSE(isUri("abc"));
    EXPECT_FALSE(isUri("/abc"));
    EXPECT_FALSE(isUri("//foo.bar/?baz=qux#quux"));
    EXPECT_FALSE(isUri(":// should fail"));
    EXPECT_FALSE(isUri("bar,baz:foo"));
    EXPECT_FALSE(isUri("1http://example.com"));
    EXPECT_FALSE(isUri("ht_tp://example.com"));
}

TEST(UriSyntax, RejectsInvalidAuthorities)
{
    EXPECT_FALSE(isUri("http:// shouldfail.com"));
    EXPECT_FALSE(isUri("https://[@example.org/test.txt"));
    EXPECT_FALSE(isUri("http://example.com:abc/path"));
    EXPECT_FALSE(isUri("http://a@b@example.com/"));
    EXPECT_FALSE(isUri("http://[2001:db8::7/"));
    EXPECT_FALSE(isUri("http://[example.com]/"));
    EXPECT_FALSE(isUri("http://[::1]x/"));
}

TEST(UriSyntax, RejectsInvalidCharacters)
{
    EXPECT_FALSE(isUri("\\\\WINDOWS\\fileshare"));
    EXPECT_FALSE(isUri("https://example.org/foobar\xc2\xae.txt"));
    EXPECT_FALSE(isUri("https://example.org/foobar\\.txt"));
    EXPECT_FALSE(isUri("https://example.org/foobar\".txt"));
    EXPECT_FALSE(isUri("https://example.org/foobar<>.txt"));
    EXPECT_FALSE(isUri("https://example.org/foobar{}.txt"));
    EXPECT_FALSE(isUri("https://example.org/foobar^.txt"));
    EXPECT_FALSE(isUri("https://example.org/foobar`.txt"));
    EXPECT_FALSE(isUri("https://example.org/foo bar.txt"));
    EXPECT_FALSE(isUri("https://example.org/foobar|.txt"));
    EXPECT_FALSE(isUri("http://example.com/#a#b"));
    EXPECT_FALSE(isUri(std::string("http://example.com/\0", 20)));
}

TEST(UriSyntax, RejectsInvalidPercentEncoding)
{
    EXPECT_FALSE(isUri("http://example.com/%6G"));
    EXPECT_FALSE(isUri("http://example.com/%A"));
    EXPECT_FALSE(isUri("http://example.com/%"));
    EXPECT_FALSE(isUri("http://example.com/?q=%"));
    EXPECT_FALSE(isUri("http://example.com/#%zz"));
}

TEST(UriSyntax, AcceptsUriReferences)
{
    EXPECT_TRUE(isUriReference("http://foo.bar/?baz=qux#quux"));
    EXPECT_TRUE(isUriReference("//foo.bar/?baz=qux#quux"));
    EXPECT_TRUE(isUriReference("/abc"));
    EXPECT_TRUE(isUriReference("abc"));
    EXPECT_TRUE(isUriReference("../abc/def"));
    EXPECT_TRUE(isUriReference("./a:b"));
    EXPECT_TRUE(isUriReference("abc/d:e"));
    EXPECT_TRUE(isUriReference("?query"));
    EXPECT_TRUE(isUriReference("#fragment"));
    EXPECT_TRUE(isUriReference(""));
}

TEST(UriSyntax, RejectsInvalidUriReferences)
{
    EXPECT_FALSE(isUriReference("\\\\WINDOWS\\fileshare"));
    EXPECT_FALSE(isUriReference("#frag\\ment"));
    EXPECT_FALSE(isUriReference("/foobar\xc2\xae.txt"));
    EXPECT_FALSE(isUriReference("https://example.org/foobar\\.txt"));

    // The first segment of a relative path must not contain a colon
    EXPECT_FALSE(isUriReference("1a:b"));
    EXPECT_FALSE(isUriReference(":abc"));
}
