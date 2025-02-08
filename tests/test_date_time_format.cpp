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

class TestDateTimeFormat : public ::testing::Test
{

};

TEST_F(TestDateTimeFormat, StrictAndPermissiveDateTimes)
{
    // Load schema document
    rapidjson::Document schemaDocument;
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/schemas/date_time_format.schema.json", schemaDocument) );
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
    ASSERT_TRUE( loadDocument(TEST_DATA_DIR "/documents/date_time_format.json", testDocument) );
    RapidJsonAdapter testAdapter(testDocument);

    // Setup validators
    Validator strictValidator(Validator::kStrongTypes, Validator::kStrictDateTime);
    Validator permissiveValidator(Validator::kStrongTypes, Validator::kPermissiveDateTime);

    const RapidJsonAdapter::Array examples = testAdapter.asArray();
    for (auto &&example : examples) {

        auto validity = example.asObject().find("validity")->second.asString();
        if (validity == "strict") {
            EXPECT_TRUE( strictValidator.validate(schema, example, NULL) );
            EXPECT_TRUE( permissiveValidator.validate(schema, example, NULL) );
        } else if (validity == "permissive") {
            EXPECT_FALSE( strictValidator.validate(schema, example, NULL) );
            EXPECT_TRUE( permissiveValidator.validate(schema, example, NULL) );
        } else {
            EXPECT_FALSE( strictValidator.validate(schema, example, NULL) );
            EXPECT_FALSE( permissiveValidator.validate(schema, example, NULL) );
        }
    }
}
