#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

class TestNlohmannJsonAdapter : public testing::Test
{

};

TEST_F(TestNlohmannJsonAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a Json document that consists of an array of numbers
    nlohmann::json document;

    for (unsigned int i = 0; i < numElements; i++) {
        document.push_back(static_cast<double>(i));
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::NlohmannJsonAdapter adapter(document);
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
    for (const valijson::adapters::NlohmannJsonAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestNlohmannJsonAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a DropBoxJson document that consists of an object that maps numeric
    // strings their corresponding numeric values
    nlohmann::json document;
    for (uint32_t i = 0; i < numElements; i++) {
        document[std::to_string(i)] = static_cast<double>(i);
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::NlohmannJsonAdapter adapter(document);
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
    for (const valijson::adapters::NlohmannJsonAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.getDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

TEST_F(TestNlohmannJsonAdapter, NonFiniteNumbersRejected)
{
    using namespace valijson;
    using namespace valijson::adapters;

    nlohmann::json schemaJson = {
        {"type", "object"},
        {"properties", {
            {"value", {{"type", "number"}}}
        }},
        {"required", nlohmann::json::array({"value"})}
    };

    Schema schema;
    SchemaParser parser;
    NlohmannJsonAdapter schemaAdapter(schemaJson);
    parser.populateSchema(schemaAdapter, schema);
    Validator validator(Validator::kStrongTypes);

    nlohmann::json docWithNaN;
    docWithNaN["value"] = std::numeric_limits<double>::quiet_NaN();
    NlohmannJsonAdapter adapterNaN(docWithNaN);
    EXPECT_FALSE(validator.validate(schema, adapterNaN, nullptr))
        << "Validation should fail for NaN (serializes to null)";

    nlohmann::json docWithPosInf;
    docWithPosInf["value"] = std::numeric_limits<double>::infinity();
    NlohmannJsonAdapter adapterPosInf(docWithPosInf);
    EXPECT_FALSE(validator.validate(schema, adapterPosInf, nullptr))
        << "Validation should fail for positive infinity (serializes to null)";

    nlohmann::json docWithNegInf;
    docWithNegInf["value"] = -std::numeric_limits<double>::infinity();
    NlohmannJsonAdapter adapterNegInf(docWithNegInf);
    EXPECT_FALSE(validator.validate(schema, adapterNegInf, nullptr))
        << "Validation should fail for negative infinity (serializes to null)";

    nlohmann::json docWithNull;
    docWithNull["value"] = nullptr;
    NlohmannJsonAdapter adapterNull(docWithNull);
    EXPECT_FALSE(validator.validate(schema, adapterNull, nullptr))
        << "Validation should fail for explicit null";

    nlohmann::json docWithFinite;
    docWithFinite["value"] = 42.5;
    NlohmannJsonAdapter adapterFinite(docWithFinite);
    EXPECT_TRUE(validator.validate(schema, adapterFinite, nullptr))
        << "Validation should pass for normal finite number";

    nlohmann::json docWithZero;
    docWithZero["value"] = 0.0;
    NlohmannJsonAdapter adapterZero(docWithZero);
    EXPECT_TRUE(validator.validate(schema, adapterZero, nullptr))
        << "Validation should pass for zero";

    nlohmann::json docWithLarge;
    docWithLarge["value"] = std::numeric_limits<double>::max();
    NlohmannJsonAdapter adapterLarge(docWithLarge);
    EXPECT_TRUE(validator.validate(schema, adapterLarge, nullptr))
        << "Validation should pass for very large but finite number";
}

TEST_F(TestNlohmannJsonAdapter, NonFiniteNumbersRejectedEvenWhenNullAllowed)
{
    using namespace valijson;
    using namespace valijson::adapters;

    nlohmann::json schemaJson = {
        {"type", "object"},
        {"properties", {
            {"value", {{"type", nlohmann::json::array({"number", "null"})}}}
        }},
        {"required", nlohmann::json::array({"value"})}
    };

    Schema schema;
    SchemaParser parser;
    NlohmannJsonAdapter schemaAdapter(schemaJson);
    parser.populateSchema(schemaAdapter, schema);
    Validator validator(Validator::kStrongTypes);

    nlohmann::json docWithNaN;
    docWithNaN["value"] = std::numeric_limits<double>::quiet_NaN();
    NlohmannJsonAdapter adapterNaN(docWithNaN);
    EXPECT_TRUE(validator.validate(schema, adapterNaN, nullptr))
        << "NaN should pass validation when null is allowed";

    nlohmann::json docWithInf;
    docWithInf["value"] = std::numeric_limits<double>::infinity();
    NlohmannJsonAdapter adapterInf(docWithInf);
    EXPECT_TRUE(validator.validate(schema, adapterInf, nullptr))
        << "Infinity should pass validation when null is allowed";
}
