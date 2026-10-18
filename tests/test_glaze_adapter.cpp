#include <gtest/gtest.h>

#include <cmath>
#include <limits>
#include <string>

#include <valijson/adapters/glaze_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/utils/glaze_utils.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::adapters::GlazeAdapter;
using valijson::adapters::GlazeDocument;

class TestGlazeAdapter : public testing::Test
{
protected:

    static GlazeDocument parse(const std::string &json)
    {
        GlazeDocument document;
        const auto error = glz::read_json(document, json);
        EXPECT_FALSE(static_cast<bool>(error))
            << "Glaze failed to parse '" << json << "'";
        return document;
    }

    static Schema buildSchema(const std::string &json)
    {
        const GlazeDocument schemaDocument = parse(json);
        const GlazeAdapter schemaAdapter(schemaDocument);

        Schema schema;
        SchemaParser parser;
        parser.populateSchema(schemaAdapter, schema);
        return schema;
    }
};

TEST_F(TestGlazeAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a Glaze document that consists of an array of numbers
    GlazeDocument::array_t array;
    for (unsigned int i = 0; i < numElements; i++) {
        GlazeDocument value;
        value = static_cast<double>(i);
        array.push_back(value);
    }

    GlazeDocument document;
    document = array;

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    GlazeAdapter adapter(document);
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
    for (const GlazeAdapter value : adapter.getArray()) {
        ASSERT_TRUE( value.isNumber() );
        EXPECT_EQ( double(expectedValue), value.asDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

TEST_F(TestGlazeAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a Glaze document that consists of an object that maps numeric
    // strings to their corresponding numeric values. Single character keys are
    // used so that insertion order and lexicographical order are the same;
    // Glaze uses a sorted map for objects prior to v6.0.0, and an
    // insertion-ordered map from v6.0.0 onwards.
    GlazeDocument document;
    for (unsigned int i = 0; i < numElements; i++) {
        document[std::to_string(i)] = static_cast<double>(i);
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    GlazeAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW( adapter.getObject() );
    ASSERT_ANY_THROW( adapter.getArray() );
    ASSERT_ANY_THROW( adapter.getBool() );
    ASSERT_ANY_THROW( adapter.getDouble() );
    ASSERT_ANY_THROW( adapter.getString() );
#endif

    // Ensure that the object contains the expected number of members
    EXPECT_EQ( numElements, adapter.getObject().size() );

    unsigned int expectedValue = 0;
    for (const GlazeAdapter::ObjectMember member : adapter.getObject()) {
        ASSERT_TRUE( member.second.isNumber() );
        EXPECT_EQ( std::to_string(expectedValue), member.first );
        EXPECT_EQ( double(expectedValue), member.second.asDouble() );
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ( numElements, expectedValue );
}

TEST_F(TestGlazeAdapter, ObjectMemberLookup)
{
    const GlazeDocument document = parse(R"({"a": 1, "b": "two"})");
    const GlazeAdapter adapter(document);
    const GlazeAdapter::Object object = adapter.getObject();

    auto itr = object.find("b");
    ASSERT_TRUE( itr != object.end() );
    EXPECT_EQ( "b", itr->first );
    EXPECT_EQ( "two", itr->second.getString() );

    EXPECT_TRUE( object.find("missing") == object.end() );
}

TEST_F(TestGlazeAdapter, NumbersWithoutFractionalPartAreIntegers)
{
    // Glaze stores every JSON number as a double when using the default number
    // mode, so the adapter classifies numbers that have no fractional part as
    // integers, and everything else as a double
    const GlazeDocument document =
        parse(R"({"integer": 42, "written_as_double": 42.0, "double": 42.5, "negative": -7})");
    const GlazeAdapter adapter(document);
    const GlazeAdapter::Object object = adapter.getObject();

    for (const char *name : {"integer", "written_as_double", "negative"}) {
        auto itr = object.find(name);
        ASSERT_TRUE( itr != object.end() ) << name;
        EXPECT_TRUE( itr->second.isNumber() ) << name;
        EXPECT_TRUE( itr->second.isInteger() ) << name;
        EXPECT_FALSE( itr->second.isDouble() ) << name;
    }

    EXPECT_EQ( 42, object.find("integer")->second.getInteger() );
    EXPECT_EQ( 42, object.find("written_as_double")->second.getInteger() );
    EXPECT_EQ( -7, object.find("negative")->second.getInteger() );

    // Values that have a fractional part are exposed as doubles only
    auto itr = object.find("double");
    ASSERT_TRUE( itr != object.end() );
    EXPECT_TRUE( itr->second.isNumber() );
    EXPECT_TRUE( itr->second.isDouble() );
    EXPECT_FALSE( itr->second.isInteger() );
    EXPECT_EQ( 42.5, itr->second.getDouble() );

    // Integers can still be read as doubles
    EXPECT_EQ( 42.0, object.find("integer")->second.asDouble() );
}

TEST_F(TestGlazeAdapter, IntegerConstraintsUseFractionalPart)
{
    const Schema schema = buildSchema(
        R"({"type": "object", "properties": {"value": {"type": "integer"}}, "required": ["value"]})");

    Validator validator(Validator::kStrongTypes);

    const GlazeDocument integerDocument = parse(R"({"value": 10})");
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(integerDocument), nullptr) )
        << "Validation should pass for an integer";

    // A JSON number with a zero fractional part is an integer, as far as JSON
    // Schema is concerned
    const GlazeDocument trailingZeroDocument = parse(R"({"value": 10.0})");
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(trailingZeroDocument), nullptr) )
        << "Validation should pass for a number with a zero fractional part";

    const GlazeDocument doubleDocument = parse(R"({"value": 10.5})");
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(doubleDocument), nullptr) )
        << "Validation should fail for a number with a fractional part";

    const GlazeDocument stringDocument = parse(R"({"value": "10"})");
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(stringDocument), nullptr) )
        << "Validation should fail for a string, when using strong types";
}

