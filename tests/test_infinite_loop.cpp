#include <string>

#include <gtest/gtest.h>

#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/internal/infinite_loop.hpp>
#include <valijson/schema.hpp>

using valijson::Schema;
using valijson::Subschema;
using valijson::internal::containsInfiniteLoop;
using namespace valijson::constraints;

namespace {

AllOfConstraint allOf(const Subschema *subschema)
{
    AllOfConstraint constraint;
    constraint.addSubschema(subschema);
    return constraint;
}

PropertiesConstraint properties(const Subschema *subschema)
{
    PropertiesConstraint constraint;
    constraint.addPropertySubschema("foo", subschema);
    return constraint;
}

} // namespace

TEST(TestInfiniteLoop, DetectsSchemaAppliedToItself)
{
    // {"allOf": [{"$ref": "#"}]}
    Schema schema;
    schema.addConstraint(allOf(&schema));
    EXPECT_TRUE(containsInfiniteLoop(schema));
}

TEST(TestInfiniteLoop, DetectsCyclesThroughEachConstraint)
{
    {
        Schema schema;
        AnyOfConstraint constraint;
        constraint.addSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "anyOf";
    }
    {
        Schema schema;
        OneOfConstraint constraint;
        constraint.addSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "oneOf";
    }
    {
        Schema schema;
        NotConstraint constraint;
        constraint.setSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "not";
    }
    {
        Schema schema;
        ConditionalConstraint constraint;
        constraint.setIfSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "if";
    }
    {
        Schema schema;
        ConditionalConstraint constraint;
        constraint.setIfSubschema(schema.createSubschema());
        constraint.setThenSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "then";
    }
    {
        Schema schema;
        ConditionalConstraint constraint;
        constraint.setIfSubschema(schema.createSubschema());
        constraint.setElseSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "else";
    }
    {
        Schema schema;
        DependenciesConstraint constraint;
        constraint.addSchemaDependency(std::string("foo"), &schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "dependencies";
    }
    {
        Schema schema;
        TypeConstraint constraint;
        constraint.addSchemaType(&schema);
        schema.addConstraint(constraint);
        EXPECT_TRUE(containsInfiniteLoop(schema)) << "type (draft 3)";
    }
}

TEST(TestInfiniteLoop, DetectsCyclesBetweenSubschemas)
{
    // {"allOf": [{"$ref": "#/definitions/a"}],
    //  "definitions": {"a": {"allOf": [{"$ref": "#/definitions/b"}]},
    //                  "b": {"allOf": [{"$ref": "#/definitions/a"}]}}}
    Schema schema;
    const Subschema *a = schema.createSubschema();
    const Subschema *b = schema.createSubschema();
    schema.addConstraint(allOf(a));
    schema.addConstraintToSubschema(allOf(b), a);
    schema.addConstraintToSubschema(allOf(a), b);
    EXPECT_TRUE(containsInfiniteLoop(schema));
}

TEST(TestInfiniteLoop, DetectsCyclesBeneathOtherValues)
{
    // {"properties": {"foo": {"$ref": "#/definitions/a"}},
    //  "definitions": {"a": {"not": {"$ref": "#/definitions/a"}}}}
    Schema schema;
    const Subschema *a = schema.createSubschema();
    NotConstraint constraint;
    constraint.setSubschema(a);
    schema.addConstraintToSubschema(constraint, a);
    schema.addConstraint(properties(a));
    EXPECT_TRUE(containsInfiniteLoop(schema));
}

TEST(TestInfiniteLoop, AllowsRecursionIntoOtherValues)
{
    {
        // {"properties": {"foo": {"$ref": "#"}}}
        Schema schema;
        schema.addConstraint(properties(&schema));
        EXPECT_FALSE(containsInfiniteLoop(schema)) << "properties";
    }
    {
        // {"additionalProperties": {"$ref": "#"}}
        Schema schema;
        PropertiesConstraint constraint;
        constraint.setAdditionalPropertiesSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_FALSE(containsInfiniteLoop(schema)) << "additionalProperties";
    }
    {
        // {"items": {"$ref": "#"}}
        Schema schema;
        SingularItemsConstraint constraint;
        constraint.setItemsSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_FALSE(containsInfiniteLoop(schema)) << "items";
    }
    {
        // {"items": [{"$ref": "#"}], "additionalItems": {"$ref": "#"}}
        Schema schema;
        LinearItemsConstraint constraint;
        constraint.addItemSubschema(&schema);
        constraint.setAdditionalItemsSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_FALSE(containsInfiniteLoop(schema)) << "items (array)";
    }
    {
        // {"contains": {"$ref": "#"}}
        Schema schema;
        ContainsConstraint constraint;
        constraint.setSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_FALSE(containsInfiniteLoop(schema)) << "contains";
    }
    {
        // {"propertyNames": {"$ref": "#"}}
        Schema schema;
        PropertyNamesConstraint constraint;
        constraint.setSubschema(&schema);
        schema.addConstraint(constraint);
        EXPECT_FALSE(containsInfiniteLoop(schema)) << "propertyNames";
    }
}

TEST(TestInfiniteLoop, AllowsSubschemaAppliedMoreThanOnce)
{
    // A subschema that is applied to the same value through more than one
    // path, without a cycle:
    // {"allOf": [{"$ref": "#/definitions/a"}, {"$ref": "#/definitions/b"}],
    //  "definitions": {"a": {"allOf": [{"$ref": "#/definitions/c"}]},
    //                  "b": {"allOf": [{"$ref": "#/definitions/c"}]},
    //                  "c": {}}}
    Schema schema;
    const Subschema *a = schema.createSubschema();
    const Subschema *b = schema.createSubschema();
    const Subschema *c = schema.createSubschema();
    AllOfConstraint constraint;
    constraint.addSubschema(a);
    constraint.addSubschema(b);
    schema.addConstraint(constraint);
    schema.addConstraintToSubschema(allOf(c), a);
    schema.addConstraintToSubschema(allOf(c), b);
    EXPECT_FALSE(containsInfiniteLoop(schema));
}
