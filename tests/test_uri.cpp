#include <string>

#include <gtest/gtest.h>

#include <valijson/internal/uri.hpp>

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