TEST_F(TestGlazeAdapter, ValidationOfParsedDocument)
{
    const Schema schema = buildSchema(R"({
        "type": "object",
        "properties": {
            "name": {"type": "string", "minLength": 1},
            "tags": {"type": "array", "items": {"type": "string"}},
            "count": {"type": "integer", "minimum": 1}
        },
        "required": ["name", "count"]
    })");

    Validator validator(Validator::kStrongTypes);

    const GlazeDocument validDocument =
        parse(R"({"name": "valijson", "tags": ["json", "schema"], "count": 3})");
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(validDocument), nullptr) );

    const GlazeDocument missingRequired = parse(R"({"name": "valijson"})");
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(missingRequired), nullptr) )
        << "Validation should fail when a required property is missing";

    const GlazeDocument wrongItemType =
        parse(R"({"name": "valijson", "tags": ["json", 7], "count": 3})");
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(wrongItemType), nullptr) )
        << "Validation should fail when an array item has the wrong type";

    const GlazeDocument belowMinimum =
        parse(R"({"name": "valijson", "count": 0})");
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(belowMinimum), nullptr) )
        << "Validation should fail when a numeric constraint is not satisfied";
}

TEST_F(TestGlazeAdapter, NonFiniteNumbersRejected)
{
    const Schema schema = buildSchema(
        R"({"type": "object", "properties": {"value": {"type": "number"}}, "required": ["value"]})");

    Validator validator(Validator::kStrongTypes);

    GlazeDocument docWithNaN;
    docWithNaN["value"] = std::numeric_limits<double>::quiet_NaN();
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(docWithNaN), nullptr) )
        << "Validation should fail for NaN (serializes to null)";

    GlazeDocument docWithPosInf;
    docWithPosInf["value"] = std::numeric_limits<double>::infinity();
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(docWithPosInf), nullptr) )
        << "Validation should fail for positive infinity (serializes to null)";

    GlazeDocument docWithNegInf;
    docWithNegInf["value"] = -std::numeric_limits<double>::infinity();
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(docWithNegInf), nullptr) )
        << "Validation should fail for negative infinity (serializes to null)";

    const GlazeDocument docWithNull = parse(R"({"value": null})");
    EXPECT_FALSE( validator.validate(schema, GlazeAdapter(docWithNull), nullptr) )
        << "Validation should fail for explicit null";

    const GlazeDocument docWithFinite = parse(R"({"value": 42.5})");
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(docWithFinite), nullptr) )
        << "Validation should pass for normal finite number";

    const GlazeDocument docWithZero = parse(R"({"value": 0})");
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(docWithZero), nullptr) )
        << "Validation should pass for zero";

    GlazeDocument docWithLarge;
    docWithLarge["value"] = (std::numeric_limits<double>::max)();
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(docWithLarge), nullptr) )
        << "Validation should pass for very large but finite number";
}

