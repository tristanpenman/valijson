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

#pragma once
#ifndef __VALIJSON_CONSTRAINTS_CONCRETE_CONSTRAINTS_HPP
#define __VALIJSON_CONSTRAINTS_CONCRETE_CONSTRAINTS_HPP

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/variant.hpp>

#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <valijson/adapters/frozen_value.hpp>
#include <valijson/constraints/basic_constraint.hpp>
#include <valijson/internal/custom_allocator.hpp>
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
    typedef std::vector<const Subschema *> Schemas;

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
    typedef std::vector<const Subschema *> Schemas;

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
class DependenciesConstraint: public BasicConstraint<DependenciesConstraint>
{
public:
    DependenciesConstraint()
      : propertyDependencies(std::less<String>(), allocator),
        schemaDependencies(std::less<String>(), allocator)
    { }

    DependenciesConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        propertyDependencies(std::less<String>(), allocator),
        schemaDependencies(std::less<String>(), allocator)
    { }

    template<typename StringType>
    DependenciesConstraint & addPropertyDependency(
            const StringType &propertyName,
            const StringType &dependencyName)
    {
        const String key(propertyName.c_str(), allocator);
        PropertyDependencies::iterator itr = propertyDependencies.find(key);
        if (itr == propertyDependencies.end()) {
            itr = propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), allocator))).first;
        }

        itr->second.insert(String(dependencyName.c_str(), allocator));

        return *this;
    }

    template<typename StringType, typename ContainerType>
    DependenciesConstraint & addPropertyDependencies(
            const StringType &propertyName,
            const ContainerType &dependencyNames)
    {
        const String key(propertyName.c_str(), allocator);
        PropertyDependencies::iterator itr = propertyDependencies.find(key);
        if (itr == propertyDependencies.end()) {
            itr = propertyDependencies.insert(PropertyDependencies::value_type(
                    key, PropertySet(std::less<String>(), allocator))).first;
        }

        typedef typename ContainerType::value_type ValueType;
        BOOST_FOREACH( const ValueType &dependencyName, dependencyNames ) {
            itr->second.insert(String(dependencyName.c_str(), allocator));
        }

        return *this;
    }

    template<typename StringType>
    DependenciesConstraint & addSchemaDependency(
            const StringType &propertyName,
            const Subschema *schemaDependency)
    {
        if (schemaDependencies.insert(SchemaDependencies::value_type(
                String(propertyName.c_str(), allocator),
                schemaDependency)).second) {
            return *this;
        }

        throw std::runtime_error(
                "Dependencies constraint already contains a dependent "
                "schema for the property '" + propertyName + "'");
    }

    template<typename FunctorType>
    bool applyToPropertyDependencies(const FunctorType &fn) const
    {
        BOOST_FOREACH( const PropertyDependencies::value_type &v,
                propertyDependencies ) {
            if (!fn(v.first, v.second)) {
                return false;
            }
        }

        return true;
    }

    template<typename FunctorType>
    bool applyToSchemaDependencies(const FunctorType &fn) const
    {
        BOOST_FOREACH( const SchemaDependencies::value_type &v,
                schemaDependencies ) {
            if (!fn(v.first, v.second)) {
                return false;
            }
        }

        return true;
    }

private:
    typedef std::set<String, std::less<String>, Allocator> PropertySet;

    typedef std::map<String, PropertySet, std::less<String>, Allocator>
            PropertyDependencies;

    typedef std::map<String, const Subschema *, std::less<String>, Allocator>
            SchemaDependencies;

    /// Mapping from property names to their property-based dependencies
    PropertyDependencies propertyDependencies;

    /// Mapping from property names to their schema-based dependencies
    SchemaDependencies schemaDependencies;
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
    typedef std::vector<const Subschema *> Schemas;

    /**
     * @brief  Construct a singular item constraint that allows no additional
     *         items
     *
     * @param  itemSchema
     */
    ItemsConstraint(const Subschema *itemSchema)
      : itemSchema(itemSchema),
        additionalItemsSchema(NULL) { }

    /**
     * @brief  Construct a singular item schema that allows additional items
     *
     * @param  itemSchema
     * @param  additionalItemsSchema
     */
    ItemsConstraint(const Subschema *itemSchema,
                    const Subschema *additionalItemsSchema)
      : itemSchema(itemSchema),
        additionalItemsSchema(additionalItemsSchema) { }

    /**
     * @brief  Construct a plural items constraint that does not allow for
     *         additional item schemas
     *
     * @param  itemSchemas  collection of item schemas
     */
    ItemsConstraint(const Schemas &itemSchemas)
      : itemSchema(NULL),
        itemSchemas(itemSchemas),
        additionalItemsSchema(NULL) { }

    /**
     * @brief  Construct a plural items constraint that allows additional items
     *
     * @param  itemSchemas
     * @param  additionalItemsSchema
     */
    ItemsConstraint(const Schemas &itemSchemas,
                    const Subschema *additionalItemsSchema)
      : itemSchema(NULL),
        itemSchemas(itemSchemas),
        additionalItemsSchema(additionalItemsSchema) { }

    /**
     * @brief  Copy constructor
     */
    ItemsConstraint(const ItemsConstraint &other)
      : itemSchema(other.itemSchema),
        itemSchemas(other.itemSchemas),
        additionalItemsSchema(other.additionalItemsSchema) { }

    const Subschema* itemSchema;
    const Schemas itemSchemas;
    const Subschema* additionalItemsSchema;
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
 * @brief  Represents a 'multipleOf' or 'divisibleBy' constraint
 */
struct MultipleOfConstraint: BasicConstraint<MultipleOfConstraint>
{
    explicit MultipleOfConstraint(int64_t value)
      : value(value) { }

    explicit MultipleOfConstraint(double value)
      : value(value) { }

    const boost::variant<double, int64_t> value;
};

/**
 * @brief   Represents a 'not' constraint.
 */
struct NotConstraint: BasicConstraint<NotConstraint>
{
    NotConstraint(const Subschema *schema)
      : schema(schema) { }

    NotConstraint(const NotConstraint &other)
      : schema(other.schema) { }

    const Subschema *schema;
};

/**
 * @brief   Represents a 'oneOf' constraint.
 */
struct OneOfConstraint: BasicConstraint<OneOfConstraint>
{
    typedef std::vector<const Subschema *> Schemas;

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

    typedef std::map<std::string, const Subschema *> PropertySchemaMap;

    PropertiesConstraint(const PropertySchemaMap &properties)
      : properties(properties),
        additionalProperties(NULL) { }

    PropertiesConstraint(const PropertySchemaMap &properties,
                         const PropertySchemaMap &patternProperties)
      : properties(properties),
        patternProperties(patternProperties),
        additionalProperties(NULL) { }

    PropertiesConstraint(const PropertySchemaMap &properties,
                         const PropertySchemaMap &patternProperties,
                         const Subschema *additionalProperties)
      : properties(properties),
        patternProperties(patternProperties),
        additionalProperties(additionalProperties) { }

    PropertiesConstraint(const PropertiesConstraint &other)
      : properties(other.properties),
        patternProperties(other.patternProperties),
        additionalProperties(other.additionalProperties) {}

    const PropertySchemaMap properties;
    const PropertySchemaMap patternProperties;
    const Subschema *additionalProperties;

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

    typedef std::vector<const Subschema *> Schemas;

    TypeConstraint(const JsonType jsonType)
      : jsonTypes(makeJsonTypes(jsonType)) { }

    TypeConstraint(const JsonTypes &jsonTypes)
      : jsonTypes(jsonTypes) { }

    TypeConstraint(const JsonTypes &jsonTypes,
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
