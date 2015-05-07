#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <gtest/gtest.h>

#include <valijson/internal/json_pointer.hpp>

#include <valijson/adapters/rapidjson_adapter.hpp>

using valijson::adapters::RapidJsonAdapter;
using valijson::internal::json_pointer::resolveJsonPointer;

typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator>
        RapidJsonCrtAllocator;

class TestJsonPointer : public testing::Test
{

};

struct JsonPointerTestCase
{
    JsonPointerTestCase(const std::string &description)
      : description(description) { }

    /// Description of test case
    std::string description;

    /// Document to traverse when resolving the JSON Pointer
    rapidjson::Value value;

    /// JSON Pointer that should guide traversal of the document
    std::string jsonPointer;

    /// Optional reference to the expected result from the original document
    rapidjson::Value *expectedValue;
};

std::vector<boost::shared_ptr<JsonPointerTestCase> >
        testCasesForSingleLevelObjectPointers(
                RapidJsonCrtAllocator &allocator)
{
    typedef boost::shared_ptr<JsonPointerTestCase> TestCase;

    std::vector<TestCase> testCases;

    TestCase testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolving '#' should cause an exception to be thrown");
    testCase->value.SetNull();
    testCase->jsonPointer = "#";
    testCase->expectedValue = NULL;
    testCases.push_back(testCase);

    testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolving an empty string should return the root node");
    testCase->value.SetNull();
    testCase->jsonPointer = "";
    testCase->expectedValue = &testCase->value;
    testCases.push_back(testCase);

    testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolving '/' should return the root node");
    testCase->value.SetNull();
    testCase->jsonPointer = "/";
    testCase->expectedValue = &testCase->value;
    testCases.push_back(testCase);

    testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolving '//' should return the root node");
    testCase->value.SetNull();
    testCase->jsonPointer = "//";
    testCase->expectedValue = &testCase->value;
    testCases.push_back(testCase);

    testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolve '/test' in object containing one member named 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "/test";
    testCase->expectedValue = &testCase->value.FindMember("test")->value;
    testCases.push_back(testCase);

    testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolve '/test/' in object containing one member named 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "/test/";
    testCase->expectedValue = &testCase->value.FindMember("test")->value;
    testCases.push_back(testCase);

    testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolve '//test//' in object containing one member named 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "//test//";
    testCase->expectedValue = &testCase->value.FindMember("test")->value;
    testCases.push_back(testCase);

    testCase = boost::make_shared<JsonPointerTestCase>(
            "Resolve '/missing' in object containing one member name 'test'");
    testCase->value.SetObject();
    testCase->value.AddMember("test", "test", allocator);
    testCase->jsonPointer = "/missing";
    testCase->expectedValue = NULL;
    testCases.push_back(testCase);

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = boost::make_shared<JsonPointerTestCase>(
                "Resolve '/test/0' in object containing one member containing "
                "an array with 3 elements");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/0";
        testCase->expectedValue = &testCase->value.FindMember("test")->value[rapidjson::SizeType(0)];
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = boost::make_shared<JsonPointerTestCase>(
                "Resolve '/test/1' in object containing one member containing "
                "an array with 3 elements");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/1";
        testCase->expectedValue = &testCase->value.FindMember("test")->value[rapidjson::SizeType(1)];
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = boost::make_shared<JsonPointerTestCase>(
                "Resolve '/test/2' in object containing one member containing "
                "an array with 3 elements");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/2";
        testCase->expectedValue = &testCase->value.FindMember("test")->value[rapidjson::SizeType(2)];
        testCases.push_back(testCase);
    }

    {
        rapidjson::Value testArray;
        testArray.SetArray();
        testArray.PushBack("test0", allocator);
        testArray.PushBack("test1", allocator);
        testArray.PushBack("test2", allocator);

        testCase = boost::make_shared<JsonPointerTestCase>(
                "Resolveing '/test/3' in object containing one member containing "
                "an array with 3 elements should throw an exception");
        testCase->value.SetObject();
        testCase->value.AddMember("test", testArray, allocator);
        testCase->jsonPointer = "/test/3";
        testCase->expectedValue = NULL;
        testCases.push_back(testCase);
    }

    return testCases;
}

TEST_F(TestJsonPointer, JsonPointerTestCases)
{
    typedef std::vector<boost::shared_ptr<JsonPointerTestCase> > TestCases;

    // Ensure memory used for test cases is freed when test function completes
    rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> allocator;

    TestCases testCases = testCasesForSingleLevelObjectPointers(allocator);

    for (TestCases::const_iterator itr = testCases.begin();
            itr != testCases.end(); ++itr) {
        const std::string &jsonPointer = (*itr)->jsonPointer;
        const RapidJsonAdapter valueAdapter((*itr)->value);
        if ((*itr)->expectedValue) {
            const RapidJsonAdapter expectedAdapter(*((*itr)->expectedValue));
            const RapidJsonAdapter actualAdapter =
                    resolveJsonPointer(valueAdapter, jsonPointer);
            EXPECT_TRUE(actualAdapter.equalTo(expectedAdapter, true)) <<
                    (*itr)->description;
        } else {
            EXPECT_THROW(
                    resolveJsonPointer(valueAdapter, jsonPointer),
                    std::runtime_error) <<
                    (*itr)->description;
        }
    }
}
