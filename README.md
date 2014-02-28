Valijson
========

Overview
--------

Valijson is a header-only [JSON Schema](http://json-schema.org/) Validation library for C++.

Valijson provides a simple validation API that allows you load JSON Schemas, and validate documents loaded by one of several supported parser libraries.

License
-------

Valijson is licensed under the Simplified BSD License. See the LICENSE file
for more information.

Dependencies
------------

Required:

 - boost 1.54

Valijson supports JSON documents loaded using JsonCpp, RapidJson and Boost Property Tree. It has been tested against the following versions of these libraries:

 - [boost::property\_tree 1.54](http://www.boost.org/doc/libs/1_54_0/doc/html/boost_propertytree/synopsis.html)
 - [jsoncpp 0.5.0](http://downloads.sourceforge.net/project/jsoncpp/jsoncpp/0.5.0/jsoncpp-src-0.5.0.tar.gz)
 - [rapidjson 0.1](https://code.google.com/p/rapidjson/downloads/detail?name=rapidjson-0.1.zip)

Other versions of these libraries may work, but have not been tested.

JSON Schema Support
-------------------

Valijson supports most of the constraints defined in [Draft 3](http://tools.ietf.org/search/draft-zyp-json-schema-03) and [Draft 4](http://tools.ietf.org/search/draft-zyp-json-schema-04) of the JSON Schema specification.

The exceptions for Draft 3 are:

 - disallow
 - divisibleBy
 - extends
 - format (optional)
 - readonly
 - ref
 - refRemote

The exceptions for Draft 4 are:

 - definitions
 - format (optional)
 - multipleOf
 - ref
 - refRemote

Support for JSON References is in development.

Build instructions
------------------

A rudimentary Makefile has been included. Running 'make examples' will build the example programs in the 'examples' directory. Running 'make check' will build and run the test suite. Executables will be placed in 'bin'.

An Xcode 5 project has also been provided, in the 'xcode' directory. Note that in order to run the test suite, you may need to configure the working directory for the 'test\_suite' scheme.

The Xcode project has been configured so that /usr/local/include is in the include path, and /usr/local/lib is in the library path. These are the locations that homebrew installed Boost on my test system.

Doxygen documentation can be built by running 'doxygen' from the project root directory. Generated documentation will be placed in 'doc/html'. Other relevant documentation such as schemas and specifications have been included in the 'doc' directory.
