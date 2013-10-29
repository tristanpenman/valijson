Valijson
========

Overview
--------

Valijson is a header-only JSON Schema Validation library for C++.

Valijson provides a simple validation API that allows you load schemas, and 
validate documents created by one of several supported parser libraries.

License
-------

Valijson is licensed under the Simplified BSD License. See the LICENSE file
for more information.

Supported parsers
-----------------

Valijson supports JsonCpp, RapidJson and Boost Property Trees. It has been
tested against the following versions of these libraries:
 - jsoncpp 0.5.0
 - rapidjson 0.1
 - boost::property_tree 1.54

Dependencies
------------

Required:
 - boost 1.54 (earlier versions may work, but have not been tested)

Optional
 - gtest 1.6.0 (for building unit tests)
 - jsoncpp 0.5.0 (for use of the JsonCppAdapter class)
 - rapidjson 0.1 (for use of the RapidJsonAdapter class)

JSON Schema Support
-------------------

Valijson supports most of the constraints defined in Drafts 3 and 4 of the JSON
Schema specification.

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

A rudimentary Makefile has been included. Running 'make examples' will build
the example programs in the 'examples' directory. Running 'make check' will
build and run the gtest-based unit tests. Executables will be placed in 'bin'.

An Xcode 5 project has also been provided, in the 'xcode' directory. Note that
in order to run the test suite, you may need to configure the working directory
for the 'test_suite' scheme.

The Xcode project has been configured so that /usr/local/include is in the 
include path, and /usr/local/lib is in the library path. These are the 
locations that homebrew installed Boost on my test system.

Doxygen documentation can be built by running 'doxygen' from the project root
directory. Generated documentation will be placed in 'doc/html'. Other relevant
documentation such as schemas and specifications have been included in the 'doc'
directory.
