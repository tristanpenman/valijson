#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <valijson/adapters/yaml_cpp_adapter.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

class TestYamlCppAdapter : public testing::Test
{
};

TEST_F(TestYamlCppAdapter, BasicArrayIteration)
{
    const unsigned int numElements = 10;

    // Create a Json document that consists of an array of numbers
    YAML::Node document;

    for (unsigned int i = 0; i < numElements; i++) {
        document.push_back(static_cast<double>(i));
    }

    // Ensure that wrapping the document preserves the array and does not allow
    // it to be cast to other types
    valijson::adapters::YamlCppAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the array contains the expected number of elements
    EXPECT_EQ(numElements, adapter.getArray().size());

    // Ensure that the elements are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::YamlCppAdapter value : adapter.getArray()) {
        ASSERT_TRUE(value.isNumber());
        ASSERT_FALSE(value.isString());
        ASSERT_TRUE(value.maybeDouble());
        EXPECT_EQ(double(expectedValue), value.getNumber());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestYamlCppAdapter, BasicObjectIteration)
{
    const unsigned int numElements = 10;

    // Create a document that consists of an object that maps
    // numeric strings their corresponding numeric values
    YAML::Node document;
    for (uint32_t i = 0; i < numElements; i++) {
        document[std::to_string(i)] = static_cast<double>(i);
    }

    // Ensure that wrapping the document preserves the object and does not
    // allow it to be cast to other types
    valijson::adapters::YamlCppAdapter adapter(document);
#if VALIJSON_USE_EXCEPTIONS
    ASSERT_NO_THROW(adapter.getObject());
    ASSERT_ANY_THROW(adapter.getArray());
    ASSERT_ANY_THROW(adapter.getBool());
    ASSERT_ANY_THROW(adapter.getDouble());
    ASSERT_ANY_THROW(adapter.getString());
#endif

    // Ensure that the object contains the expected number of members
    EXPECT_EQ(numElements, adapter.getObject().size());

    // Ensure that the members are returned in the order they were inserted
    unsigned int expectedValue = 0;
    for (const valijson::adapters::YamlCppAdapter::ObjectMember member :
         adapter.getObject()) {
        ASSERT_TRUE(member.second.isNumber());
        ASSERT_FALSE(member.second.isString());
        ASSERT_TRUE(member.second.maybeDouble());
        EXPECT_EQ(std::to_string(expectedValue), member.first);
        EXPECT_EQ(double(expectedValue), member.second.getNumber());
        expectedValue++;
    }

    // Ensure that the correct number of elements were iterated over
    EXPECT_EQ(numElements, expectedValue);
}

TEST_F(TestYamlCppAdapter, BasicObjectMemberAccess)
{
    const unsigned int numElements = 10;

    // Create a document that consists of an object that maps
    // numeric strings their corresponding numeric values
    YAML::Node document;
    for (uint32_t i = 0; i < numElements; i++) {
        document[std::to_string(i)] = static_cast<double>(i);
    }
    valijson::adapters::YamlCppAdapter adapter(document);
    const auto adapterObject = adapter.asObject();

    // Ensure that accessing an element that exists produces the expected result.
    const auto result3 = adapterObject.find("3");
    EXPECT_NE(result3, adapterObject.end());
    EXPECT_EQ(result3->second.asDouble(), 3);

    // Ensure that accessing an element that does not exists.
    const auto result12 = adapterObject.find("12");
    EXPECT_EQ(result12, adapterObject.end());
}

namespace {

valijson::adapters::YamlCppAdapter load(const std::string &yaml, YAML::Node &document)
{
    document = YAML::Load(yaml);
    return valijson::adapters::YamlCppAdapter(document);
}

} // namespace

