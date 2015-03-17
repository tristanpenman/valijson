/**
 * @file
 *
 * @brief   Class definitions to support JSON Schema constraints
 *
 * This file contains class definitions for all of the constraints required to
 * support JSON Schema. These classes all inherit from the BasicConstraint
 * template class, which implements the common parts of the Constraint
 * interface.
 *
 * @see BasicConstraint
 * @see Constraint
 */
#ifndef __VALIJSON_CONSTRAINTS_CONCRETE_CONSTRAINTS_HPP
#define __VALIJSON_CONSTRAINTS_CONCRETE_CONSTRAINTS_HPP

#include <limits>
#include <set>
#include <string>

#include <boost/ptr_container/ptr_map.hpp>
#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include <valijson/adapters/frozen_value.hpp>
#include <valijson/constraints/basic_constraint.hpp>
#include <valijson/schema.hpp>

namespace valijson {

class Schema;

namespace constraints {

/**
 * @brief  Represents an 'allOf' constraint.
 *
 * An allOf constraint provides a collection of sub-schemas that a value must
 * validate against. If a value fails to validate against any of these sub-
 * schemas, then validation fails.
 */
struct AllOfConstraint: BasicConstraint<AllOfConstraint>
{
    typedef boost::ptr_vector<Schema> Schemas;

    AllOfConstraint(const Schemas &schemas)
      : schemas(schemas) { }

    /// Collection of schemas that must all be satisfied
    const Schemas schemas;
};

/**
 * @brief  Represents an 'anyOf' constraint
 *
 * An anyOf constraint provides a collection of sub-schemas that a value can
 * validate against. If a value validates against one of these sub-schemas,
 * then the validation passes.
 */
struct AnyOfConstraint: BasicConstraint<AnyOfConstraint>
{
    typedef boost::ptr_vector<Schema> Schemas;

    AnyOfConstraint(const Schemas &schemas)
      : schemas(schemas) { }

    /// Collection of schemas of which one must be satisfied
    const Schemas schemas;
};

/**
 * @brief  Represents a 'dependencies' constraint.
 *
 * A dependency constraint ensures that a given property is valid only if the
 * properties that it depends on are present.
 */
struct DependenciesConstraint: BasicConstraint<DependenciesConstraint>
{
    // A mapping from property names to the set of names of their dependencies
    typedef std::set<std::string> Dependencies;
    typedef std::map<std::string, Dependencies> PropertyDependenciesMap;

    // A mapping from property names to dependent schemas
    typedef boost::ptr_map<std::string, Schema> PropertyDependentSchemasMap;

    DependenciesConstraint(const PropertyDependenciesMap &dependencies,
                           const PropertyDependentSchemasMap &dependentSchemas)
      : dependencies(dependencies),
        dependentSchemas(dependentSchemas) { }

    const PropertyDependenciesMap dependencies;
    const PropertyDependentSchemasMap dependentSchemas;
};

/**
 * @brief  Represents an 'enum' constraint.
 *
 * An enum constraint provides a set of permissible values for a JSON node. The
 * node will only validate against this constraint if it matches one of the
 * values in the set.
 */
struct EnumConstraint: BasicConstraint<EnumConstraint>
{
    typedef boost::ptr_vector<adapters::FrozenValue> Values;

    EnumConstraint(const Values &values)   // Copy each of the frozen values
      : values(values) { }

    const Values values;
};

/**
 * @brief  Represents a pair of 'items' and 'additionalItems' constraints.
 */
struct ItemsConstraint: BasicConstraint<ItemsConstraint>
{
    typedef boost::ptr_vector<Schema> Schemas;

    /**
     * @brief  Construct a singular item constraint that allows no additional
     *         items
     *
     * @param  itemSchema
     */
    ItemsConstraint(const Schema &itemSchema)
      : itemSchema(new Schema(itemSchema)) { }

    /**
     * @brief  Construct a singular item schema that allows additional items
     *
     * @param  itemSchema
     * @param  additionalItemsSchema
     */
    ItemsConstraint(const Schema &itemSchema,
                    const Schema &additionalItemsSchema)
      : itemSchema(new Schema(itemSchema)),
        additionalItemsSchema(new Schema(additionalItemsSchema)) { }

    /**
     * @brief  Construct a plural items constraint that does not allow for
     *         additional item schemas
     *
     * @param  itemSchemas  collection of item schemas
     */
    ItemsConstraint(const Schemas &itemSchemas)
      : itemSchemas(new Schemas(itemSchemas)) { }

