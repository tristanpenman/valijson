#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::adapters::RapidJsonAdapter;

class TestSchemaParserDialects : public ::testing::Test
{

};

TEST_F(TestSchemaParserDialects, Draft202012AcceptsBooleanSchema)
{
    rapidjson::Document schemaDocument;
    schemaDocument.SetBool(false);

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    rapidjson::Document targetDocument;
    targetDocument.SetObject();

    Validator validator;
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(targetDocument), nullptr));
}

TEST_F(TestSchemaParserDialects, Draft202012ReadsDollarId)
{
    rapidjson::Document schemaDocument;
    schemaDocument.SetObject();
    schemaDocument.AddMember("$id", "https://example.com/schema", schemaDocument.GetAllocator());

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    ASSERT_TRUE(schema.hasId());
    EXPECT_EQ("https://example.com/schema", schema.getId());
}

TEST_F(TestSchemaParserDialects, Draft202012AliasesDollarDefsToDefinitions)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({
        "$defs": {
            "positiveInteger": {
                "type": "integer",
                "minimum": 1
            }
        },
        "$ref": "#/definitions/positiveInteger"
    })");
    ASSERT_FALSE(schemaDocument.HasParseError());

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    rapidjson::Document validDocument;
    validDocument.SetInt(2);

    rapidjson::Document invalidDocument;
    invalidDocument.SetInt(0);

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(validDocument), nullptr));
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(invalidDocument), nullptr));
}

TEST_F(TestSchemaParserDialects, Draft202012PrefixItemsValidatesTupleElements)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({
        "prefixItems": [
            { "type": "string" },
            { "type": "number" }
        ]
    })");
    ASSERT_FALSE(schemaDocument.HasParseError());

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    rapidjson::Document validDocument;
    validDocument.Parse(R"(["first", 2, false])");
    ASSERT_FALSE(validDocument.HasParseError());

    rapidjson::Document invalidDocument;
    invalidDocument.Parse(R"([1, 2])");
    ASSERT_FALSE(invalidDocument.HasParseError());

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(validDocument), nullptr));
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(invalidDocument), nullptr));
}

TEST_F(TestSchemaParserDialects, Draft202012ItemsAppliesAfterPrefixItems)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({
        "prefixItems": [
            { "type": "string" }
        ],
        "items": { "type": "number" }
    })");
    ASSERT_FALSE(schemaDocument.HasParseError());

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    rapidjson::Document validDocument;
    validDocument.Parse(R"(["first", 2, 3])");
    ASSERT_FALSE(validDocument.HasParseError());

    rapidjson::Document invalidDocument;
    invalidDocument.Parse(R"(["first", 2, "third"])");
    ASSERT_FALSE(invalidDocument.HasParseError());

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(validDocument), nullptr));
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(invalidDocument), nullptr));
}

TEST_F(TestSchemaParserDialects, Draft202012UnevaluatedItemsAppliesAfterPrefixItems)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({
        "prefixItems": [
            { "type": "string" }
        ],
        "unevaluatedItems": false
    })");
    ASSERT_FALSE(schemaDocument.HasParseError());

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    rapidjson::Document validDocument;
    validDocument.Parse(R"(["first"])");
    ASSERT_FALSE(validDocument.HasParseError());

    rapidjson::Document invalidDocument;
    invalidDocument.Parse(R"(["first", 2])");
    ASSERT_FALSE(invalidDocument.HasParseError());

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(validDocument), nullptr));
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(invalidDocument), nullptr));
}

TEST_F(TestSchemaParserDialects, Draft202012UnevaluatedItemsUsesAllOfPrefixItemsAnnotations)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({
        "allOf": [
            {
                "prefixItems": [
                    { "type": "string" }
                ]
            }
        ],
        "unevaluatedItems": false
    })");
    ASSERT_FALSE(schemaDocument.HasParseError());

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    rapidjson::Document validDocument;
    validDocument.Parse(R"(["first"])");
    ASSERT_FALSE(validDocument.HasParseError());

    rapidjson::Document invalidDocument;
    invalidDocument.Parse(R"(["first", 2])");
    ASSERT_FALSE(invalidDocument.HasParseError());

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(validDocument), nullptr));
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(invalidDocument), nullptr));
}

TEST_F(TestSchemaParserDialects, Draft202012IgnoresAdditionalItems)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({
        "prefixItems": [
            { "type": "string" }
        ],
        "additionalItems": false
    })");
    ASSERT_FALSE(schemaDocument.HasParseError());

    Schema schema;
    SchemaParser schemaParser(SchemaParser::kDraft202012);
    schemaParser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

    rapidjson::Document document;
    document.Parse(R"(["first", 2])");
    ASSERT_FALSE(document.HasParseError());

    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(document), nullptr));
}
