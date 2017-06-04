#include "json/json.h" // jsoncpp rolled up

#include <iostream>
#include <regex>

#define STRINGY1(arg) #arg
#define STRINGY2(arg) STRINGY1(arg)

#define deblog(args)  { \
    std::cerr <<  __FILE__  ":" STRINGY2(__LINE__ ) ": " ; \
    std::cerr << args << " "<< __PRETTY_FUNCTION__  << '\n'; \
}

#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::ValidationResults;
using valijson::adapters::JsonCppAdapter;

using namespace std;

Json::Value jsonParse(const string &jsonStr) {
     Json::Value retJson;
     Json::Reader json_reader;
     if (jsonStr.length() != 0) {
         if (!json_reader.parse(jsonStr, retJson)) {
             throw runtime_error("json parse failure\n"
                 + json_reader.getFormattedErrorMessages());
         }
     }
     return retJson;
}

const static string badttl (R"(
{ 
    "foo" : -10,
    "bar" : "xxx"
}
)");

const static string tschema(R"(
{
    "type" : "object",
    "properties" : {
        "foo" : { 
            "type": "integer",
            "minimum": 0,
             "maximum": 2147483647
        },
        "bar" : {
            "type": "string",
            "pattern": "1.1.1.1"
        }
    },
    "required": ["foo", "bar"],
    "additionalProperties": false
}
)");

std::string err2String(ValidationResults& results) {
    ValidationResults::Error error;
    std::stringstream strstr;
    while (results.popError(error)) {
        strstr << error.description;
        for (std::string& str : error.context)
            strstr << str;
        strstr << "\n";
    }
    return strstr.str();
}

int main(int argc,char ** argv) {
    Json::Value schemaJson(jsonParse(tschema));

    JsonCppAdapter schemaDocumentAdapter(schemaJson);

    SchemaParser parser;
    Schema schema;
    parser.populateSchema(schemaDocumentAdapter, schema);

    Json::Value doc(jsonParse(badttl));
    JsonCppAdapter targetDocumentAdapter(doc);
    deblog("doc " << doc);
    deblog("schema" << schemaJson);
    ValidationResults results;
    Validator validator;
    std::regex bar("Failed.*bar");
    std::regex foo("Failed.*foo");
    if (validator.validate(schema,targetDocumentAdapter, &results)) {
        deblog("oops, should have failed " << err2String(results));
    } else {
        std::string err(err2String(results));
        deblog("error out:\n" << err2String(results));
        if (!regex_search(err, bar)) {
            deblog("failed to complain about bar");
        } else if (!regex_search(err, foo)) {
            deblog("failed to complain about foo");
        } else {
            deblog("success");
        }
    }
}

