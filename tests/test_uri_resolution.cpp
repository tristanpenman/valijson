#include <gtest/gtest.h>

#include <valijson/schema.hpp>

using valijson::Schema;

class TestUriResolution : public ::testing::Test
{

};

TEST_F(TestUriResolution, TestDefaultScopeAndUri)
{
    Schema schema;
    EXPECT_FALSE( schema.hasId() );
    EXPECT_ANY_THROW( schema.getId() );
    EXPECT_EQ( "", schema.getUri() );
    EXPECT_EQ( "", schema.getScope() );
}