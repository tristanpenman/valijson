#include <gtest/gtest.h>
#include "json/json.h" // jsoncpp rolled up

#define STRINGY1(arg) #arg
#define STRINGY2(arg) STRINGY1(arg)

#define deblog(args)  { \
    std::cerr <<  __FILE__  ":" STRINGY2(__LINE__ ) ": " ; \
    std::cerr << args << " "<< __PRETTY_FUNCTION__  << '\n'; \
}

#include <iostream>
#include <regex>
#include <boost/regex.hpp>

#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

namespace {
using namespace valijson;
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

const static string emplrec (R"(
{ 
    "employee" : {
         "John" : {
              "fullname": "John Doe"
         }
     },
     "elist" : [
           {"rec" : "John"},
           {"rec" : "Jane"}
      ]     
}
)");

const static string tschema(R"(
{
    "type" : "object",
    "id" : "eroot",
    "properties" : {
        "employee": {
            "type":"object"
        },
        "elist" : {
            "type" : "array",
            "items": {
                 "type" : "object",
                 "additionalProperties": false,
                 "required": ["rec"],
                 "properties" : {
                     "rec" : { 
                         "type" : "string"
                     }
                 }
            }
        }
    },
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

using namespace constraints;

Json::Value *root; // needs to be a better way to do this.

class PathConstraint : public valijson::constraints::PolyConstraint {
    const std::string path;

public:
    PathConstraint(const std::string &path) : path(path){ }
    
    virtual valijson::constraints::PolyConstraint * clone() const {
        deblog("clone !");
        return new  PathConstraint(path);
     };

    virtual bool validate(const adapters::Adapter &target, const std::vector<std::string> &context, ValidationResults *results) const {
        std::string spath(path + "." + target.asString());
        const Json::Path jpath(spath);
        const Json::Value& find(jpath.resolve(*root));
        if (!find) {
            std::string estring("Failed to find " + spath + " in input");
            if (results) {
                results->pushError(context, estring);
            }
            return false;
        } else {
            return true;
        } 
    }
    virtual Constraint * clone(CustomAlloc allocFn, CustomFree freeFn) const
    {
        void *ptr = allocFn(sizeof(PathConstraint));
        if (!ptr) {
            throw std::runtime_error(
                    "Failed to allocate memory for cloned constraint");
        }

        try {
            return new (ptr) PathConstraint(
                    *static_cast<const PathConstraint*>(this));
        } catch (...) {
            freeFn(ptr);
            throw;
        }
    }
};

}

TEST(PolyConstraint, Insert) {
    Json::Value schemaJson(jsonParse(tschema));

    JsonCppAdapter schemaDocumentAdapter(schemaJson);

    SchemaParser parser;
    Schema schema;
    parser.populateSchema(schemaDocumentAdapter, schema);
    std::string empl(".employee");
    PathConstraint empConstraint(empl);
    // how to add this to schema?

    Json::Value doc = jsonParse(emplrec);
    root = &doc;

    JsonCppAdapter targetDocumentAdapter(doc);
    deblog("doc " << doc);
    deblog("schema" << schemaJson);
    ValidationResults results;
    Validator validator;
    std::regex John("Failed.*John");
    std::regex Jane("Failed.*Jane");
    EXPECT_FALSE(validator.validate(schema,targetDocumentAdapter, &results));
    std::string err(err2String(results));
    deblog("err " << err);
    EXPECT_FALSE(regex_search(err, John));
    EXPECT_TRUE(regex_search(err, Jane));
}
