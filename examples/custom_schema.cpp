/**
 * @file
 *
 * @brief Demonstrates validation against a manually constructed schema.
 *
 * This example demonstrates the construction and composition of a Schema object
 * using manually created Constraint objects. The following Constraint classes
 * are used:
 *  - EnumConstraint
 *  - MaxLengthConstraint
 *  - MinimumConstraint
 *  - MinLengthConstraint
 *  - PropertiesConstraint
 *  - RequiredConstraint
 *  - TypeConstraint
 *
 * The MinimumConstraint class provides support for the exclusiveMinimum and
 * minimum keywords in JSON Schema. And the PropertiesConstraint class provides
 * support for the properties, patternProperties, and additionalProperties
 * keywords.
 *
 * This is the JSON Schema representation of the Schema that will be created:
 *
 *  {
 *    "properties": {
 *      "category": {
 *        "enum": [
 *          "album",
 *          "book",
 *          "other",
 *          "video"
 *        ]
 *      },
 *      "description": {
 *        "type": "string"
 *      },
 *      "price": {
 *        "exclusiveMinimum": true,
 *        "minimum": 0.0,
 *        "type": "number"
 *      },
 *      "title": {
 *        "maxLength": 200,
 *        "minLength": 1,
 *        "type": "string"
 *      }
 *    },
 *    "required": [
 *      "category",
 *      "price",
 *      "title"
 *    ],
 *    "type": "object"
 *  }
 *
 */

#include <fstream>
#include <iostream>
#include <stdexcept>

#include <rapidjson/document.h>

#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validation_results.hpp>
#include <valijson/validator.hpp>

using std::cerr;
using std::endl;

using valijson::Schema;
using valijson::SchemaParser;
using valijson::Validator;
using valijson::ValidationResults;
using valijson::adapters::RapidJsonAdapter;
using valijson::adapters::RapidJsonFrozenValue;
using valijson::constraints::EnumConstraint;
using valijson::constraints::MaxLengthConstraint;
using valijson::constraints::MinimumConstraint;
using valijson::constraints::MinLengthConstraint;
using valijson::constraints::PropertiesConstraint;
using valijson::constraints::RequiredConstraint;
using valijson::constraints::TypeConstraint;

void addPropertiesConstraint(Schema &schema)
{

    PropertiesConstraint::PropertySchemaMap propertySchemaMap;
    PropertiesConstraint::PropertySchemaMap patternPropertiesSchemaMap;

    {
        // Create a child schema for the 'category' property that requires one
        // of several possible values.
        Schema &propertySchema = propertySchemaMap["category"];
        EnumConstraint::Values enumConstraintValues;
        enumConstraintValues.push_back(new RapidJsonFrozenValue("album"));
        enumConstraintValues.push_back(new RapidJsonFrozenValue("book"));
        enumConstraintValues.push_back(new RapidJsonFrozenValue("other"));
        enumConstraintValues.push_back(new RapidJsonFrozenValue("video"));
        propertySchema.addConstraint(new EnumConstraint(enumConstraintValues));
    }

    {
        // Create a child schema for the 'description' property that requires
        // a string, but does not enforce any length constraints.
        Schema &propertySchema = propertySchemaMap["description"];
        propertySchema.addConstraint(new TypeConstraint(TypeConstraint::kString));
    }

    {
        // Create a child schema for the 'price' property, that requires a
        // number with a value greater than zero.
        Schema &propertySchema = propertySchemaMap["price"];
        propertySchema.addConstraint(new MinimumConstraint(0.0, true));
        propertySchema.addConstraint(new TypeConstraint(TypeConstraint::kNumber));
    }

    {
        // Create a child schema for the 'title' property that requires a string
        // that is between 1 and 200 characters in length.
        Schema &propertySchema = propertySchemaMap["title"];
        propertySchema.addConstraint(new MaxLengthConstraint(200));
        propertySchema.addConstraint(new MinLengthConstraint(1));
        propertySchema.addConstraint(new TypeConstraint(TypeConstraint::kString));
    }

    // Add a PropertiesConstraint to the schema, with the properties defined
    // above, no pattern properties, and with additional property schemas
    // prohibited.
    schema.addConstraint(new PropertiesConstraint(
        propertySchemaMap, patternPropertiesSchemaMap));
}

void addRequiredConstraint(Schema &schema)
{
    // Add a RequiredConstraint to the schema, specifying that the category,
    // price, and title properties must be present.
    RequiredConstraint::RequiredProperties requiredProperties;
    requiredProperties.insert("category");
    requiredProperties.insert("price");
    requiredProperties.insert("title");
    schema.addConstraint(new RequiredConstraint(requiredProperties));
}

void addTypeConstraint(Schema &schema)
{
    // Add a TypeConstraint to the schema, specifying that the root of the
    // document must be an object.
    schema.addConstraint(new TypeConstraint(TypeConstraint::kObject));
}

int main(int argc, char *argv[])
{
    // Load the document that is to be validated
    rapidjson::Document targetDocument;
    if (!valijson::utils::loadDocument(argv[1], targetDocument)) {
        cerr << "Failed to load target document." << endl;
        return 1;
    }

    // Populate a schema
    Schema schema;
    addPropertiesConstraint(schema);
    addRequiredConstraint(schema);
    addTypeConstraint(schema);

    // Perform validation
    Validator validator(schema);
    ValidationResults results;
    RapidJsonAdapter targetDocumentAdapter(targetDocument);
    if (!validator.validate(targetDocumentAdapter, &results)) {
        std::cerr << "Validation failed." << endl;
        ValidationResults::Error error;
        unsigned int errorNum = 1;
        while (results.popError(error)) {
            cerr << "Error #" << errorNum << std::endl
                 << "  context: " << error.context << endl
                 << "  desc:    " << error.description << endl;
            ++errorNum;
        }
        return 1;
    }

    return 0;
}