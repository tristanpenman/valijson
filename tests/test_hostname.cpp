#include <string>

#include <gtest/gtest.h>

#include <valijson/internal/hostname.hpp>

using valijson::internal::isHostname;

TEST(Hostname, AcceptsHostnames)
{
    EXPECT_TRUE(isHostname("www.example.com"));
    EXPECT_TRUE(isHostname("localhost"));
    EXPECT_TRUE(isHostname("WWW.Example.COM"));
    EXPECT_TRUE(isHostname("host-name.example"));
    EXPECT_TRUE(isHostname("ab--cd.example"));

    // RFC 1123 allows labels to start with a digit
    EXPECT_TRUE(isHostname("1host.example"));
    EXPECT_TRUE(isHostname("123.example"));

    // A-labels are only checked against this syntax
    EXPECT_TRUE(isHostname("xn--4gbwdl.xn--wgbh1c"));
}

TEST(Hostname, RejectsInvalidLabels)
{
    EXPECT_FALSE(isHostname("-hostname"));
    EXPECT_FALSE(isHostname("hostname-"));
    EXPECT_FALSE(isHostname("www.-example.com"));
    EXPECT_FALSE(isHostname("www.example-.com"));
    EXPECT_FALSE(isHostname("host_name"));
    EXPECT_FALSE(isHostname("host name"));
    EXPECT_FALSE(isHostname("example\xef\xbc\x8e" "com"));
}

TEST(Hostname, RejectsEmptyLabels)
{
    EXPECT_FALSE(isHostname(""));
    EXPECT_FALSE(isHostname("."));
    EXPECT_FALSE(isHostname(".example"));
    EXPECT_FALSE(isHostname("example."));
    EXPECT_FALSE(isHostname("www..example"));
}

TEST(Hostname, ChecksLengthLimits)
{
    // Labels have at most 63 characters
    EXPECT_TRUE(isHostname(std::string(63, 'a') + ".com"));
    EXPECT_FALSE(isHostname(std::string(64, 'a') + ".com"));

    // Host names have at most 255 characters
    std::string longest;
    while (longest.size() + 64 <= 255) {
        longest += std::string(63, 'a') + ".";
    }
    longest += std::string(255 - longest.size(), 'b');
    ASSERT_EQ(255u, longest.size());
    EXPECT_TRUE(isHostname(longest));
    EXPECT_FALSE(isHostname(longest + "b"));
}

TEST(Hostname, RejectsEmbeddedNullBytes)
{
    EXPECT_FALSE(isHostname(std::string("example\0", 8)));
    EXPECT_FALSE(isHostname(std::string("exa\0mple", 8)));
}
