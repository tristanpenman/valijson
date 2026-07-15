#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

namespace {

const rapidjson::Document * fetchRelativeUriDocument(const std::string &uri)
{
    rapidjson::Document *document = new rapidjson::Document();

    if (uri == "schema-B.json") {
        document->Parse(R"({
            "type": "object",
            "properties": {
                "testC": { "$ref": "schema-C.json" }
            }
        })");
    } else if (uri == "schema-C.json") {
        document->Parse(R"({ "type": "object" })");
    } else {
        ADD_FAILURE() << "Unexpected schema URI: " << uri;
        delete document;
        return nullptr;
    }

    return document;
}

void freeRelativeUriDocument(const rapidjson::Document *document)
{
    delete document;
}

} // namespace

TEST(FetchRelativeUriDocumentCallback, ResolvesNestedSiblingReferences)
{
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({
        "type": "object",
        "properties": {
            "testB": { "$ref": "schema-B.json" }
        }
    })");

    valijson::Schema schema;
    valijson::SchemaParser parser;
    parser.populateSchema(valijson::adapters::RapidJsonAdapter(schemaDocument),
            schema, fetchRelativeUriDocument, freeRelativeUriDocument);

    rapidjson::Document validDocument;
    validDocument.Parse(R"({ "testB": { "testC": {} } })");
    rapidjson::Document invalidDocument;
    invalidDocument.Parse(R"({ "testB": { "testC": "invalid" } })");

    valijson::Validator validator;
    EXPECT_TRUE(validator.validate(schema,
            valijson::adapters::RapidJsonAdapter(validDocument), nullptr));
    EXPECT_FALSE(validator.validate(schema,
            valijson::adapters::RapidJsonAdapter(invalidDocument), nullptr));
}
