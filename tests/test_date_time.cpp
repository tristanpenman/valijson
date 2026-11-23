#include <string>

#include <gtest/gtest.h>

#include <valijson/internal/date_time.hpp>

using valijson::internal::isDate;
using valijson::internal::isDateTime;
using valijson::internal::isTime;

TEST(DateTime, AcceptsDates)
{
    EXPECT_TRUE(isDate("2022-07-18"));
    EXPECT_TRUE(isDate("0000-01-01"));
    EXPECT_TRUE(isDate("9999-12-31"));
    EXPECT_TRUE(isDate("2020-02-29"));
    EXPECT_TRUE(isDate("2000-02-29"));
}

TEST(DateTime, RejectsInvalidDates)
{
    EXPECT_FALSE(isDate(""));
    EXPECT_FALSE(isDate("2022-7-18"));
    EXPECT_FALSE(isDate("2022-07-8"));
    EXPECT_FALSE(isDate("2022/07/18"));
    EXPECT_FALSE(isDate("2022-07-18T00:00:00Z"));
    EXPECT_FALSE(isDate("2022-00-18"));
    EXPECT_FALSE(isDate("2022-13-18"));
    EXPECT_FALSE(isDate("2022-07-00"));
    EXPECT_FALSE(isDate("2022-07-32"));
    EXPECT_FALSE(isDate("2022-04-31"));
    EXPECT_FALSE(isDate("2021-02-29"));
    EXPECT_FALSE(isDate("1900-02-29"));
    EXPECT_FALSE(isDate("2022-07-1\xd9\xa8"));
    EXPECT_FALSE(isDate(std::string("2022-07-1\0", 10)));
}

TEST(DateTime, RejectsDatesWithoutFourDigitYears)
{
    EXPECT_FALSE(isDate("22-07-18"));
    EXPECT_FALSE(isDate("022-07-18"));
    EXPECT_FALSE(isDate("02022-07-18"));

    // Previously, years were converted with std::stoi, which threw
    // std::out_of_range for years that do not fit in an int
    EXPECT_FALSE(isDate("99999999999999999999-01-01"));
    EXPECT_FALSE(isDateTime("99999999999999999999-01-01T00:00:00Z", true));
}

TEST(DateTime, AcceptsTimes)
{
    EXPECT_TRUE(isTime("16:52:45Z", true));
    EXPECT_TRUE(isTime("16:52:45z", true));
    EXPECT_TRUE(isTime("16:52:45+02:00", true));
    EXPECT_TRUE(isTime("16:52:45-23:59", true));
    EXPECT_TRUE(isTime("16:52:45.123456Z", true));
    EXPECT_TRUE(isTime("00:00:00Z", true));
    EXPECT_TRUE(isTime("23:59:59Z", true));
}

TEST(DateTime, RejectsInvalidTimes)
{
    EXPECT_FALSE(isTime("", true));
    EXPECT_FALSE(isTime("24:00:00Z", true));
    EXPECT_FALSE(isTime("16:60:00Z", true));
    EXPECT_FALSE(isTime("16:52:61Z", true));
    EXPECT_FALSE(isTime("1:52:45Z", true));
    EXPECT_FALSE(isTime("16:52:45.Z", true));
    EXPECT_FALSE(isTime("16:52:45+24:00", true));
    EXPECT_FALSE(isTime("16:52:45+02:60", true));
    EXPECT_FALSE(isTime("16:52:45+0200", true));
    EXPECT_FALSE(isTime("16:52:45+02:00Z", true));
    EXPECT_FALSE(isTime("16:52:45ZZ", true));
    EXPECT_FALSE(isTime("16:52:45 Z", true));
}

TEST(DateTime, RequiresTimeOffsetOnlyInStrictMode)
{
    EXPECT_FALSE(isTime("16:52:45", true));
    EXPECT_TRUE(isTime("16:52:45", false));
    EXPECT_TRUE(isTime("16:52:45.5", false));
    EXPECT_FALSE(isDateTime("2022-07-18T16:52:45", true));
    EXPECT_TRUE(isDateTime("2022-07-18T16:52:45", false));
}

TEST(DateTime, AcceptsLeapSecondsOnlyAt235960Utc)
{
    EXPECT_TRUE(isTime("23:59:60Z", true));
    EXPECT_TRUE(isTime("15:59:60-08:00", true));
    EXPECT_TRUE(isTime("00:29:60+00:30", true));
    EXPECT_TRUE(isTime("23:59:60", false));
    EXPECT_FALSE(isTime("22:59:60Z", true));
    EXPECT_FALSE(isTime("23:59:60+01:00", true));
}

TEST(DateTime, AcceptsDateTimes)
{
    EXPECT_TRUE(isDateTime("2022-07-18T16:52:45Z", true));
    EXPECT_TRUE(isDateTime("2022-07-18t16:52:45z", true));
    EXPECT_TRUE(isDateTime("2022-07-18T16:52:45.123+02:00", true));
    EXPECT_TRUE(isDateTime("1998-12-31T23:59:60Z", true));
}

TEST(DateTime, RejectsInvalidDateTimes)
{
    EXPECT_FALSE(isDateTime("", true));
    EXPECT_FALSE(isDateTime("2022-07-18", true));
    EXPECT_FALSE(isDateTime("2022-07-18T", true));
    EXPECT_FALSE(isDateTime("2022-07-18 16:52:45Z", true));
    EXPECT_FALSE(isDateTime("2022-02-30T16:52:45Z", true));
    EXPECT_FALSE(isDateTime("22-07-18T16:52:45Z", true));
    EXPECT_FALSE(isDateTime("2022-07-18T25:52:45Z", true));
}
