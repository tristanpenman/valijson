#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

using valijson::Schema;
using valijson::SchemaParser;
using valijson::ValidationResults;
using valijson::Validator;
using valijson::adapters::AdapterTraits;
using valijson::adapters::RapidJsonAdapter;
using AdapterType = RapidJsonAdapter;

void runOneTest(const AdapterType &test, const Schema &schema,
                Validator::TypeCheckingMode mode)
{
    try {
        if (!test.isObject()) {
            return;
        }

        const AdapterType::Object testObject = test.getObject();
        const auto dataItr = testObject.find("data");

        if (dataItr == testObject.end()) {
            return;
        }

        Validator validator(mode);
        ValidationResults results;
        validator.validate(schema, dataItr->second, &results);
    } catch (const std::exception &) {
    }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    AdapterTraits<AdapterType>::DocumentType document;
    document.template Parse<rapidjson::kParseIterativeFlag>(reinterpret_cast<const char *>(data), size);

    if (document.HasParseError() || !document.IsArray()) {
        return 0;
    }

    for (const auto &testCase : AdapterType(document).getArray()) {
        if (!testCase.isObject()) {
            continue;
        }

        const AdapterType::Object object = testCase.getObject();
        const auto schemaItr = object.find("schema");
        const auto testsItr = object.find("tests");

        if (schemaItr == object.end() || testsItr == object.end() ||
            !testsItr->second.isArray()) {
            continue;
        }

        Schema schema;
        SchemaParser parser(size % 2 ? SchemaParser::kDraft4
                                     : SchemaParser::kDraft7);

        try {
            parser.populateSchema(schemaItr->second, schema);
        } catch (const std::exception &) {
            continue;
        }

        const auto mode = testsItr->second.hasStrictTypes()
                              ? Validator::kStrongTypes
                              : Validator::kWeakTypes;

        for (const AdapterType test : testsItr->second.getArray()) {
            runOneTest(test, schema, mode);
        }
    }

    return 0;
}
