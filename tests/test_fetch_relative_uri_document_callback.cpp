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

namespace {

int fetchCount = 0;
int freeCount = 0;

const rapidjson::Document * fetchCountedDocument(const std::string &uri)
{
    ++fetchCount;

    rapidjson::Document *document = new rapidjson::Document();

    if (uri == "schema-A.json") {
        document->Parse(R"({
            "$ref": "schema-B.json#/definitions/b",
            "definitions": {
                "a": { "type": "integer" }
            }
        })");
    } else if (uri == "schema-B.json") {
        document->Parse(R"({
            "definitions": {
                "b": { "$ref": "schema-A.json#/definitions/a" }
            }
        })");
    } else {
        ADD_FAILURE() << "Unexpected schema URI: " << uri;
        delete document;
        return nullptr;
    }

    return document;
}

void freeCountedDocument(const rapidjson::Document *document)
{
    ++freeCount;
    delete document;
}

} // namespace

TEST(FetchRelativeUriDocumentCallback, FetchesEachDocumentOnce)
{
    // The root schema refers to schema-A.json, which refers to schema-B.json,
    // which refers back to schema-A.json. Each reference is resolved without
    // populating a schema in between, which used to fetch schema-A.json again
    // and leak the second copy, as it was not added to the document cache.
    rapidjson::Document schemaDocument;
    schemaDocument.Parse(R"({ "$ref": "schema-A.json" })");

    fetchCount = 0;
    freeCount = 0;

    {
        valijson::Schema schema;
        valijson::SchemaParser parser;
        parser.populateSchema(valijson::adapters::RapidJsonAdapter(schemaDocument),
                schema, fetchCountedDocument, freeCountedDocument);

        rapidjson::Document validDocument;
        validDocument.Parse("1");
        rapidjson::Document invalidDocument;
        invalidDocument.Parse(R"("invalid")");

        valijson::Validator validator;
        EXPECT_TRUE(validator.validate(schema,
                valijson::adapters::RapidJsonAdapter(validDocument), nullptr));
        EXPECT_FALSE(validator.validate(schema,
                valijson::adapters::RapidJsonAdapter(invalidDocument), nullptr));
    }

    EXPECT_EQ(2, fetchCount);
    EXPECT_EQ(fetchCount, freeCount);
}
