#include <string>

#include <gtest/gtest.h>

#include <valijson/internal/email.hpp>

using valijson::internal::isEmail;

TEST(Email, AcceptsDotAtomAddresses)
{
    EXPECT_TRUE(isEmail("joe.bloggs@example.com"));
    EXPECT_TRUE(isEmail("te.s.t@example.com"));
    EXPECT_TRUE(isEmail("~test@example.com"));
    EXPECT_TRUE(isEmail("test~@example.com"));
    EXPECT_TRUE(isEmail("user+tag@example.com"));
    EXPECT_TRUE(isEmail("!#$%&'*+-/=?^_`{|}~@example.com"));
    EXPECT_TRUE(isEmail("user@localhost"));
}

TEST(Email, AcceptsQuotedLocalParts)
{
    EXPECT_TRUE(isEmail("\"joe bloggs\"@example.com"));
    EXPECT_TRUE(isEmail("\"joe..bloggs\"@example.com"));
    EXPECT_TRUE(isEmail("\"joe@bloggs\"@example.com"));
    EXPECT_TRUE(isEmail("\"joe\\\"bloggs\"@example.com"));
    EXPECT_TRUE(isEmail("\"\"@example.com"));
}

TEST(Email, AcceptsDomainLiterals)
{
    EXPECT_TRUE(isEmail("joe.bloggs@[127.0.0.1]"));
    EXPECT_TRUE(isEmail("joe.bloggs@[IPv6:::1]"));
}

TEST(Email, RejectsMissingParts)
{
    EXPECT_FALSE(isEmail(""));
    EXPECT_FALSE(isEmail("2962"));
    EXPECT_FALSE(isEmail("@example.com"));
    EXPECT_FALSE(isEmail("joe.bloggs@"));
    EXPECT_FALSE(isEmail("@"));
}

TEST(Email, RejectsInvalidDots)
{
    EXPECT_FALSE(isEmail(".test@example.com"));
    EXPECT_FALSE(isEmail("test.@example.com"));
    EXPECT_FALSE(isEmail("te..st@example.com"));
    EXPECT_FALSE(isEmail("test@.example.com"));
    EXPECT_FALSE(isEmail("test@example.com."));
    EXPECT_FALSE(isEmail("test@example..com"));
}

TEST(Email, RejectsInvalidCharacters)
{
    EXPECT_FALSE(isEmail("joe bloggs@example.com"));
    EXPECT_FALSE(isEmail("joe@bloggs@example.com"));
    EXPECT_FALSE(isEmail("user1@oceania.org, user2@oceania.org"));
    EXPECT_FALSE(isEmail("\"Winston Smith\" <winston.smith@recdep.minitrue> "
                         "(Records Department)"));
    EXPECT_FALSE(isEmail("joe(comment)@example.com"));
    EXPECT_FALSE(isEmail("j\xc3\xb6"
                         "e@example.com"));
    EXPECT_FALSE(isEmail(std::string("joe\0@example.com", 16)));
    EXPECT_FALSE(isEmail(std::string("joe@example\0.com", 16)));
}

TEST(Email, RejectsInvalidQuotedLocalParts)
{
    EXPECT_FALSE(isEmail("\"@example.com"));
    EXPECT_FALSE(isEmail("\"joe\"bloggs\"@example.com"));
    EXPECT_FALSE(isEmail("\"joe\\\"@example.com"));
    EXPECT_FALSE(isEmail("\"joe\"bloggs@example.com"));
}

TEST(Email, RejectsInvalidDomainLiterals)
{
    EXPECT_FALSE(isEmail("joe@[127.0.0.1"));
    EXPECT_FALSE(isEmail("joe@[127.0.0.1]x"));
    EXPECT_FALSE(isEmail("joe@[127 0 0 1]"));
    EXPECT_FALSE(isEmail("joe@[[127.0.0.1]]"));
}