TEST_F(TestYamlCppAdapter, PlainScalarTypes)
{
    YAML::Node document;

    // Booleans, as defined by the YAML 1.2 core schema
    for (const char *value : {"true", "True", "TRUE"}) {
        const auto adapter = load(value, document);
        EXPECT_TRUE(adapter.isBool()) << value;
        EXPECT_FALSE(adapter.isString()) << value;
        EXPECT_TRUE(adapter.getBool()) << value;
    }
    for (const char *value : {"false", "False", "FALSE"}) {
        const auto adapter = load(value, document);
        EXPECT_TRUE(adapter.isBool()) << value;
        EXPECT_FALSE(adapter.getBool()) << value;
    }

    // YAML 1.1 booleans are strings in YAML 1.2
    for (const char *value : {"yes", "no", "on", "off", "y", "n", "tRUE"}) {
        const auto adapter = load(value, document);
        EXPECT_FALSE(adapter.isBool()) << value;
        EXPECT_TRUE(adapter.isString()) << value;
    }

    // Integers
    const std::pair<const char *, int64_t> integers[] = {
        {"0", 0}, {"42", 42}, {"-42", -42}, {"+42", 42}, {"007", 7},
        {"0o17", 15}, {"0x1f", 31}, {"0xFF", 255},
        {"9223372036854775807", INT64_MAX}, {"-9223372036854775808", INT64_MIN},
    };
    for (const auto &integer : integers) {
        const auto adapter = load(integer.first, document);
        EXPECT_TRUE(adapter.isInteger()) << integer.first;
        EXPECT_TRUE(adapter.isNumber()) << integer.first;
        EXPECT_FALSE(adapter.isDouble()) << integer.first;
        EXPECT_FALSE(adapter.isString()) << integer.first;
        EXPECT_EQ(integer.second, adapter.getInteger()) << integer.first;
    }

    // Integers that do not fit in an int64_t are numbers
    {
        const auto adapter = load("18446744073709551616", document);
        EXPECT_FALSE(adapter.isInteger());
        EXPECT_TRUE(adapter.isDouble());
        EXPECT_EQ(18446744073709551616.0, adapter.getDouble());
    }

    // Numbers
    const std::pair<const char *, double> doubles[] = {
        {"4.5", 4.5}, {"-4.5", -4.5}, {"+4.5", 4.5}, {"1.0", 1.0}, {".5", 0.5},
        {"1.", 1.0}, {"1e3", 1000.0}, {"1.5E-1", 0.15}, {"-2e+2", -200.0},
    };
    for (const auto &number : doubles) {
        const auto adapter = load(number.first, document);
        EXPECT_TRUE(adapter.isDouble()) << number.first;
        EXPECT_TRUE(adapter.isNumber()) << number.first;
        EXPECT_FALSE(adapter.isInteger()) << number.first;
        EXPECT_FALSE(adapter.isString()) << number.first;
        EXPECT_EQ(number.second, adapter.getDouble()) << number.first;
    }
    EXPECT_EQ(std::numeric_limits<double>::infinity(), load(".inf", document).getDouble());
    EXPECT_EQ(-std::numeric_limits<double>::infinity(), load("-.Inf", document).getDouble());
    EXPECT_TRUE(std::isnan(load(".NaN", document).getDouble()));

    // Anything else is a string
    for (const char *value : {"bonjour", "0x", "0o8", "0xg", "1e", "1.2.3", "--1", "inf", "nan",
                "1_000", "1,5", "0b101"}) {
        const auto adapter = load(value, document);
        EXPECT_TRUE(adapter.isString()) << value;
        EXPECT_FALSE(adapter.isNumber()) << value;
        EXPECT_FALSE(adapter.isBool()) << value;
        EXPECT_EQ(value, adapter.getString()) << value;
    }

    // Null
    for (const char *value : {"~", "null", "Null", "NULL", ""}) {
        const auto adapter = load(value, document);
        EXPECT_TRUE(adapter.isNull()) << value;
        EXPECT_FALSE(adapter.isString()) << value;
    }
}

TEST_F(TestYamlCppAdapter, QuotedAndBlockScalarsAreStrings)
{
    YAML::Node document;
    for (const char *value : {"\"42\"", "'42'", "\"true\"", "'null'", "\"4.5\""}) {
        const auto adapter = load(value, document);
        EXPECT_TRUE(adapter.isString()) << value;
        EXPECT_FALSE(adapter.isNumber()) << value;
        EXPECT_FALSE(adapter.isBool()) << value;
        EXPECT_FALSE(adapter.isNull()) << value;
    }

    const auto literal = load("|\n  42\n", document);
    EXPECT_TRUE(literal.isString());
    EXPECT_EQ("42\n", literal.getString());
}

