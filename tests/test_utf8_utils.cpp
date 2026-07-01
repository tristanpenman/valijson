#include <string>

#include <gtest/gtest.h>
#include <valijson/utils/utf8_utils.hpp>

class TestUtf8Utils : public testing::Test
{
};

// Helper that derives the byte length of a string literal automatically, so
// the tests below read much like the original null-terminated expectations.
#define U8_STRLEN_LITERAL(literal) \
    valijson::utils::u8_strlen((literal), sizeof(literal) - 1)

TEST_F(TestUtf8Utils, Utf8StringLength)
{
    EXPECT_EQ(U8_STRLEN_LITERAL(""), 0);
    EXPECT_EQ(U8_STRLEN_LITERAL("a"), 1);
    EXPECT_EQ(U8_STRLEN_LITERAL("abc"), 3);

    // U+0416
    EXPECT_EQ(U8_STRLEN_LITERAL("\xD0\x96"), 1);

    // U+0915
    EXPECT_EQ(U8_STRLEN_LITERAL("\xE0\xA4\x95"), 1);

    // U+10348
    EXPECT_EQ(U8_STRLEN_LITERAL("\xF0\x90\x8D\x88"), 1);

    // U+0915 + U+0416
    EXPECT_EQ(U8_STRLEN_LITERAL("\xE0\xA4\x95\xD0\x96"), 2);

    // incomplete U+0416 at the end
    EXPECT_EQ(U8_STRLEN_LITERAL("\xD0"), 1);

    // incomplete U+0416 in the middle
    EXPECT_EQ(U8_STRLEN_LITERAL("\320abc"), 4);

    // incomplete U+0915 at the end
    EXPECT_EQ(U8_STRLEN_LITERAL("\xE0\xA4"), 1);

    // incomplete U+0915 in the middle
    EXPECT_EQ(U8_STRLEN_LITERAL("\xE0\244abc"), 4);

    // U+DFFF
    EXPECT_EQ(U8_STRLEN_LITERAL("\xED\xBF\xBF"), 1);

    // Overlong encoding for U+0000
    EXPECT_EQ(U8_STRLEN_LITERAL("\xC0\x80"), 1);

    // U+110000 (out of Unicode range)
    EXPECT_EQ(U8_STRLEN_LITERAL("\xF5\x80\x80\x80"), 1);

    // 0xE0 + 0xA4 repeating 9 times
    EXPECT_EQ(U8_STRLEN_LITERAL("\340\244\244\244\244\244\244\244\244\244"), 5);
}

TEST_F(TestUtf8Utils, Utf8StringLengthRespectsExplicitLength)
{
    using valijson::utils::u8_strlen;

    // Only the first byte of a 2-byte sequence is within the given length, so
    // the trailing continuation byte must be ignored.
    EXPECT_EQ(u8_strlen("\xD0\x96", 1), 1);

    // A zero length yields a count of zero, regardless of buffer contents.
    EXPECT_EQ(u8_strlen("abc", 0), 0);

    // Only a prefix of an ASCII string is counted when a shorter length is
    // provided.
    EXPECT_EQ(u8_strlen("abc", 2), 2);
}

TEST_F(TestUtf8Utils, Utf8StringLengthCountsEmbeddedNulls)
{
    using valijson::utils::u8_strlen;

    // Unlike a null-terminated count, an embedded null byte is a valid 1-byte
    // (ASCII) character and the bytes following it are still counted.
    const std::string withNull("a\0bc", 4);
    EXPECT_EQ(u8_strlen(withNull.c_str(), withNull.size()), 4);

    // A buffer consisting entirely of null bytes counts each one.
    const std::string nulls("\0\0\0", 3);
    EXPECT_EQ(u8_strlen(nulls.c_str(), nulls.size()), 3);

    // A null byte appearing in the middle of a multi-byte sequence terminates
    // that sequence early, but the null itself and subsequent bytes are still
    // counted as characters.
    const std::string nullInSequence("\xE0\x00\x95", 3);
    EXPECT_EQ(u8_strlen(nullInSequence.c_str(), nullInSequence.size()), 3);
}
