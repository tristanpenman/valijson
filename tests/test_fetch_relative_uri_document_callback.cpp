#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

namespace {

const rapidjson::Document * fetchRelativeUriDocument(const std::string &uri)
{
    rapidjson::Document *document = new rapidjson::Document();
    document->SetObject();

    if (uri == "json-schemas/schema-B.json") {
        document->AddMember("$ref", "schema-C.json", document->GetAllocator());
    } else if (uri == "json-schemas/schema-C.json") {
        document->AddMember("type", "string", document->GetAllocator());
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
    schemaDocument.SetObject();
    schemaDocument.AddMember("$ref", "json-schemas/schema-B.json",
            schemaDocument.GetAllocator());

    valijson::Schema schema;
    valijson::SchemaParser parser;
    parser.populateSchema(valijson::adapters::RapidJsonAdapter(schemaDocument),
            schema, fetchRelativeUriDocument, freeRelativeUriDocument);

    rapidjson::Document validDocument;
    validDocument.SetString("valid");
    rapidjson::Document invalidDocument;
    invalidDocument.SetInt(123);

    valijson::Validator validator;
    EXPECT_TRUE(validator.validate(schema,
            valijson::adapters::RapidJsonAdapter(validDocument), nullptr));
    EXPECT_FALSE(validator.validate(schema,
            valijson::adapters::RapidJsonAdapter(invalidDocument), nullptr));
}
