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
#include <valijson/adapters/property_tree_adapter.hpp>
#include <valijson/adapters/adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>
#include "temp/AdapterPath.h"
#include <boost/property_tree/json_parser.hpp>

using namespace valijson;
using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::ValidationResults;
using valijson::adapters::JsonCppAdapter;
using valijson::adapters::PropertyTreeAdapter;
using valijson::adapters::Adapter;

using namespace std;

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

//        "foo" : { 
//            "type": "integer",
//            "minimum": 0,
//             "maximum": 2147483647
//        },
//        "bar" : {
//            "type": "string",
//            "pattern": "1.1.1.1"
//        },
const static string emplschema(R"(
{
    "type" : "object",
    "properties" : {
        "elist" : {
            "type" : "array",
            "items": {
                 "type" : "object",
                 "properties" : {
                     "rec" : {
                         "jsonpath": ".employee"
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


template <typename DocType>
class RootResults : public ValidationResults {
  public:
    RootResults(const DocType& root)
        : root(root) {}
    const DocType& root;
};

class PathConstraint : public valijson::constraints::PolyConstraint {
    const Path jpath;

public:
    PathConstraint(const std::string &path) : jpath(path){ }
    
    virtual valijson::constraints::PolyConstraint * clone() const {
        deblog("clone !");
        return new PathConstraint(*this);
     };

    virtual bool validate(const Adapter& target,
            const std::vector<std::string>&context,
            ValidationResults* results) const {

        // determine type of Adapter, and call appropriate validator.
        // This will throw for any other type of adapter.
        auto target2 = dynamic_cast<PropertyTreeAdapter const * const>(&target);
        if (target2) 
            return validate2(*target2, context, results);
         else 
            return validate2(dynamic_cast<const JsonCppAdapter &>(target), context, results);
        
    }

    template <typename DocType>
    bool validate2(const DocType &target, const std::vector<std::string> &context, ValidationResults *results) const {
        RootResults<DocType> &presult = dynamic_cast<RootResults<DocType >&>(*results);

        const DocType base(jpath.resolve(presult.root));
        Path local(target.asString());
        std::string err(local.resolveErr(base));
        if (err.length()) {
            if (results)
                results->pushError(context, err);
            return false;
        }
        return true;
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


class PathConstraintBuilder : public valijson::ConstraintBuilder {
     std::unique_ptr<Constraint> make (adapters::Adapter &val) {
         return std::unique_ptr<Constraint>(new PathConstraint(val.asString()));
     }
};

void
readTree(const std::string& json, Json::Value& doc) {
    Json::Reader json_reader;
    if (!json_reader.parse(json, doc)) {
        throw std::runtime_error(
            "json parse failure\n" + json_reader.getFormattedErrorMessages());
    }
}

void
readTree(const std::string& json, boost::property_tree::ptree& ptree) {
    std::istringstream gstream(json);
    boost::property_tree::json_parser::read_json(gstream, ptree);
}

template <typename AdapterType, typename DocType>
int doTest(const AdapterType &schemaDoc, const DocType &doc) {

    SchemaParser parser;
    PathConstraintBuilder pcb;
    parser.addConstraintBuilder("jsonpath", pcb);
    Schema schema;
    parser.populateSchema(schemaDoc, schema);

    RootResults<DocType> results(doc) ;
    Validator validator;
    std::regex John("Failed.*John");
    std::regex Jane("Failed.*Jane");
    if (validator.validate(schema, doc, &results)) {
        deblog("oops, should have failed " << err2String(results));
    } else {
        std::string err(err2String(results));
        deblog("error out:\n" << err << "\n:");
        if (regex_search(err, John)) {
            deblog("complained about John");
        } else if (!regex_search(err, Jane)) {
            deblog("failed to complain about Jane");
        } else {
            deblog("success");
        }
    }
}

int main(int argc,char ** argv) {
    Json::Value jschema;
    readTree(emplschema,jschema);
    Json::Value jdoc;
    readTree(emplrec,jdoc);

    doTest(JsonCppAdapter(jschema),JsonCppAdapter(jdoc));

    boost::property_tree::ptree tschema;
    readTree(emplschema,tschema);
    boost::property_tree::ptree tdoc;
    readTree(emplrec,tdoc);

    doTest(PropertyTreeAdapter(tschema),PropertyTreeAdapter(tdoc));


    doTest(JsonCppAdapter(jschema),PropertyTreeAdapter(tdoc));
    doTest(PropertyTreeAdapter(tschema),JsonCppAdapter(jdoc));
}

#include "temp/AdapterPath.cc"
