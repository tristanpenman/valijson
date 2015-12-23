#include <boost/make_shared.hpp>

#include <gtest/gtest.h>

#include <valijson/adapters/rapidjson_adapter.hpp>

#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::SchemaParser;
using valijson::adapters::RapidJsonAdapter;
using valijson::Validator;

typedef SchemaParser::FetchDocumentFunction<RapidJsonAdapter>::Type
        FetchDocumentFunction;

namespace {

static rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> allocator;
static rapidjson::Value fetchedRoot;
static RapidJsonAdapter fetchedRootAdapter;

}

class TestFetchDocumentCallback : public ::testing::Test
{

};

boost::shared_ptr<const RapidJsonAdapter> fetchDocument(const std::string &uri)
{
    EXPECT_STREQ("test", uri.c_str());

    rapidjson::Value valueOfTypeAttribute;
    valueOfTypeAttribute.SetString("string", allocator);

    rapidjson::Value schemaOfTestProperty;
    schemaOfTestProperty.SetObject();
    schemaOfTestProperty.AddMember("type", valueOfTypeAttribute, allocator);

    rapidjson::Value propertiesConstraint;
    propertiesConstraint.SetObject();
    propertiesConstraint.AddMember("test", schemaOfTestProperty, allocator);

    fetchedRoot.SetObject();
    fetchedRoot.AddMember("properties", propertiesConstraint, allocator);

    // Have to ensure that fetchedRoot exists for at least as long as the
    // shared pointer that we return here
    return boost::make_shared<RapidJsonAdapter>(fetchedRoot);
}

TEST_F(TestFetchDocumentCallback, Basics)
{
    // Define schema
    rapidjson::Document schemaDocument;
    RapidJsonAdapter schemaDocumentAdapter(schemaDocument);
    schemaDocument.SetObject();
    schemaDocument.AddMember("$ref", "test#/", allocator);

    // Parse schema document
    Schema schema;
    SchemaParser schemaParser;
    schemaParser.populateSchema(schemaDocumentAdapter, schema,
            boost::make_optional<FetchDocumentFunction>(fetchDocument));

    // Test resulting schema with a valid document
    rapidjson::Document validDocument;
    validDocument.SetObject();
    validDocument.AddMember("test", "valid", allocator);
    Validator validator;
    EXPECT_TRUE(validator.validate(schema, RapidJsonAdapter(validDocument), NULL));

    // Test resulting schema with an invalid document
    rapidjson::Document invalidDocument;
    invalidDocument.SetObject();
    invalidDocument.AddMember("test", 123, allocator);
    EXPECT_FALSE(validator.validate(schema, RapidJsonAdapter(invalidDocument), NULL));
}
