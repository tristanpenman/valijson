# Valijson [![Build Status](https://travis-ci.org/tristanpenman/valijson.svg?branch=master)](https://travis-ci.org/tristanpenman/valijson) #

## Overview ##

Valijson is a header-only [JSON Schema](http://json-schema.org/) Validation library for C++.

Valijson provides a simple validation API that allows you load JSON Schemas, and validate documents loaded by one of several supported parser libraries.

## Project Goals ##

The goal of this project is to support validation of all constraints available in JSON Schema v4, while being competitive with the performance of hand-written JSON validators. The library is intended to include support for remote JSON References via a callback interface.

## Usage ##

The following code snippets show how you might implement a simple validator using RapidJson as the underlying JSON Parser.

Include the necessary headers:

    #include <rapidjson/document.h>

    #include <valijson/adapters/rapidjson_adapter.hpp>
    #include <valijson/utils/rapidjson_utils.hpp>
    #include <valijson/schema.hpp>
    #include <valijson/schema_parser.hpp>
    #include <valijson/validator.hpp>

Loading a schema using RapidJSON:

    // Load JSON document using RapidJSON with Valijson helper function
    rapidjson::Document mySchemaDoc;
    if (!valijson::utils::loadDocument("mySchema.json", mySchemaDoc)) {
        throw std::runtime_error("Failed to load schema document");
    }

    // Parse JSON schema content using valijson
    Schema mySchema;
    SchemaParser parser;
    RapidJsonAdapter mySchemaAdapter(mySchemaDoc);
    parser.populateSchema(mySchemaAdapter, mySchema);

Load a document to validate:

    rapidjson::Document myTargetDoc;
    if (!valijson::utils::loadDocument("myTarget.json", myTargetDoc)) {
        throw std::runtime_error("Failed to load target document");
    }

Validate a document:

    Validator validator(mySchema);
    RapidJsonAdapter myTargetAdapter(myTargetDoc);
    if (!validator.validate(myTargetAdapter, NULL)) {
        std::runtime_error("Validation failed.");
    }

Note that Valijson's `SchemaParser` and `Validator` classes expect you to pass in a `RapidJsonAdapter` rather than a `rapidjson::Document`. This is due to the fact that `SchemaParser` and `Validator` are template classes that can be used with any of the JSON parsers supported by Valijson.

## Test Suite ##

Valijson's' test suite currently contains several hand-crafted tests and uses the standard [JSON Schema Test Suite](https://github.com/json-schema/JSON-Schema-Test-Suite) to test support for parts of the JSON Schema feature set that have been implemented.

### cmake ###

The examples and test suite can be built using cmake:

    # Build examples and test suite
    mkdir build
    cd build
    cmake ..
    make

    # Run test suite (from build directory)
    ./test_suite

### Xcode ###

An Xcode project has also been provided, in the 'xcode' directory. Note that in order to run the test suite, you may need to configure the working directory for the 'test\_suite' scheme. It is recommended that you use the 'xcode' directory as the working directory.

The Xcode project has been configured so that /usr/local/include is in the include path, and /usr/local/lib is in the library path. These are the locations that homebrew installed Boost on my test system.

## Examples ##

Building the Valijson Test Suite, using the instructions above, will also compile two example applications: `custom_schema` and `external_schema`.

`custom_schema` shows how you can hard-code a schema definition into an application, while `external_schema` builds on the example code above to show you how to validate and document and report on any validation errors.

## JSON Schema Support ##

Valijson supports most of the constraints defined in [Draft 3](http://tools.ietf.org/search/draft-zyp-json-schema-03) and [Draft 4](http://tools.ietf.org/search/draft-zyp-json-schema-04) of the JSON Schema specification.

The exceptions for Draft 3 are:

 - disallow
 - extends
 - format (optional)
 - readonly
 - ref
 - refRemote

The exceptions for Draft 4 are:

 - definitions
 - format (optional)
 - ref
 - refRemote

Support for JSON References is in development.

## Documentation ##

Doxygen documentation can be built by running 'doxygen' from the project root directory. Generated documentation will be placed in 'doc/html'. Other relevant documentation such as schemas and specifications have been included in the 'doc' directory.

## Dependencies ##

Required:

 - boost 1.54

Later versions of boost (up to 1.57) are also known to work correctly.

## Supported Parsers ##

Valijson supports JSON documents loaded using JsonCpp, RapidJson, Boost Property Tree and PicoJSON. It has been tested against the following versions of these libraries:

 - [boost::property_tree 1.54](http://www.boost.org/doc/libs/1_54_0/doc/html/boost_propertytree/synopsis.html)
 - [jsoncpp 0.9.4](https://github.com/open-source-parsers/jsoncpp/archive/0.9.4.tar.gz)
 - [rapidjson 0.1](https://code.google.com/p/rapidjson/downloads/detail?name=rapidjson-0.1.zip)
 - [PicoJSON 1.3.0](https://github.com/kazuho/picojson/archive/v1.3.0.tar.gz)

Version of JsonCpp going back to 0.5.0 should also work correctly, but versions from 1.0 onwards have not yet been tested.

Also note that when using PicoJSON, it may be necessary to include the `picojson.h` before other headers to ensure that the appropriate macros have been enabled.

Other versions of these libraries may work, but have not been tested.

## License ##

Valijson is licensed under the Simplified BSD License.

See the LICENSE file for more information.
