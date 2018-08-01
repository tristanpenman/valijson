cmake_minimum_required (VERSION 3.1)
project (valijson)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

option(INSTALL_HEADERS "Install valijson Headers." FALSE)
option(BUILD_EXAMPLES "Build valijson Examples." FALSE)
option(BUILD_TESTS "Build valijson Tests." TRUE)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

if(INSTALL_HEADERS)
    install(DIRECTORY include/ DESTINATION include)
endif()

if(NOT BUILD_TESTS AND NOT BUILD_EXAMPLES)
    return()
endif()

SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DVALIJSON_BUILD_CXX11_ADAPTERS=1")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

add_definitions(-DBOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.54.0 REQUIRED)
find_package(Poco OPTIONAL_COMPONENTS JSON)
find_package(Qt5Core)

# jsoncpp library
add_library(jsoncpp
    thirdparty/jsoncpp-0.9.4/src/lib_json/json_reader.cpp
    thirdparty/jsoncpp-0.9.4/src/lib_json/json_value.cpp
    thirdparty/jsoncpp-0.9.4/src/lib_json/json_writer.cpp
)

target_include_directories(jsoncpp SYSTEM PRIVATE thirdparty/jsoncpp-0.9.4/include)
set_target_properties(jsoncpp PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/thirdparty/jsoncpp-0.9.4)

add_library(json11
    thirdparty/json11-ec4e452/json11.cpp
)

target_include_directories(json11 SYSTEM PRIVATE thirdparty/json11-ec4e452)
set_target_properties(json11 PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/thirdparty/json11-ec4e452)

# Build local gtest
set(gtest_force_shared_crt ON)
add_subdirectory(thirdparty/gtest-1.7.0)

# Include path
include_directories(include)
include_directories(SYSTEM
    thirdparty/gtest-1.7.0/include
    thirdparty/json11-ec4e452
    thirdparty/jsoncpp-0.9.4/include
    thirdparty/rapidjson-1.0.2/include
    thirdparty/picojson-1.3.0
    thirdparty/nlohmann-json-3.1.2
    ${Boost_INCLUDE_DIRS}
    ${Qt5Core_INCLUDE_DIRS}
)

# Custom schema validation example
add_executable(custom_schema
    examples/custom_schema.cpp
)

# External schema validation example
add_executable(external_schema
    examples/external_schema.cpp
)

add_executable(array_iteration_basics
    examples/array_iteration_basics.cpp
)

add_executable(array_iteration_template_fn
    examples/array_iteration_template_fn.cpp
)

add_executable(object_iteration
    examples/object_iteration.cpp
)

add_executable(json_pointers
    examples/json_pointers.cpp
)

add_executable(remote_resolution
    examples/remote_resolution.cpp
)

set(TEST_SOURCES
    tests/test_adapter_comparison.cpp
    tests/test_fetch_document_callback.cpp
    tests/test_json_pointer.cpp
    tests/test_json11_adapter.cpp
    tests/test_jsoncpp_adapter.cpp
    tests/test_nlohmann_json_adapter.cpp
    tests/test_property_tree_adapter.cpp
    tests/test_rapidjson_adapter.cpp
    tests/test_picojson_adapter.cpp
    tests/test_poco_json_adapter.cpp
    tests/test_validation_errors.cpp
    tests/test_validator.cpp
    tests/test_poly_constraint.cpp
)

if(Qt5Core_FOUND)
    list(APPEND TEST_SOURCES tests/test_qtjson_adapter.cpp)
endif()

# Unit tests executable
add_executable(test_suite ${TEST_SOURCES})

# Definition for using picojson
set_target_properties(test_suite PROPERTIES COMPILE_DEFINITIONS "PICOJSON_USE_INT64")

set(TEST_LIBS gtest gtest_main jsoncpp json11)

if (Qt5Core_FOUND)
    list(APPEND TEST_LIBS Qt5::Core)
    target_compile_definitions(test_suite PRIVATE "VALIJSON_BUILD_QT_ADAPTERS")
endif()

if(Poco_FOUND)
    list(APPEND TEST_LIBS ${Poco_Foundation_LIBRARIES} ${Poco_JSON_LIBRARIES})
    target_compile_definitions(test_suite PRIVATE "VALIJSON_BUILD_POCO_ADAPTERS")
endif()

target_link_libraries(test_suite ${TEST_LIBS} ${Boost_LIBRARIES})
target_link_libraries(custom_schema ${Boost_LIBRARIES})
target_link_libraries(external_schema ${Boost_LIBRARIES})
target_link_libraries(array_iteration_basics jsoncpp)
target_link_libraries(array_iteration_template_fn jsoncpp)
target_link_libraries(object_iteration jsoncpp)
target_link_libraries(json_pointers)
target_link_libraries(remote_resolution curl curlpp)