    /**
     * @brief  Construct a plural items constraint that allows additional items
     *
     * @param  itemSchemas
     * @param  additionalItemsSchema
     */
    ItemsConstraint(const Schemas &itemSchemas,
                    const Schema &additionalItemsSchema)
      : itemSchemas(new Schemas(itemSchemas)),
        additionalItemsSchema(new Schema(additionalItemsSchema)) { }

    /**
     * @brief  Copy constructor
     */
    ItemsConstraint(const ItemsConstraint &other)
      : itemSchema(other.itemSchema ? new Schema(*other.itemSchema.get()) : NULL),
        itemSchemas(other.itemSchemas ? new Schemas(*other.itemSchemas.get()) : NULL),
        additionalItemsSchema(other.additionalItemsSchema ? new Schema(*other.additionalItemsSchema.get()) : NULL) { }

    const boost::scoped_ptr<const Schema> itemSchema;
    const boost::scoped_ptr<const Schemas> itemSchemas;
    const boost::scoped_ptr<const Schema> additionalItemsSchema;
};

/**
 * @brief   Represents a 'maximum' constraint.
 */
struct MaximumConstraint: BasicConstraint<MaximumConstraint>
{
    MaximumConstraint(double maximum, bool exclusiveMaximum)
      : maximum(maximum),
        exclusiveMaximum(exclusiveMaximum) { }

    const double maximum;
    const bool exclusiveMaximum;
};

/**
 * @brief   Represents a 'maxItems' constraint.
 */
struct MaxItemsConstraint: BasicConstraint<MaxItemsConstraint>
{
    MaxItemsConstraint(int64_t maxItems)
      : maxItems(maxItems) { }

    const int64_t maxItems;
};

/**
 * @brief   Represents a 'maxLength' constraint.
 */
struct MaxLengthConstraint: BasicConstraint<MaxLengthConstraint>
{
    MaxLengthConstraint(int64_t maxLength)
      : maxLength(maxLength) { }

    const int64_t maxLength;
};

/**
 * @brief   Represents a 'maxProperties' constraint.
 */
struct MaxPropertiesConstraint: BasicConstraint<MaxPropertiesConstraint>
{
    MaxPropertiesConstraint(int64_t maxProperties)
      : maxProperties(maxProperties) { }

    const int64_t maxProperties;
};

/**
 * @brief   Represents a pair of 'minimum' and 'exclusiveMinimum' constraints.
 */
struct MinimumConstraint: BasicConstraint<MinimumConstraint>
{
    MinimumConstraint(double minimum, bool exclusiveMinimum)
      : minimum(minimum),
        exclusiveMinimum(exclusiveMinimum) { }

    const double minimum;
    const bool exclusiveMinimum;
};

/**
 * @brief   Represents a 'minItems' constraint.
 */
struct MinItemsConstraint: BasicConstraint<MinItemsConstraint>
{
    MinItemsConstraint(int64_t minItems)
      : minItems(minItems) { }

    const int64_t minItems;
};

/**
 * @brief   Represents a 'minLength' constraint.
 */
struct MinLengthConstraint: BasicConstraint<MinLengthConstraint>
{
    MinLengthConstraint(int64_t minLength)
      : minLength(minLength) { }

    const int64_t minLength;
};

/**
 * @brief   Represents a 'minProperties' constraint.
 */
struct MinPropertiesConstraint: BasicConstraint<MinPropertiesConstraint>
{
    MinPropertiesConstraint(int64_t minProperties)
      : minProperties(minProperties) { }

    const int64_t minProperties;
};

/**
 * @brief  Represents a 'multipleOf' or 'divisibleBy' constraint for decimals
 */
struct MultipleOfDecimalConstraint: BasicConstraint<MultipleOfDecimalConstraint>
{
    MultipleOfDecimalConstraint(double multipleOf)
      : multipleOf(multipleOf) { }

    const double multipleOf;
};

/**
 * @brief  Represents a 'multipleOf' or 'divisibleBy' constraint for int64_t
 */
struct MultipleOfIntegerConstraint: BasicConstraint<MultipleOfIntegerConstraint>
{
    MultipleOfIntegerConstraint(int64_t multipleOf)
      : multipleOf(multipleOf) { }

    const int64_t multipleOf;
};

/**
 * @brief   Represents a 'not' constraint.
 */
struct NotConstraint: BasicConstraint<NotConstraint>
{
    NotConstraint(const Schema &schema)
      : schema(new Schema(schema)) { }

