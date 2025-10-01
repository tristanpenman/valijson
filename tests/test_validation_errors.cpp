#include <iostream>

#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

#define TEST_DATA_DIR "../tests/data"

using std::string;

using valijson::adapters::AdapterTraits;
using valijson::adapters::RapidJsonAdapter;
using valijson::utils::loadDocument;
using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::ValidationResults;

class TestValidationErrors : public ::testing::Test
{

};

TEST_F(TestValidationErrors, AllOfConstraintFailure)
{
    // Load schema document
    rapidjson::Document schemaDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/schemas/allof_integers_and_numbers.schema.json", schemaDocument) );
    RapidJsonAdapter schemaAdapter(schemaDocument);

    // Parse schema document
    Schema schema;
    SchemaParser schemaParser;
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(schemaParser.populateSchema(schemaAdapter, schema));
#else
    schemaParser.populateSchema(schemaAdapter, schema);
#endif

    // Load test document
    rapidjson::Document testDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/documents/array_doubles_1_2_3.json", testDocument) );
    RapidJsonAdapter testAdapter(testDocument);

    Validator validator;
    ValidationResults results;
    EXPECT_FALSE( validator.validate(schema, testAdapter, &results) );

    ValidationResults::Error error;

    EXPECT_TRUE( results.popError(error) );
    // context (legacy)
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[0]", error.context[1] );
    // description
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );
    // json pointer
    EXPECT_EQ( "/0", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate item #0 in array.", error.description );
    // Note: empty JSON pointer refers to the entire document
    EXPECT_EQ( "", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[1]", error.context[1] );
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );
    EXPECT_EQ( "/1", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate item #1 in array.", error.description );
    EXPECT_EQ( "", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[2]", error.context[1] );
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );
    EXPECT_EQ( "/2", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate item #2 in array.", error.description );
    EXPECT_EQ( "", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate against child schema #0.", error.description );
    EXPECT_EQ( "", error.jsonPointer );

    EXPECT_FALSE( results.popError(error) );
}

TEST_F(TestValidationErrors, AdditionalPropertiesConstraintError_1)
{
    // Load schema document
    rapidjson::Document schemaDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/schemas/additional_properties_string.schema.json", schemaDocument) );
    RapidJsonAdapter schemaAdapter(schemaDocument);

    // Parse schema document
    Schema schema;
    SchemaParser schemaParser;
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(schemaParser.populateSchema(schemaAdapter, schema));
#else
    schemaParser.populateSchema(schemaAdapter, schema);
#endif

    // Load test document
    rapidjson::Document testDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/documents/object_property_number.json", testDocument) );
    RapidJsonAdapter testAdapter(testDocument);

    Validator validator;
    ValidationResults results;
    EXPECT_FALSE( validator.validate(schema, testAdapter, &results) );

    ValidationResults::Error error;

    EXPECT_TRUE( results.popError(error) );
    // context (legacy)
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[\"abc\"]", error.context[1] );
    // description
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );
    // json pointer
    EXPECT_EQ( "/abc", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate against additional properties schema.", error.description );
    EXPECT_EQ( "", error.jsonPointer );

    EXPECT_FALSE( results.popError(error) );
}

TEST_F(TestValidationErrors, AdditionalPropertiesConstraintError_2)
{
    // Load schema document
    rapidjson::Document schemaDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/schemas/additional_properties_number.schema.json", schemaDocument) );
    RapidJsonAdapter schemaAdapter(schemaDocument);

    // Parse schema document
    Schema schema;
    SchemaParser schemaParser;
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(schemaParser.populateSchema(schemaAdapter, schema));
#else
    schemaParser.populateSchema(schemaAdapter, schema);
#endif

    // Load test document
    rapidjson::Document testDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/documents/object_property_string.json", testDocument) );
    RapidJsonAdapter testAdapter(testDocument);

    Validator validator;
    ValidationResults results;
    EXPECT_FALSE( validator.validate(schema, testAdapter, &results) );

    ValidationResults::Error error;

    EXPECT_TRUE( results.popError(error) );
    // context (legacy)
    EXPECT_EQ( size_t(2), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "[\"hello\"]", error.context[1] );
    // description
    EXPECT_EQ( "Value type not permitted by 'type' constraint.", error.description );
    // json pointer
    EXPECT_EQ( "/hello", error.jsonPointer );

    EXPECT_TRUE( results.popError(error) );
    EXPECT_EQ( size_t(1), error.context.size() );
    EXPECT_EQ( "<root>", error.context[0] );
    EXPECT_EQ( "Failed to validate against additional properties schema.", error.description );
    EXPECT_EQ( "", error.jsonPointer );

    EXPECT_FALSE( results.popError(error) );
}