TEST_F(TestYamlCppAdapter, TaggedScalars)
{
    YAML::Node document;

    EXPECT_TRUE(load("!!str 42", document).isString());
    EXPECT_EQ("42", load("!!str 42", document).getString());
    EXPECT_TRUE(load("!!str true", document).isString());

    EXPECT_TRUE(load("!!int '42'", document).isInteger());
    EXPECT_EQ(42, load("!!int '42'", document).getInteger());

    EXPECT_TRUE(load("!!float 1", document).isDouble());
    EXPECT_EQ(1.0, load("!!float 1", document).getDouble());
    EXPECT_EQ(4.5, load("!!float '4.5'", document).getDouble());
    EXPECT_EQ(18446744073709551616.0, load("!!float 18446744073709551616", document).getDouble());
    EXPECT_EQ(18446744073709551616.0, load("!!int 18446744073709551616", document).getDouble());

    EXPECT_TRUE(load("!!bool 'true'", document).isBool());
    EXPECT_TRUE(load("!!null ''", document).isNull());

    // A value that does not match its tag, or an application-specific tag,
    // is a string
    EXPECT_TRUE(load("!!int abc", document).isString());
    EXPECT_TRUE(load("!!bool yes", document).isString());
    EXPECT_TRUE(load("!!float abc", document).isString());
    EXPECT_TRUE(load("!!null abc", document).isString());
    EXPECT_TRUE(load("!custom 42", document).isString());
}

TEST_F(TestYamlCppAdapter, ScalarsCreatedInCode)
{
    // yaml-cpp stores scalars created in code as strings without a tag, so
    // they are resolved like plain scalars
    const YAML::Node integer(42);
    EXPECT_TRUE(valijson::adapters::YamlCppAdapter(integer).isInteger());

    const YAML::Node number(4.5);
    EXPECT_TRUE(valijson::adapters::YamlCppAdapter(number).isDouble());

    const YAML::Node boolean(true);
    EXPECT_TRUE(valijson::adapters::YamlCppAdapter(boolean).isBool());

    const YAML::Node string("bonjour");
    EXPECT_TRUE(valijson::adapters::YamlCppAdapter(string).isString());
}

TEST_F(TestYamlCppAdapter, NonScalars)
{
    YAML::Node document;
    for (const char *value : {"[1, 2]", "{a: 1}"}) {
        const auto adapter = load(value, document);
        EXPECT_FALSE(adapter.isString()) << value;
        EXPECT_FALSE(adapter.isNumber()) << value;
        EXPECT_FALSE(adapter.isBool()) << value;
        EXPECT_FALSE(adapter.isNull()) << value;
    }
}

TEST_F(TestYamlCppAdapter, ValidatesScalarTypes)
{
    // Schemas and documents from https://github.com/tristanpenman/valijson/issues/178
    const auto validate = [](const char *schemaYaml, const char *documentYaml) {
        const YAML::Node schemaNode = YAML::Load(schemaYaml);
        valijson::Schema schema;
        valijson::SchemaParser parser;
        parser.populateSchema(valijson::adapters::YamlCppAdapter(schemaNode), schema);

        const YAML::Node documentNode = YAML::Load(documentYaml);
        valijson::Validator validator;
        return validator.validate(schema, valijson::adapters::YamlCppAdapter(documentNode), nullptr);
    };

    EXPECT_TRUE(validate("type: string", "\"bonjour\""));
    EXPECT_TRUE(validate("type: array", "[]"));
    EXPECT_TRUE(validate("type: object", "{}"));
    EXPECT_TRUE(validate("type: boolean", "true"));
    EXPECT_TRUE(validate("type: integer", "42"));
    EXPECT_TRUE(validate("type: number", "49.3"));
    EXPECT_TRUE(validate("type: number", "42"));
    EXPECT_TRUE(validate("type: \"null\"", "null"));

    EXPECT_FALSE(validate("type: string", "42"));
    EXPECT_FALSE(validate("type: string", "true"));
    EXPECT_FALSE(validate("type: integer", "\"42\""));
    EXPECT_FALSE(validate("type: boolean", "\"true\""));
    EXPECT_FALSE(validate("type: boolean", "yes"));

    EXPECT_TRUE(validate("{type: integer, enum: [1, 2]}", "1"));
    EXPECT_FALSE(validate("{type: integer, enum: [1, 2]}", "3"));
    EXPECT_FALSE(validate("{enum: [1, 2]}", "\"1\""));
    EXPECT_TRUE(validate("{const: \"1\"}", "\"1\""));
    EXPECT_FALSE(validate("{const: \"1\"}", "1"));
    EXPECT_TRUE(validate("{minimum: 5}", "7"));
    EXPECT_FALSE(validate("{minimum: 5}", "3"));
}
