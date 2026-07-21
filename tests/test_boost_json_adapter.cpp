#include <gtest/gtest.h>

#include <boost/json/src.hpp> // Needs to be included exactly once in the code to use header-only version of Boost.JSON

#include <valijson/adapters/boost_json_adapter.hpp>

class TestBoostJsonAdapter : public testing::Test
{

};

TEST_F(TestBoostJsonAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a Json document that consists of an array of numbers
    boost::json::array array;
    for (unsigned int i = 0; i < numElements; i++) {
        // Boost.JSON differs from some other libraries in offering emplace_back()
        // as well as push_back().  Using the former here saves us having to create
        // a temporary.
        array.emplace_back(static_cast<double>(i));
    }
    boost::json::value document(array);

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::BoostJsonAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getString() );
#endif

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ( numElements, adapter.getArray().size() );

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::BoostJsonAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestBoostJsonAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a DropBoxJson document that consists of an object that maps numeric
    // strings their corresponding numeric values
    boost::json::object object;
    for (uint32_t i = 0; i < numElements; i++) {
        object[std::to_string(i)] = static_cast<double>(i);
    }
    boost::json::value document(object);

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::BoostJsonAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );
#endif

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::BoostJsonAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

TEST_F(TestBoostJsonAdapter, UnsignedIntegersAreIntegers)
{
    // Boost.JSON stores integers as either kind::int64 or kind::uint64. A value
    // constructed from an unsigned integer, or a parsed number greater than
    // INT64_MAX, has kind::uint64. Previously isInteger() only tested is_int64(),
    // so these values were incorrectly reported as non-integers (issue #170).

    // Value constructed from an unsigned integer
    {
        boost::json::value document(5u);
        valijson::adapters::BoostJsonAdapter adapter(document);
        EXPECT_TRUE( adapter.isInteger() );
        EXPECT_TRUE( adapter.isNumber() );
        EXPECT_FALSE( adapter.isDouble() );

        EXPECT_TRUE( adapter.maybeInteger() );
        EXPECT_EQ( int64_t(5), adapter.asInteger() );
    }

    // Value constructed from a signed integer (unchanged behaviour)
    {
        boost::json::value document(5);
        valijson::adapters::BoostJsonAdapter adapter(document);
        EXPECT_TRUE( adapter.isInteger() );
        EXPECT_EQ( int64_t(5), adapter.asInteger() );
    }

    // Parsed number greater than INT64_MAX is stored as uint64
    {
        boost::json::value document = boost::json::parse("9223372036854775808");
        valijson::adapters::BoostJsonAdapter adapter(document);
        EXPECT_TRUE( adapter.isInteger() );
        EXPECT_TRUE( adapter.isNumber() );
    }
}
