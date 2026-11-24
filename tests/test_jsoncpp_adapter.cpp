#include <sstream>

#include <gtest/gtest.h>

#include <valijson/adapters/jsoncpp_adapter.hpp>

class TestJsonCppAdapter : public testing::Test
{

};

TEST_F(TestJsonCppAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a jsoncpp document that consists of an array of numbers
    Json::Value document(Json::arrayValue);
    for (unsigned int i = 0; i < numElements; i++) {
        document.append(Json::Value(i));
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::JsonCppAdapter adapter(document);
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
    for (const valijson::adapters::JsonCppAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getNumber() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestJsonCppAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a jsoncpp document that consists of an object that maps numeric
    // strings their corresponding numeric values
    Json::Value document(Json::objectValue);
    for (unsigned int i = 0; i < numElements; i++) {
        std::string name(std::to_string(i));
        document[name] = Json::Value(double(i));
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::JsonCppAdapter adapter(document);
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
    for (const valijson::adapters::JsonCppAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

TEST_F(TestJsonCppAdapter, Integers)
{
    Json::Value document;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::istringstream stream(R"([1, 1.0, 5000000000, -5000000000])");
    ASSERT_TRUE(Json::parseFromStream(builder, stream, &document, &errors));

    // A number written with a fractional part is not an integer, even if the
    // fractional part is zero
    const valijson::adapters::JsonCppAdapter one(document[0]);
    const valijson::adapters::JsonCppAdapter oneWithFraction(document[1]);
    EXPECT_TRUE(one.isInteger());
    EXPECT_FALSE(oneWithFraction.isInteger());
    EXPECT_TRUE(oneWithFraction.isNumber());

    int64_t value = 0;
    EXPECT_TRUE(one.getInteger(value));
    EXPECT_EQ(1, value);
    EXPECT_FALSE(oneWithFraction.getInteger(value));

    // Integers that do not fit in an int are not truncated
    const valijson::adapters::JsonCppAdapter large(document[2]);
    EXPECT_TRUE(large.isInteger());
    ASSERT_TRUE(large.getInteger(value));
    EXPECT_EQ(5000000000, value);

    const valijson::adapters::JsonCppAdapter largeNegative(document[3]);
    ASSERT_TRUE(largeNegative.getInteger(value));
    EXPECT_EQ(-5000000000, value);
}
