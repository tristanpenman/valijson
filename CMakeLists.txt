cmake_minimum_required (VERSION 2.6)
project (valijson)

option (INSTALL_HEADERS "Install valijson Headers." TRUE)
option (BUILD_EXAMPLES "Build valijson Examples." FALSE)
option (BUILD_TESTS "Build valijson Tests." FALSE)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall ")
SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
if(VALIJSON_CXX11_ADAPTERS STREQUAL "disabled")
    message(STATUS "Building with C++11 support disabled")
else()
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
    if(COMPILER_SUPPORTS_CXX11)
        set(VALIJSON_CXX11_ADAPTERS_ARE_ENABLED 1)
        message(STATUS "Building with C++11 support enabled")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DVALIJSON_BUILD_CXX11_ADAPTERS=1")
    else()
        message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
    endif()
endif()

add_definitions(-DBOOST_ALL_DYN_LINK)
set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)

find_package(Boost 1.55.0 COMPONENTS regex REQUIRED)

# jsoncpp library
add_library(jsoncpp
    thirdparty/jsoncpp-0.9.4/src/lib_json/json_reader.cpp
    thirdparty/jsoncpp-0.9.4/src/lib_json/json_value.cpp
    thirdparty/jsoncpp-0.9.4/src/lib_json/json_writer.cpp
)

if(VALIJSON_CXX11_ADAPTERS_ARE_ENABLED)
    add_library(json11
        thirdparty/json11-2016-01-26/json11.cpp
        )
endif()

# Build local gtest
set(gtest_force_shared_crt ON)
add_subdirectory(thirdparty/gtest-1.7.0)

# Include path
include_directories(
  include
)

include_directories(SYSTEM 
  ${Boost_INCLUDE_DIRS}
  thirdparty/gtest-1.7.0/include
  thirdparty/jsoncpp-0.9.4/include
  thirdparty/rapidjson-1.0.2/include
  thirdparty/picojson-1.3.0
  thirdparty/nlohmann-json-1.1.0
)
if(VALIJSON_CXX11_ADAPTERS_ARE_ENABLED)
    include_directories(SYSTEM
        thirdparty/json11-2016-01-26
        )
endif()

# Custom schema validation example
add_executable(custom_schema
    examples/custom_schema.cpp
)

# External schema validation example
add_executable(external_schema
    examples/external_schema.cpp
)

set(TEST_SOURCES
    tests/test_adapter_comparison.cpp
    tests/test_fetch_document_callback.cpp
    tests/test_json_pointer.cpp
    tests/test_jsoncpp_adapter.cpp
    tests/test_property_tree_adapter.cpp
    tests/test_rapidjson_adapter.cpp
    tests/test_picojson_adapter.cpp
    tests/test_validation_errors.cpp
    tests/test_validator.cpp
    tests/test_poly_constraint.cpp
    )

if(VALIJSON_CXX11_ADAPTERS_ARE_ENABLED)
    set(TEST_SOURCES
        ${TEST_SOURCES}
        tests/test_json11_adapter.cpp
        tests/test_nlohmann_json_adapter.cpp
        )
endif()

# Unit tests executable
add_executable(test_suite ${TEST_SOURCES})

# Definition for using picojson
set_target_properties(test_suite
    PROPERTIES COMPILE_DEFINITIONS "PICOJSON_USE_INT64"
)

set(TEST_LIBS gtest gtest_main jsoncpp)
if(VALIJSON_CXX11_ADAPTERS_ARE_ENABLED)
    set(TEST_LIBS ${TEST_LIBS} json11)
endif()

target_link_libraries(test_suite ${TEST_LIBS} ${Boost_LIBRARIES})
target_link_libraries(custom_schema ${Boost_LIBRARIES})
target_link_libraries(external_schema ${Boost_LIBRARIES})

