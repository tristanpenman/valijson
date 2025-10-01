#include <nlohmann/json.hpp>

#include "valijson_nlohmann_bundled.hpp"

using namespace std;
using namespace valijson;
using namespace valijson::adapters;

int main(int argc, char *argv[])
{
    if (argc != 3) {
        cerr << "Usage: " << argv[0] << " <schema document> <test/target document>" << endl;
        return 1;
    }

    // Load the document containing the schema
    nlohmann::json schemaDocument;
    if (!valijson::utils::loadDocument(argv[1], schemaDocument)) {
        cerr << "Failed to load schema document." << endl;
        return 1;
    }

    // Load the document that is to be validated
    nlohmann::json targetDocument;
    if (!valijson::utils::loadDocument(argv[2], targetDocument)) {
        cerr << "Failed to load target document." << endl;
        return 1;
    }

    // Parse the json schema into an internal schema format
    Schema schema;
    SchemaParser parser;
    NlohmannJsonAdapter schemaAdapter(schemaDocument);
    try {
        parser.populateSchema(schemaAdapter, schema);
    } catch (std::exception &e) {
        cerr << "Failed to parse schema: " << e.what() << endl;
        return 1;
    }

    // Perform validation
    Validator validator(Validator::kStrongTypes);
    ValidationResults results;
    NlohmannJsonAdapter targetAdapter(targetDocument);
    if (validator.validate(schema, targetAdapter, &results)) {
        std::cerr << "Validation succeeded." << std::endl;
        return 0;
    }

    std::cerr << "Validation failed." << std::endl;
    valijson::ValidationResults::Error error;
    unsigned int errorNum = 1;
    while (results.popError(error)) {
        std::cerr << "Error #" << errorNum << std::endl;
        std::cerr << " @ " << error.jsonPointer << std::endl;
        std::cerr << " - " << error.description << std::endl;
        ++errorNum;
    }

    return 1;
}
