#include <gtest/gtest.h>

#include <valijson/internal/json_reference.hpp>

#include <valijson/adapters/rapidjson_adapter.hpp>

using valijson::adapters::RapidJsonAdapter;
using valijson::internal::json_reference::resolveJsonPointer;

namespace {
    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> allocator;
}

class TestJsonReference : public testing::Test
{

};

TEST_F(TestJsonReference, PointerWithoutLeadingSlashShouldThrow)
{
    // Given an Adapter for a JSON object
    rapidjson::Value value;
    value.SetObject();
    const RapidJsonAdapter rootAdapter(value);

    // When a JSON Pointer that does not begin with a slash is resolved
    // Then an exception should be thrown
    EXPECT_THROW(resolveJsonPointer(rootAdapter, "#"), std::runtime_error);
}

TEST_F(TestJsonReference, RootPointer)
{
    // Given an Adapter for a JSON object containing one attribute
    rapidjson::Value value;
    value.SetObject();
    value.AddMember("test", "test", allocator);
    const RapidJsonAdapter rootAdapter(value);

    // When a JSON Pointer that points to the root node is resolved
    const RapidJsonAdapter resultAdapter = resolveJsonPointer(rootAdapter, "/");

    // Then the returned Adapter should also refer to the root node
    EXPECT_TRUE(resultAdapter.isObject());
    const RapidJsonAdapter::Object object = resultAdapter.asObject();
    EXPECT_NE(object.end(), object.find("test"));
}