TEST_F(TestGlazeAdapter, NonFiniteNumbersTreatedAsNull)
{
    const Schema schema = buildSchema(
        R"({"type": "object", "properties": {"value": {"type": ["number", "null"]}}, "required": ["value"]})");

    Validator validator(Validator::kStrongTypes);

    GlazeDocument docWithNaN;
    docWithNaN["value"] = std::numeric_limits<double>::quiet_NaN();
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(docWithNaN), nullptr) )
        << "NaN should pass validation when null is allowed";

    GlazeDocument docWithInf;
    docWithInf["value"] = std::numeric_limits<double>::infinity();
    EXPECT_TRUE( validator.validate(schema, GlazeAdapter(docWithInf), nullptr) )
        << "Infinity should pass validation when null is allowed";
}

TEST_F(TestGlazeAdapter, TypesAreNotConflated)
{
    const GlazeDocument document =
        parse(R"({"array": [], "object": {}, "string": "s", "bool": true, "null": null, "number": 1})");
    const GlazeAdapter adapter(document);
    const GlazeAdapter::Object object = adapter.getObject();

    const GlazeAdapter array = object.find("array")->second;
    EXPECT_TRUE( array.isArray() );
    EXPECT_FALSE( array.isObject() );
    EXPECT_EQ( 0u, array.getArray().size() );

    const GlazeAdapter nested = object.find("object")->second;
    EXPECT_TRUE( nested.isObject() );
    EXPECT_FALSE( nested.isArray() );
    EXPECT_EQ( 0u, nested.getObject().size() );

    const GlazeAdapter string = object.find("string")->second;
    EXPECT_TRUE( string.isString() );
    EXPECT_FALSE( string.isNumber() );
    EXPECT_EQ( "s", string.getString() );

    const GlazeAdapter boolean = object.find("bool")->second;
    EXPECT_TRUE( boolean.isBool() );
    EXPECT_FALSE( boolean.isNumber() );
    EXPECT_TRUE( boolean.getBool() );

    const GlazeAdapter null = object.find("null")->second;
    EXPECT_TRUE( null.isNull() );
    EXPECT_FALSE( null.isNumber() );

    const GlazeAdapter number = object.find("number")->second;
    EXPECT_TRUE( number.isNumber() );
    EXPECT_FALSE( number.isBool() );
    EXPECT_FALSE( number.isNull() );

    // The adapter reports strict types, so weak comparisons must be requested
    // explicitly
    EXPECT_TRUE( adapter.hasStrictTypes() );
}

TEST_F(TestGlazeAdapter, FrozenValueRetainsValue)
{
    const GlazeDocument document = parse(R"({"a": [1, 2, 3]})");
    const GlazeAdapter adapter(document);

    valijson::adapters::FrozenValue *frozen = adapter.freeze();

    // The frozen value should compare equal to the original, and to an
    // equivalent document parsed separately
    EXPECT_TRUE( frozen->equalTo(adapter, true) );

    const GlazeDocument equivalent = parse(R"({"a": [1, 2, 3]})");
    EXPECT_TRUE( frozen->equalTo(GlazeAdapter(equivalent), true) );

    const GlazeDocument different = parse(R"({"a": [1, 2, 4]})");
    EXPECT_FALSE( frozen->equalTo(GlazeAdapter(different), true) );

    valijson::adapters::FrozenValue *clone = frozen->clone();
    EXPECT_TRUE( clone->equalTo(adapter, true) );

    delete clone;
    delete frozen;
}

TEST_F(TestGlazeAdapter, LoadDocumentFromFile)
{
    GlazeDocument document;
    ASSERT_TRUE( valijson::utils::loadDocument(
        "../tests/data/documents/array_doubles_1_2_3.json", document) );

    const GlazeAdapter adapter(document);
    ASSERT_TRUE( adapter.isArray() );
    EXPECT_EQ( 3u, adapter.getArray().size() );

    GlazeDocument missing;
    EXPECT_FALSE( valijson::utils::loadDocument(
        "../tests/data/documents/does_not_exist.json", missing) );
}
