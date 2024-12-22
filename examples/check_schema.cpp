/**
 * @file
 *
 * @brief Loads a schema then exits. Exit code will be 0 if the schema is
 *        valid, and 1 otherwise. This example uses jsoncpp to parse the
 *        schema document.
 */

#include <iostream>

#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>

using std::cerr;
using std::endl;

using valijson::Schema;
using valijson::SchemaParser;
using valijson::adapters::JsonCppAdapter;

int main(int argc, char *argv[])
{
    if (argc != 2) {
        cerr << "Usage: " << argv[0] << " <schema document>" << endl;
        return 1;
    }

    // Load the document containing the schema
    Json::Value schemaDocument;
    if (!valijson::utils::loadDocument(argv[1], schemaDocument)) {
        cerr << "Failed to load schema document." << endl;
        return 1;
    }

    Schema schema;
    SchemaParser parser;
    JsonCppAdapter adapter(schemaDocument);
    try {
        parser.populateSchema(adapter, schema);
    } catch (std::exception &e) {
        cerr << "Failed to parse schema: " << e.what() << endl;
        return 1;
    }

    return 0;
}