    NotConstraint(const NotConstraint &other)
      : schema(other.schema ? new Schema(*other.schema) : NULL) { }

    const boost::scoped_ptr<const Schema> schema;
};

/**
 * @brief   Represents a 'oneOf' constraint.
 */
struct OneOfConstraint: BasicConstraint<OneOfConstraint>
{
    typedef boost::ptr_vector<Schema> Schemas;

    OneOfConstraint(const Schemas &schemas)
      : schemas(schemas) { }

    /// Collection of schemas that must all be satisfied
    const Schemas schemas;
};

/**
 * @brief   Represents a 'pattern' constraint.
 */
struct PatternConstraint: BasicConstraint<PatternConstraint>
{
    PatternConstraint(const std::string &pattern)
      : pattern(pattern) { }

    const std::string pattern;
};

/**
 * @brief   Represents a tuple of 'properties', 'patternProperties' and
 *          'additionalProperties' constraints.
 */
struct PropertiesConstraint: BasicConstraint<PropertiesConstraint> {

    typedef boost::ptr_map<std::string, Schema> PropertySchemaMap;

    PropertiesConstraint(const PropertySchemaMap &properties,
                         const PropertySchemaMap &patternProperties)
      : properties(properties),
        patternProperties(patternProperties) { }

    PropertiesConstraint(const PropertySchemaMap &properties,
                         const PropertySchemaMap &patternProperties,
                         const Schema &additionalProperties)
      : properties(properties),
        patternProperties(patternProperties),
        additionalProperties(new Schema(additionalProperties)) { }

    PropertiesConstraint(const PropertiesConstraint &other)
      : properties(other.properties),
        patternProperties(other.patternProperties),
        additionalProperties(other.additionalProperties ?
                 new Schema(*other.additionalProperties.get()) : NULL) {}

    const PropertySchemaMap properties;
    const PropertySchemaMap patternProperties;
    const boost::scoped_ptr<const Schema> additionalProperties;

};

/**
 * @brief   Represents a 'required' constraint.
 */
struct RequiredConstraint: BasicConstraint<RequiredConstraint>
{
    typedef std::set<std::string> RequiredProperties;

    RequiredConstraint(const RequiredProperties &requiredProperties)
      : requiredProperties(requiredProperties)
    {

    }

    const RequiredProperties requiredProperties;
};

/**
 * @brief   Represents a 'type' constraint.
 */
struct TypeConstraint: BasicConstraint<TypeConstraint>
{
    enum JsonType {
        kAny,
        kArray,
        kBoolean,
        kInteger,
        kNull,
        kNumber,
        kObject,
        kString
    };

    typedef std::set<JsonType> JsonTypes;

    typedef boost::ptr_vector<Schema> Schemas;

    TypeConstraint(const JsonType jsonType)
      : jsonTypes(makeJsonTypes(jsonType)) { }

    TypeConstraint(const JsonTypes jsonTypes)
      : jsonTypes(jsonTypes) { }

    TypeConstraint(const JsonTypes jsonTypes,
                   const Schemas &schemas)
      : jsonTypes(jsonTypes),
        schemas(schemas) { }

    static JsonTypes makeJsonTypes(const JsonType jsonType)
    {
        JsonTypes jsonTypes;
        jsonTypes.insert(jsonType);
        return jsonTypes;
    }

    static JsonType jsonTypeFromString(const std::string &typeName)
    {
        if (typeName.compare("any") == 0) {
            return kAny;
        } else if (typeName.compare("array") == 0) {
            return kArray;
        } else if (typeName.compare("boolean") == 0) {
            return kBoolean;
        } else if (typeName.compare("integer") == 0) {
            return kInteger;
        } else if (typeName.compare("null") == 0) {
            return kNull;
        } else if (typeName.compare("number") == 0) {
            return kNumber;
        } else if (typeName.compare("object") == 0) {
            return kObject;
        } else if (typeName.compare("string") == 0) {
            return kString;
        }

        throw std::runtime_error("Unrecognised JSON type name '" + typeName + "'");
    }

    const JsonTypes jsonTypes;
    const Schemas schemas;
};

/**
 * @brief   Represents a 'uniqueItems' constraint.
 */
struct UniqueItemsConstraint: BasicConstraint<UniqueItemsConstraint>
{
    // Don't need anything here.
};

} // namespace constraints
} // namespace valijson

#endif
