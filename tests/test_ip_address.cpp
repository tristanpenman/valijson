#include <string>

#include <gtest/gtest.h>

#include <valijson/internal/ip_address.hpp>

using valijson::internal::isIpv4DottedQuad;
using valijson::internal::isIpv6Address;

namespace {

bool isDottedQuad(const std::string &s)
{
    return isIpv4DottedQuad(s, 0, s.size());
}

} // namespace

TEST(IpAddress, AcceptsDottedQuads)
{
    EXPECT_TRUE(isDottedQuad("0.0.0.0"));
    EXPECT_TRUE(isDottedQuad("192.168.0.1"));
    EXPECT_TRUE(isDottedQuad("255.255.255.255"));

    // RFC 2673 allows one to three digits, including leading zeros
    EXPECT_TRUE(isDottedQuad("010.001.000.001"));
}

TEST(IpAddress, RejectsInvalidDottedQuads)
{
    EXPECT_FALSE(isDottedQuad(""));
    EXPECT_FALSE(isDottedQuad("1.2.3"));
    EXPECT_FALSE(isDottedQuad("1.2.3.4.5"));
    EXPECT_FALSE(isDottedQuad("1.2.3."));
    EXPECT_FALSE(isDottedQuad(".1.2.3"));
    EXPECT_FALSE(isDottedQuad("1..2.3"));
    EXPECT_FALSE(isDottedQuad("256.0.0.0"));
    EXPECT_FALSE(isDottedQuad("0.0.0.0255"));
    EXPECT_FALSE(isDottedQuad("1.2.3.ff"));
    EXPECT_FALSE(isDottedQuad("1.2.3.4 "));
}

TEST(IpAddress, ChecksOnlyTheGivenRangeOfDottedQuads)
{
    const std::string s = "::ffff:192.168.0.1";
    EXPECT_TRUE(isIpv4DottedQuad(s, 7, s.size()));
    EXPECT_FALSE(isIpv4DottedQuad(s, 7, s.size() - 1));
}

TEST(IpAddress, AcceptsIpv6Addresses)
{
    EXPECT_TRUE(isIpv6Address("1:2:3:4:5:6:7:8"));
    EXPECT_TRUE(isIpv6Address("1080:0:0:0:8:800:200C:417A"));
    EXPECT_TRUE(isIpv6Address("FEDC:BA98:7654:3210:fedc:ba98:7654:3210"));
    EXPECT_TRUE(isIpv6Address("0001:02:003:4:5:6:7:8"));
    EXPECT_TRUE(isIpv6Address("0:0:0:0:0:0:0:0"));
    EXPECT_TRUE(isIpv6Address("ffff:FFFF:0a0A:AbCd:1234:0001:0:9"));
}

TEST(IpAddress, AcceptsCompressedIpv6Addresses)
{
    EXPECT_TRUE(isIpv6Address("::"));
    EXPECT_TRUE(isIpv6Address("::1"));
    EXPECT_TRUE(isIpv6Address("1::"));
    EXPECT_TRUE(isIpv6Address("1080::8:800:200C:417A"));

    // '::' stands for at least one group
    EXPECT_TRUE(isIpv6Address("::2:3:4:5:6:7:8"));
    EXPECT_TRUE(isIpv6Address("1:2:3:4:5:6:7::"));
    EXPECT_FALSE(isIpv6Address("::1:2:3:4:5:6:7:8"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:8::"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4::5:6:7:8"));
    EXPECT_TRUE(isIpv6Address("1:2:3:4:5:6::8"));
    EXPECT_TRUE(isIpv6Address("1::3:4:5:6:7:8"));
    EXPECT_TRUE(isIpv6Address("1:2:3:4:5:6:7::"));
}

