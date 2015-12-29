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
namespace constraints {

/**
 * @brief  Represents an 'allOf' constraint.
 *
 * An allOf constraint provides a collection of sub-schemas that a value must
 * validate against. If a value fails to validate against any of these sub-
 * schemas, then validation fails.
 */
class AllOfConstraint: public BasicConstraint<AllOfConstraint>
{
public:
    AllOfConstraint()
      : subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    AllOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        BOOST_FOREACH( const Subschema *subschema, subschemas ) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *,
            internal::CustomAllocator<const Subschema *> > Subschemas;

    /// Collection of sub-schemas, all of which must be satisfied
    Subschemas subschemas;
};

/**
 * @brief  Represents an 'anyOf' constraint
 *
 * An anyOf constraint provides a collection of sub-schemas that a value can
 * validate against. If a value validates against one of these sub-schemas,
 * then the validation passes.
 */
class AnyOfConstraint: public BasicConstraint<AnyOfConstraint>
{
public:
    AnyOfConstraint()
      : subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    AnyOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        BOOST_FOREACH( const Subschema *subschema, subschemas ) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *,
            internal::CustomAllocator<const Subschema *> > Subschemas;

    /// Collection of sub-schemas, at least one of which must be satisfied
    Subschemas subschemas;
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
    void applyToPropertyDependencies(const FunctorType &fn) const
    {
        BOOST_FOREACH( const PropertyDependencies::value_type &v,
                propertyDependencies ) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaDependencies(const FunctorType &fn) const
    {
        BOOST_FOREACH( const SchemaDependencies::value_type &v,
                schemaDependencies ) {
            if (!fn(v.first, v.second)) {
                return;
            }
        }
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
 * @brief  Represents non-singular 'items' and 'additionalItems' constraints
 *
 * Unlike the SingularItemsConstraint class, this class represents an 'items'
 * constraint that specifies an array of sub-schemas, which should be used to
 * validate each item in an array, in sequence. It also represents an optional
 * 'additionalItems' sub-schema that should be used when an array contains
 * more values than there are sub-schemas in the 'items' constraint.
 *
 * The prefix 'Linear' comes from the fact that this class contains a list of
 * sub-schemas that corresponding array items must be validated against, and
 * this validation is performed linearly (i.e. in sequence).
 */
class LinearItemsConstraint: public BasicConstraint<LinearItemsConstraint>
{
public:
    LinearItemsConstraint()
      : itemSubschemas(Allocator::rebind<const Subschema *>::other(allocator)),
        additionalItemsSubschema(NULL) { }

    LinearItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        itemSubschemas(Allocator::rebind<const Subschema *>::other(allocator)),
        additionalItemsSubschema(NULL) { }

    void addItemSubschema(const Subschema *subschema)
    {
        itemSubschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToItemSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        BOOST_FOREACH( const Subschema *subschema, itemSubschemas ) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

    const Subschema * getAdditionalItemsSubschema() const
    {
        return additionalItemsSubschema;
    }

    size_t getItemSubschemaCount() const
    {
        return itemSubschemas.size();
    }

    void setAdditionalItemsSubschema(const Subschema *subschema)
    {
        additionalItemsSubschema = subschema;
    }

private:
    typedef std::vector<const Subschema *,
            internal::CustomAllocator<const Subschema *> > Subschemas;

    Subschemas itemSubschemas;

    const Subschema* additionalItemsSubschema;
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
class OneOfConstraint: public BasicConstraint<OneOfConstraint>
{
public:
    OneOfConstraint()
      : subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    OneOfConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        subschemas(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addSubschema(const Subschema *subschema)
    {
        subschemas.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToSubschemas(const FunctorType &fn) const
    {
        unsigned int index = 0;
        BOOST_FOREACH( const Subschema *subschema, subschemas ) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

private:
    typedef std::vector<const Subschema *,
            internal::CustomAllocator<const Subschema *> > Subschemas;

    /// Collection of sub-schemas, exactly one of which must be satisfied
    Subschemas subschemas;
};

/**
 * @brief   Represents a 'pattern' constraint
 */
class PatternConstraint: public BasicConstraint<PatternConstraint>
{
public:
    PatternConstraint()
      : pattern(Allocator::rebind<char>::other(allocator)) { }

    PatternConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        pattern(Allocator::rebind<char>::other(allocator)) { }

    template<typename AllocatorType>
    bool getPattern(std::basic_string<char, std::char_traits<char>,
            AllocatorType> &result) const
    {
        result.assign(this->pattern.c_str());
        return true;
    }

    template<typename AllocatorType>
    std::basic_string<char, std::char_traits<char>, AllocatorType> getPattern(
            const AllocatorType &alloc = AllocatorType()) const
    {
        return std::basic_string<char, std::char_traits<char>, AllocatorType>(
                pattern.c_str(), alloc);
    }

    template<typename AllocatorType>
    void setPattern(const std::basic_string<char, std::char_traits<char>,
            AllocatorType> &pattern)
    {
        this->pattern.assign(pattern.c_str());
    }

private:
    String pattern;
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
class RequiredConstraint: public BasicConstraint<RequiredConstraint>
{
public:
    RequiredConstraint()
      : requiredProperties(std::less<String>(), allocator) { }

    RequiredConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        requiredProperties(std::less<String>(), allocator) { }

    bool addRequiredProperty(const char *propertyName)
    {
        return requiredProperties.insert(String(propertyName,
                Allocator::rebind<char>::other(allocator))).second;
    }

    template<typename AllocatorType>
    bool addRequiredProperty(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &propertyName)
    {
        return addRequiredProperty(propertyName.c_str());
    }

    template<typename FunctorType>
    void applyToRequiredProperties(const FunctorType &fn) const
    {
        BOOST_FOREACH( const String &propertyName, requiredProperties ) {
            if (!fn(propertyName)) {
                return;
            }
        }
    }

private:
    typedef std::set<String, std::less<String>,
            internal::CustomAllocator<String> > RequiredProperties;

    RequiredProperties requiredProperties;
};

/**
 * @brief  Represents an 'items' constraint that specifies one sub-schema
 *
 * A value is considered valid against this constraint if it is an array, and
 * each item in the array validates against the sub-schema specified by this
 * constraint.
 *
 * The prefix 'Singular' comes from the fact that array items must validate
 * against exactly one sub-schema.
 */
class SingularItemsConstraint: public BasicConstraint<SingularItemsConstraint>
{
public:
    SingularItemsConstraint()
      : itemsSubschema(NULL) { }

    SingularItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        itemsSubschema(NULL) { }

    const Subschema * getItemsSubschema() const
    {
        return itemsSubschema;
    }

    void setItemsSubschema(const Subschema *subschema)
    {
        itemsSubschema = subschema;
    }

private:
    const Subschema *itemsSubschema;
};

/**
 * @brief   Represents a 'type' constraint.
 */
class TypeConstraint: public BasicConstraint<TypeConstraint>
{
public:
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

    TypeConstraint()
      : namedTypes(std::less<JsonType>(), allocator),
        schemaTypes(Allocator::rebind<const Subschema *>::other(allocator)) { }

    TypeConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn),
        namedTypes(std::less<JsonType>(), allocator),
        schemaTypes(Allocator::rebind<const Subschema *>::other(allocator)) { }

    void addNamedType(JsonType type)
    {
        namedTypes.insert(type);
    }

    void addSchemaType(const Subschema *subschema)
    {
        schemaTypes.push_back(subschema);
    }

    template<typename FunctorType>
    void applyToNamedTypes(const FunctorType &fn) const
    {
        BOOST_FOREACH( const JsonType namedType, namedTypes ) {
            if (!fn(namedType)) {
                return;
            }
        }
    }

    template<typename FunctorType>
    void applyToSchemaTypes(const FunctorType &fn) const
    {
        unsigned int index = 0;
        BOOST_FOREACH( const Subschema *subschema, schemaTypes ) {
            if (!fn(index, subschema)) {
                return;
            }

            index++;
        }
    }

    template<typename AllocatorType>
    static JsonType jsonTypeFromString(const std::basic_string<char,
            std::char_traits<char>, AllocatorType> &typeName)
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

        throw std::runtime_error("Unrecognised JSON type name '" +
                std::string(typeName.c_str()) + "'");
    }

private:
    typedef std::set<JsonType, std::less<JsonType>, Allocator> NamedTypes;

    typedef std::vector<const Subschema *,
            Allocator::rebind<const Subschema *>::other> SchemaTypes;

    /// Set of named JSON types that serve as valid types
    NamedTypes namedTypes;

    /// Set of sub-schemas that serve as valid types
    SchemaTypes schemaTypes;
};

/**
 * @brief   Represents a 'uniqueItems' constraint
 */
class UniqueItemsConstraint: public BasicConstraint<UniqueItemsConstraint>
{
public:
    UniqueItemsConstraint() { }

    UniqueItemsConstraint(CustomAlloc allocFn, CustomFree freeFn)
      : BasicConstraint(allocFn, freeFn) { }
};

} // namespace constraints
} // namespace valijson

#endif
