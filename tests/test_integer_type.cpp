#include <string>

#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

using valijson::adapters::RapidJsonAdapter;
using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;

class TestIntegerType : public ::testing::Test
{
protected:

    /**
     * @brief  Validate a JSON document against '{"type": "integer"}'
     *
     * @param  parser     parser to parse the schema with
     * @param  json       document to validate
     * @param  validator  validator to validate the document with
     */
    static bool validate(SchemaParser &parser, const std::string &json,
            Validator validator = Validator())
    {
        rapidjson::Document schemaDocument;
        schemaDocument.Parse(R"({"type": "integer"})");
        Schema schema;
        parser.populateSchema(RapidJsonAdapter(schemaDocument), schema);

        rapidjson::Document document;
        document.Parse(json.c_str());
        return validator.validate(schema, RapidJsonAdapter(document), nullptr);
    }
};

TEST_F(TestIntegerType, Draft4DoesNotAcceptZeroFractionalPart)
{
    SchemaParser parser(SchemaParser::kDraft4);
    EXPECT_FALSE(parser.getIntegralNumbersAreIntegers());
    EXPECT_TRUE(validate(parser, "1"));
    EXPECT_FALSE(validate(parser, "1.0"));
    EXPECT_FALSE(validate(parser, "1.5"));

    // Weak types accept 1.0 regardless of the setting
    EXPECT_TRUE(validate(parser, "1.0", Validator(Validator::kWeakTypes)));
}

TEST_F(TestIntegerType, Draft4AcceptsZeroFractionalPartWhenEnabled)
{
    SchemaParser parser(SchemaParser::kDraft4);
    parser.setIntegralNumbersAreIntegers(true);
    EXPECT_TRUE(parser.getIntegralNumbersAreIntegers());
    EXPECT_TRUE(validate(parser, "1"));
    EXPECT_TRUE(validate(parser, "1.0"));
    EXPECT_FALSE(validate(parser, "1.5"));
}

TEST_F(TestIntegerType, Draft3DoesNotAcceptZeroFractionalPart)
{
    SchemaParser parser(SchemaParser::kDraft3);
    EXPECT_FALSE(parser.getIntegralNumbersAreIntegers());
    EXPECT_FALSE(validate(parser, "1.0"));

    parser.setIntegralNumbersAreIntegers(true);
    EXPECT_TRUE(validate(parser, "1.0"));
}

TEST_F(TestIntegerType, Draft6AcceptsZeroFractionalPart)
{
    SchemaParser parser(SchemaParser::kDraft6);
    EXPECT_TRUE(parser.getIntegralNumbersAreIntegers());
    EXPECT_TRUE(validate(parser, "1.0"));
    EXPECT_FALSE(validate(parser, "1.5"));
}

TEST_F(TestIntegerType, Draft7AcceptsZeroFractionalPart)
{
    SchemaParser parser(SchemaParser::kDraft7);
    EXPECT_TRUE(parser.getIntegralNumbersAreIntegers());
    EXPECT_TRUE(validate(parser, "1.0"));
    EXPECT_FALSE(validate(parser, "1.5"));
}
