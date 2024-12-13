#ifdef _MSC_VER
#pragma warning(disable: 4706)
#include <picojson.h>
#pragma warning(default: 4706)
#else
#include <picojson.h>
#endif

#include <iostream>

#include <gtest/gtest.h>

#include <valijson/adapters/json11_adapter.hpp>
#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/adapters/picojson_adapter.hpp>
#include <valijson/adapters/nlohmann_json_adapter.hpp>
#include <valijson/utils/json11_utils.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>
#include <valijson/utils/picojson_utils.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/utils/nlohmann_json_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>
#include <valijson/exceptions.hpp>
#ifdef VALIJSON_BUILD_POCO_ADAPTER
#include <valijson/adapters/poco_json_adapter.hpp>
#include <valijson/utils/poco_json_utils.hpp>
#endif

using valijson::adapters::AdapterTraits;
using valijson::adapters::RapidJsonAdapter;
using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;

static void createFileFromContent(const std::string& filename, const std::string& content)
{
    std::ofstream outfile(filename, std::ofstream::out | std::ofstream::trunc);
    outfile << content << std::endl;
    outfile.close();
};

//Potentially :
// Define a struct CustomRegexEngine that handle both problem and use it as replacement of Validator.
//using CustomValidator = ValidatorT<CustomRegexEngine>;

TEST(valijson, valijson_be_robust_against_bad_regular_expression)
{
    GTEST_SKIP() << "Skipping: causes segmentation fault with default Validator";

    static const std::string schema = R"(
    {
        "properties": {
            "text": {
                "pattern": "^[\\s\\S]+$",
                "type": "string"
            }
        }
    }
    )";

    createFileFromContent("schema.json", schema);
    rapidjson::Document mySchemaDoc;
    ASSERT_TRUE(valijson::utils::loadDocument("schema.json", mySchemaDoc));

    Schema mySchema;
    SchemaParser parser;
    RapidJsonAdapter mySchemaAdapter(mySchemaDoc);
    parser.populateSchema(mySchemaAdapter, mySchema);
    rapidjson::Document myTargetDoc;

    std::string payload = "{ \"text\" :  \"";
    for (int i = 0; i< 100000; ++i) {
        payload += 'A';
    }
    payload += "\"}";

    createFileFromContent("payload.json", payload);

    ASSERT_TRUE(valijson::utils::loadDocument("payload.json", myTargetDoc));

    // This test crash (segfault) is validator is not customized with custom RegexpEngine
    Validator validator;
    RapidJsonAdapter myTargetAdapter(myTargetDoc);
    ASSERT_TRUE(validator.validate(mySchema, myTargetAdapter, nullptr));
}

TEST(valijson, valijson_be_robust_against_catastrophic_backtracking_regular_expression)
{
    GTEST_SKIP() << "Skipping: hangs due to non management of catastrophic backtracking with default Validator";

    static const std::string schema = R"(
    {
        "properties": {
            "text": {
                "pattern": "((A+)*)+$",
                "type": "string"
            }
        }
    }
    )";

    createFileFromContent("schema.json", schema);
    rapidjson::Document mySchemaDoc;
    ASSERT_TRUE(valijson::utils::loadDocument("schema.json", mySchemaDoc));

    Schema mySchema;
    SchemaParser parser;
    RapidJsonAdapter mySchemaAdapter(mySchemaDoc);
    parser.populateSchema(mySchemaAdapter, mySchema);
    rapidjson::Document myTargetDoc;

    std::string payload = "{ \"text\" :  \"AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAC\"}";
    createFileFromContent("payload.json", payload);

    ASSERT_TRUE(valijson::utils::loadDocument("payload.json", myTargetDoc));

    //This test takes endless time if validator is not customized with custom RegexpEngine
    Validator validator;
    RapidJsonAdapter myTargetAdapter(myTargetDoc);

    //payload is correct regarding the regexp but evaluation is impossible due to catastrophic regexp backtracking. so we return false.
    ASSERT_FALSE(validator.validate(mySchema, myTargetAdapter, nullptr));
}
