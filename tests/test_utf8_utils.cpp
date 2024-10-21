#include <gtest/gtest.h>
#include <valijson/utils/utf8_utils.hpp>

class TestUtf8Utils : public testing::Test
{
};

TEST_F(TestUtf8Utils, Utf8StringLength)
{
    using valijson::utils::u8_strlen;

    EXPECT_EQ(u8_strlen(""), 0);
    EXPECT_EQ(u8_strlen("a"), 1);
    EXPECT_EQ(u8_strlen("abc"), 3);

    // U+0416
    EXPECT_EQ(u8_strlen("\xD0\x96"), 1);

    // U+0915
    EXPECT_EQ(u8_strlen("\xE0\xA4\x95"), 1);

    // U+10348
    EXPECT_EQ(u8_strlen("\xF0\x90\x8D\x88"), 1);

    // U+0915 + U+0416
    EXPECT_EQ(u8_strlen("\xE0\xA4\x95\xD0\x96"), 2);

    // incomplete U+0416 at the end
    EXPECT_EQ(u8_strlen("\xD0"), 1);

    // incomplete U+0416 in the middle
    EXPECT_EQ(u8_strlen("\320abc"), 4);

    // incomplete U+0915 at the end
    EXPECT_EQ(u8_strlen("\xE0\xA4"), 1);

    // incomplete U+0915 at the end
    EXPECT_EQ(u8_strlen("\xE0\244abc"), 4);

    // U+DFFF
    EXPECT_EQ(u8_strlen("\xED\xBF\xBF"), 1);

    // Overlong encoding for U+0000
    EXPECT_EQ(u8_strlen("\xC0\x80"), 1);

    // U+110000 (out of Unicode range)
    EXPECT_EQ(u8_strlen("\xF5\x80\x80\x80"), 1);

    // 0xE0 + 0xA4 repeating 9 times
    EXPECT_EQ(u8_strlen("\340\244\244\244\244\244\244\244\244\244"), 5);
}
