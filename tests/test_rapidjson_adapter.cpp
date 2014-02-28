#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>

class TestRapidJsonAdapter : public testing::Test
{

};

TEST_F(TestRapidJsonAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a rapidjson document that consists of an array of numbers
    rapidjson::Document document;
    document.SetArray();
    for (unsigned int i = 0; i < numElements; i++) {
        rapidjson::Value value;
        value.SetDouble(i);
        document.PushBack(value, document.GetAllocator());
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::RapidJsonAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ( numElements, adapter.getArray().size() );

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    BOOST_FOREACH( const valijson::adapters::RapidJsonAdapter value, adapter.getArray() ) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestRapidJsonAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a rapidjson document that consists of an object that maps numeric
    // strings their corresponding numeric values
    rapidjson::Document document;
    document.SetObject();
    for (unsigned int i = 0; i < numElements; i++) {
        rapidjson::Value name, value;
        name.SetString(boost::lexical_cast<std::string>(i).c_str(), document.GetAllocator());
        value.SetDouble(i);
        document.AddMember(name, value, document.GetAllocator());
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::RapidJsonAdapter adapter(document);
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    BOOST_FOREACH( const valijson::adapters::RapidJsonAdapter::ObjectMember member, adapter.getObject() ) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( boost::lexical_cast<std::string>(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}