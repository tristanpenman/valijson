#include <boost/foreach.hpp>
#include <boost/lexical_cast.hpp>

#include <gtest/gtest.h>

#include <valijson/adapters/jsoncpp_adapter.hpp>
#include <valijson/adapters/property_tree_adapter.hpp>
#include <valijson/adapters/rapidjson_adapter.hpp>
#include <valijson/adapters/picojson_adapter.hpp>
#include <valijson/utils/jsoncpp_utils.hpp>
#include <valijson/utils/property_tree_utils.hpp>
#include <valijson/utils/rapidjson_utils.hpp>
#include <valijson/utils/picojson_utils.hpp>

#define TEST_DATA_DIR "../tests/data/documents/"

using valijson::adapters::AdapterTraits;

class TestAdapterComparison : public testing::Test
{
protected:

    struct JsonFile
    {
        JsonFile(const std::string &path, int strictGroup, int looseGroup)
          : path(path),
            strictGroup(strictGroup),
            looseGroup(looseGroup) { }

        std::string path;

        int strictGroup;
        int looseGroup;
    };

    static void SetUpTestCase() {

        const std::string testDataDir(TEST_DATA_DIR);

        //
        // Each test is allocated to two groups. The first group is the strict
        // comparison group. All test files that have been assigned to the same
        // group should be equal, when compared using strict types. The second
        // group is the loose comparison group. All tests files in a loose
        // group should be equal, when compared without using strict types.
        //
        // As an example, the first three test files are in the same loose
        // group. This means they are expected to be equal when compared without
        // strict types. However, only the first two files in the same strict
        // group, which means that only they should be equal.
        //
        jsonFiles.push_back(JsonFile(testDataDir + "array_doubles_1_2_3.json",        1,  1));
        jsonFiles.push_back(JsonFile(testDataDir + "array_integers_1_2_3.json",       1,  1));
        jsonFiles.push_back(JsonFile(testDataDir + "array_strings_1_2_3.json",        2,  1));

        jsonFiles.push_back(JsonFile(testDataDir + "array_doubles_1_2_3_4.json",      3,  2));
        jsonFiles.push_back(JsonFile(testDataDir + "array_integers_1_2_3_4.json",     3,  2));
        jsonFiles.push_back(JsonFile(testDataDir + "array_strings_1_2_3_4.json",      4,  2));

        jsonFiles.push_back(JsonFile(testDataDir + "array_doubles_10_20_30_40.json",  5,  3));
        jsonFiles.push_back(JsonFile(testDataDir + "array_integers_10_20_30_40.json", 5,  3));
        jsonFiles.push_back(JsonFile(testDataDir + "array_strings_10_20_30_40.json",  6,  3));
    }

    template<typename Adapter1, typename Adapter2>
    static void testComparison()
    {
        std::vector<JsonFile>::const_iterator outerItr, innerItr;

        for(outerItr = jsonFiles.begin(); outerItr != jsonFiles.end() - 1; ++outerItr) {
            for(innerItr = outerItr; innerItr != jsonFiles.end(); ++innerItr) {

                const bool expectedStrict = (outerItr->strictGroup == innerItr->strictGroup);
                const bool expectedLoose = (outerItr->looseGroup == innerItr->looseGroup);

                typename AdapterTraits<Adapter1>::DocumentType document1;
                ASSERT_TRUE( valijson::utils::loadDocument(outerItr->path, document1) );
                const Adapter1 adapter1(document1);
                const std::string adapter1Name = AdapterTraits<Adapter1>::adapterName();

                typename AdapterTraits<Adapter2>::DocumentType document2;
                ASSERT_TRUE( valijson::utils::loadDocument(innerItr->path, document2) );
                const Adapter2 adapter2(document2);
                const std::string adapter2Name = AdapterTraits<Adapter2>::adapterName();

                // If either adapter does not support strict types, then strict
                // comparison should not be used, UNLESS the adapters are of the
                // same type. If they are of the same type, then the internal
                // type degradation should be the same, therefore strict testing
                // of equality makes sense.
                if (adapter1.hasStrictTypes() && adapter2.hasStrictTypes() && adapter1Name == adapter2Name) {
                    EXPECT_EQ(expectedStrict, adapter1.equalTo(adapter2, true))
                        << "Comparing '" << outerItr->path << "' to '"
                        << innerItr->path << "' "
                        << "with strict comparison enabled";
                    EXPECT_EQ(expectedStrict, adapter2.equalTo(adapter1, true))
                        << "Comparing '" << innerItr->path << "' to '"
                        << outerItr->path << "' "
                        << "with strict comparison enabled";
                }

                EXPECT_EQ(expectedLoose, adapter1.equalTo(adapter2, false))
                    << "Comparing '" << outerItr->path << "' to '"
                    << innerItr->path << "' "
                    << "with strict comparison disabled";
                EXPECT_EQ(expectedLoose, adapter2.equalTo(adapter1, false))
                    << "Comparing '" << innerItr->path << "' to '"
                    << outerItr->path << "' "
                    << "with strict comparison disabled";
            }
        }
    }

    static std::vector<JsonFile> jsonFiles;
};

std::vector<TestAdapterComparison::JsonFile> TestAdapterComparison::jsonFiles;

TEST_F(TestAdapterComparison, JsonCppVsJsonCpp)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::JsonCppAdapter>();
}

TEST_F(TestAdapterComparison, JsonCppVsPropertyTree)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::PropertyTreeAdapter>();
}

TEST_F(TestAdapterComparison, JsonCppVsRapidJson)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, JsonCppVsPicoJson)
{
    testComparison<
        valijson::adapters::JsonCppAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

TEST_F(TestAdapterComparison, PropertyTreeVsPropertyTree)
{
    testComparison<
        valijson::adapters::PropertyTreeAdapter,
        valijson::adapters::PropertyTreeAdapter>();
}

TEST_F(TestAdapterComparison, PropertyTreeVsRapidJson)
{
    testComparison<
        valijson::adapters::PropertyTreeAdapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, PropertyTreeVsPicoJson)
{
    testComparison<
        valijson::adapters::PropertyTreeAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

TEST_F(TestAdapterComparison, RapidJsonVsRapidJson)
{
    testComparison<
        valijson::adapters::RapidJsonAdapter,
        valijson::adapters::RapidJsonAdapter>();
}

TEST_F(TestAdapterComparison, RapidJsonVsPicoJson)
{
    testComparison<
        valijson::adapters::RapidJsonAdapter,
        valijson::adapters::PicoJsonAdapter>();
}

TEST_F(TestAdapterComparison, PicoJsonVsPicoJson)
{
    testComparison<
        valijson::adapters::PicoJsonAdapter,
        valijson::adapters::PicoJsonAdapter>();
}
