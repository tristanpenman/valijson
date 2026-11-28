#include <cstdio>
#include <string>

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
using DocumentType = AdapterTraits<AdapterType>::DocumentType;

namespace {

/// Versions of JSON Schema that schemas are parsed with
constexpr SchemaParser::Version kVersions[] = {
    SchemaParser::kDraft3,
    SchemaParser::kDraft4,
    SchemaParser::kDraft6,
    SchemaParser::kDraft7,
};

/// Maximum depth of a document that is copied for a remote reference
constexpr size_t kMaxCopyDepth = 1000;

/// Parses JSON text into a document, which is the only adapter-specific code
bool parseDocument(const char *data, size_t size, DocumentType &document)
{
    document.Parse<rapidjson::kParseIterativeFlag>(data, size);
    return !document.HasParseError();
}

void writeString(const std::string &value, std::string &json)
{
    json += '"';
    for (const char c : value) {
        if (c == '"' || c == '\\') {
            json += '\\';
            json += c;
        } else if (static_cast<unsigned char>(c) < 0x20) {
            char escaped[7];
            std::snprintf(escaped, sizeof(escaped), "\\u%04x", c);
            json += escaped;
        } else {
            json += c;
        }
    }
    json += '"';
}

/// Serializes a value to JSON text, so that it can be parsed into a document
bool writeJson(const AdapterType &value, std::string &json, size_t depth = 0)
{
    if (depth > kMaxCopyDepth) {
        return false;
    }

    if (value.isNull()) {
        json += "null";
    } else if (value.isBool()) {
        json += value.getBool() ? "true" : "false";
    } else if (value.isInteger()) {
        json += std::to_string(value.getInteger());
    } else if (value.isDouble()) {
        char number[32];
        std::snprintf(number, sizeof(number), "%.17g", value.getDouble());
        json += number;
    } else if (value.isString()) {
        writeString(value.getString(), json);
    } else if (value.isArray()) {
        json += '[';
        bool first = true;
        for (const auto &item : value.getArray()) {
            if (!first) {
                json += ',';
            }
            first = false;
            if (!writeJson(item, json, depth + 1)) {
                return false;
            }
        }
        json += ']';
    } else if (value.isObject()) {
        json += '{';
        bool first = true;
        for (const auto &member : value.getObject()) {
            if (!first) {
                json += ',';
            }
            first = false;
            writeString(member.first, json);
            json += ':';
            if (!writeJson(member.second, json, depth + 1)) {
                return false;
            }
        }
        json += '}';
    } else {
        return false;
    }

    return true;
}

void runOneTest(const AdapterType &test, const Schema &schema)
{
    if (!test.isObject()) {
        return;
    }

    const AdapterType::Object testObject = test.getObject();
    const auto dataItr = testObject.find("data");
    if (dataItr == testObject.end()) {
        return;
    }

    for (const auto mode : {Validator::kStrongTypes, Validator::kWeakTypes}) {
        try {
            Validator validator(mode);
            ValidationResults results;
            validator.validate(schema, dataItr->second, &results);
        } catch (const std::exception &) {
        }
    }
}

void runTestCases(const AdapterType &testCases, size_t size)
{
    if (!testCases.isArray()) {
        return;
    }

    for (const auto &testCase : testCases.getArray()) {
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

        // Test cases provide the schemas they reference, keyed by URI, in
        // their 'externalSchemas' property
        const auto externalSchemasItr = object.find("externalSchemas");
        const bool hasExternalSchemas = externalSchemasItr != object.end() &&
                                        externalSchemasItr->second.isObject();

        const auto fetchDoc = [&](const std::string &uri) -> const DocumentType * {
            if (!hasExternalSchemas) {
                return nullptr;
            }

            const AdapterType::Object externalSchemas =
                externalSchemasItr->second.getObject();
            const auto itr = externalSchemas.find(uri);
            std::string json;
            if (itr == externalSchemas.end() || !writeJson(itr->second, json)) {
                return nullptr;
            }

            auto *document = new DocumentType();
            if (!parseDocument(json.data(), json.size(), *document)) {
                delete document;
                return nullptr;
            }

            return document;
        };
        const auto freeDoc = [](const DocumentType *document) {
            delete document;
        };

        Schema schema;
        SchemaParser parser(kVersions[size % (sizeof(kVersions) / sizeof(kVersions[0]))]);
        try {
            parser.populateSchema(schemaItr->second, schema, fetchDoc, freeDoc);
        } catch (const std::exception &) {
            continue;
        }

        for (const AdapterType test : testsItr->second.getArray()) {
            runOneTest(test, schema);
        }
    }
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    DocumentType document;
    if (!parseDocument(reinterpret_cast<const char *>(data), size, document)) {
        return 0;
    }

    // Test cases are contained in the 'tests' property of each file in the
    // JSON-Schema-Test-Suite. A top-level array of test cases, as used by
    // earlier versions of the test suite, is also accepted.
    const AdapterType root(document);
    if (root.isObject()) {
        const AdapterType::Object rootObject = root.getObject();
        const auto testCasesItr = rootObject.find("tests");
        if (testCasesItr != rootObject.end()) {
            runTestCases(testCasesItr->second, size);
        }
    } else {
        runTestCases(root, size);
    }

    return 0;
}
