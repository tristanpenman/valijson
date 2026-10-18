#include <locale>

#include <gtest/gtest.h>

#include <valijson/internal/double_parser.hpp>

namespace {

class CommaDecimalPoint: public std::numpunct<char>
{
protected:
    char do_decimal_point() const override
    {
        return ',';
    }
};

class GlobalLocaleGuard
{
public:
    explicit GlobalLocaleGuard(const std::locale &locale)
      : previous(std::locale::global(locale)) { }

    ~GlobalLocaleGuard()
    {
        std::locale::global(previous);
    }

private:
    const std::locale previous;
};

} // namespace

TEST(DoubleParser, ParsesCompleteFloatingPointStrings)
{
    double result = 0.0;
    EXPECT_TRUE(valijson::internal::parseDouble("-12.5e2", result));
    EXPECT_DOUBLE_EQ(-1250.0, result);

    EXPECT_FALSE(valijson::internal::parseDouble("", result));
    EXPECT_FALSE(valijson::internal::parseDouble("1.5suffix", result));
    EXPECT_FALSE(valijson::internal::parseDouble(" 1.5", result));
    EXPECT_FALSE(valijson::internal::parseDouble("1.5 ", result));
}

TEST(DoubleParser, IsIndependentOfGlobalLocale)
{
    const std::locale commaLocale(
            std::locale::classic(), new CommaDecimalPoint);
    const GlobalLocaleGuard guard(commaLocale);

    double result = 0.0;
    EXPECT_TRUE(valijson::internal::parseDouble("1.5", result));
    EXPECT_DOUBLE_EQ(1.5, result);
    EXPECT_FALSE(valijson::internal::parseDouble("1,5", result));
}
