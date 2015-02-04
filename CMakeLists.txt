cmake_minimum_required (VERSION 2.6)
project (valijson)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.55.0 COMPONENTS regex REQUIRED)

# jsoncpp library
add_library(jsoncpp
    thirdparty/jsoncpp-0.5.0/src/lib_json/json_reader.cpp
    thirdparty/jsoncpp-0.5.0/src/lib_json/json_value.cpp
    thirdparty/jsoncpp-0.5.0/src/lib_json/json_writer.cpp
)

# Build local gtest
add_subdirectory(thirdparty/gtest-1.7.0)

# Include path
include_directories(
  include
  thirdparty/gtest-1.7.0/include
  thirdparty/jsoncpp-0.5.0/include
  thirdparty/rapidjson-0.1/include
  ${Boost_INCLUDE_DIRS}
)

# External schema validation example
add_executable(external_schema
    examples/external_schema.cpp
)

# Unit tests executable
add_executable(test_suite
    tests/test_adapter_comparison.cpp
    tests/test_dereference_callback.cpp
    tests/test_jsoncpp_adapter.cpp
    tests/test_property_tree_adapter.cpp
    tests/test_rapidjson_adapter.cpp
    tests/test_uri_resolution.cpp
    tests/test_validation_errors.cpp
    tests/test_validator.cpp
)

set(TEST_LIBS gtest gtest_main jsoncpp)

target_link_libraries(test_suite ${TEST_LIBS} ${Boost_LIBRARIES})

target_link_libraries(external_schema ${Boost_LIBRARIES})