TEST(IpAddress, AcceptsIpv6AddressesWithIpv4Suffix)
{
    EXPECT_TRUE(isIpv6Address("0:0:0:0:0:0:13.1.68.3"));
    EXPECT_TRUE(isIpv6Address("::13.1.68.3"));
    EXPECT_TRUE(isIpv6Address("::FFFF:129.144.52.38"));
    EXPECT_TRUE(isIpv6Address("::ffff:010.001.000.001"));

    // The IPv4 address takes the place of two groups, and must come last
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:1.2.3.4"));
    EXPECT_FALSE(isIpv6Address("1.2.3.4::1"));
    EXPECT_FALSE(isIpv6Address("::1.2.3.4:1"));
    EXPECT_FALSE(isIpv6Address("::256.0.0.0"));
    EXPECT_FALSE(isIpv6Address("::1.2.3"));
    EXPECT_FALSE(isIpv6Address("::1.2.3.4.5"));
    EXPECT_FALSE(isIpv6Address("::1.2.3."));
    EXPECT_FALSE(isIpv6Address("::1..3.4"));
    EXPECT_FALSE(isIpv6Address("::1.2.3.256"));
    EXPECT_FALSE(isIpv6Address("::1.2.3.04x"));
}

TEST(IpAddress, RejectsInvalidIpv6Addresses)
{
    EXPECT_FALSE(isIpv6Address(""));
    EXPECT_FALSE(isIpv6Address(":"));
    EXPECT_FALSE(isIpv6Address(":::"));
    EXPECT_FALSE(isIpv6Address(":1::"));
    EXPECT_FALSE(isIpv6Address("1::2:"));
    EXPECT_FALSE(isIpv6Address("1::2::3"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:8:9"));
    EXPECT_FALSE(isIpv6Address("12345::"));
    EXPECT_FALSE(isIpv6Address("::g"));
    EXPECT_FALSE(isIpv6Address("fe80::/64"));
    EXPECT_FALSE(isIpv6Address("fe80::a%eth1"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:12345"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:gggg"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:+8"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:-8"));
    EXPECT_FALSE(isIpv6Address(" 1:2:3:4:5:6:7:8"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:8 "));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:8\n"));
    EXPECT_FALSE(isIpv6Address("1:::2"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:::"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:8::"));
    EXPECT_FALSE(isIpv6Address("::1:2:3:4:5:6:7:8"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7:8:9"));
    EXPECT_FALSE(isIpv6Address("1:2:3:4:5:6:7::8"));
}

TEST(IpAddress, RejectsEmbeddedNullBytes)
{
    EXPECT_FALSE(isIpv6Address(std::string("::1\0", 4)));
    EXPECT_FALSE(isIpv6Address(std::string("::\0" "1", 4)));
    EXPECT_FALSE(isIpv6Address(std::string("::1.2.3.4\0", 10)));
}

TEST(IpAddress, RejectsSpecialCharactersEscapesAndUnicode)
{
    // URI delimiters, brackets, quoting, and other punctuation are not part
    // of an IPv6 address literal.
    EXPECT_FALSE(isIpv6Address("[::1]"));
    EXPECT_FALSE(isIpv6Address("::1]"));
    EXPECT_FALSE(isIpv6Address("::1?query"));
    EXPECT_FALSE(isIpv6Address("::1#fragment"));
    EXPECT_FALSE(isIpv6Address("::1\\"));
    EXPECT_FALSE(isIpv6Address("::1\""));
    EXPECT_FALSE(isIpv6Address("::1'"));
    EXPECT_FALSE(isIpv6Address("::1;"));
    EXPECT_FALSE(isIpv6Address("::1,"));

    // Backslash escapes are text, not a way to encode address characters.
    EXPECT_FALSE(isIpv6Address("\\x3a\\x3a1"));
    EXPECT_FALSE(isIpv6Address("::\\u0031"));
    EXPECT_FALSE(isIpv6Address("::\\u{31}"));

    // Non-ASCII digits and letters must not be accepted as hexadecimal.
    EXPECT_FALSE(isIpv6Address("::１"));      // fullwidth digit one
    EXPECT_FALSE(isIpv6Address("::١"));       // Arabic-Indic digit one
    EXPECT_FALSE(isIpv6Address("::ａ"));      // fullwidth Latin small a
    EXPECT_FALSE(isIpv6Address("::é"));
    EXPECT_FALSE(isIpv6Address("::😀"));
    EXPECT_FALSE(isIpv6Address("１:2:3:4:5:6:7:8"));
    EXPECT_FALSE(isIpv6Address("::1.2.3.４"));
}
