#ifndef __VALIJSON_SCHEMA_PARSER_HPP
#define __VALIJSON_SCHEMA_PARSER_HPP

#include <stdexcept>
#include <iostream>

#include <boost/foreach.hpp>

#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/schema.hpp>

namespace valijson {

/**
 * @brief  Parser for populating a Schema based on a JSON Schema document.
 *
 * The SchemaParser class supports Drafts 3 and 4 of JSON Schema, however
 * Draft 3 support should be considered deprecated.
 *
 * The functions provided by this class have been templated so that they can
 * be used with different Adapter types.
 */
class SchemaParser
{
public:

    /// Supported versions of JSON Schema
    enum Version {
        kDraft3,      /// @deprecated
        kDraft4
    };

    /// Version of JSON Schema that should be expected when parsing
    const Version version;

    /**
     * @brief  Construct a new SchemaParser for a given version of JSON Schema.
     *
     * @param  version  Version of JSON Schema that will be expected
     */
    SchemaParser(const Version version = kDraft4)
      : version(version) { }

    /**
     * @brief  Functor for dereferencing JSON References, templated to support
     *         multiple Adapter types.
     */
    template<typename AdapterType>
    struct DereferenceFunction: boost::function<const AdapterType & (const std::string &uri)> { };

    /**
     * @brief  Populate a Schema object from JSON Schema document.
     *
     * When processing Draft 3 schemas, the parentSchema and ownName pointers
     * should be set in contexts where a 'required' constraint would be valid.
     * These are used to add a RequiredConstraint object to the Schema that
     * contains the required property.
     *
     * @param  node          Reference to node to parse
     * @param  schema        Reference to Schema to populate
     * @param  deref         Function to use for dereferencing URIs (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3.
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3.
     */
    template<typename AdapterType>
    void populateSchema(
        const AdapterType &node,
        Schema &schema,
        boost::optional<DereferenceFunction<AdapterType> > deref = boost::none,
        Schema *parentSchema = NULL,
        const std::string *ownName = NULL)
    {
        if ((isReference(node))) {
            const AdapterType &childNode = resolveReference<AdapterType>(deref, schema, node);
            populateSchema<AdapterType>(childNode, schema, deref, parentSchema, ownName);
            return;
        }

        const typename AdapterType::Object object = node.asObject();
        typename AdapterType::Object::const_iterator itr(object.end());

        if ((itr = object.find("id")) != object.end()) {
            if (itr->second.maybeString()) {
                schema.setId(itr->second.asString());
            }
        }

        if ((itr = object.find("allOf")) != object.end()) {
            const AdapterType &childNode = resolveReference<AdapterType>(deref, schema, itr->second);
            schema.addConstraint(makeAllOfConstraint(childNode, deref));
        }

        if ((itr = object.find("anyOf")) != object.end()) {
            schema.addConstraint(makeAnyOfConstraint(itr->second, deref));
        }

        if ((itr = object.find("dependencies")) != object.end()) {
            schema.addConstraint(makeDependenciesConstraint(itr->second, deref));
        }

        if ((itr = object.find("enum")) != object.end()) {
            schema.addConstraint(makeEnumConstraint(itr->second));
        }

        {
            // Check for schema keywords that require the creation of a
            // ItemsConstraint instance.
            const typename AdapterType::Object::const_iterator
                itemsItr = object.find("items"),
                additionalitemsItr = object.find("additionalItems");
            if (object.end() != itemsItr ||
                object.end() != additionalitemsItr) {
                schema.addConstraint(makeItemsConstraint(
                    itemsItr != object.end() ? &itemsItr->second : NULL,
                    additionalitemsItr != object.end() ? &additionalitemsItr->second : NULL,
                    deref));
            }
        }

        if ((itr = object.find("maximum")) != object.end()) {
            typename AdapterType::Object::const_iterator exclusiveMaximumItr = object.find("exclusiveMaximum");
            if (exclusiveMaximumItr == object.end()) {
                schema.addConstraint(makeMaximumConstraint<AdapterType>(itr->second, NULL));
            } else {
                schema.addConstraint(makeMaximumConstraint(itr->second, &exclusiveMaximumItr->second));
            }
        } else if (object.find("exclusiveMaximum") != object.end()) {
            // throw exception
        }

        if ((itr = object.find("maxItems")) != object.end()) {
            schema.addConstraint(makeMaxItemsConstraint(itr->second));
        }

        if ((itr = object.find("maxLength")) != object.end()) {
            schema.addConstraint(makeMaxLengthConstraint(itr->second));
        }

        if ((itr = object.find("maxProperties")) != object.end()) {
            schema.addConstraint(makeMaxPropertiesConstraint(itr->second));
        }

        if ((itr = object.find("minimum")) != object.end()) {
            typename AdapterType::Object::const_iterator exclusiveMinimumItr = object.find("exclusiveMinimum");
            if (exclusiveMinimumItr == object.end()) {
                schema.addConstraint(makeMinimumConstraint<AdapterType>(itr->second, NULL));
            } else {
                schema.addConstraint(makeMinimumConstraint(itr->second, &exclusiveMinimumItr->second));
            }
        } else if (object.find("exclusiveMinimum") != object.end()) {
            // throw exception
        }

        if ((itr = object.find("minItems")) != object.end()) {
            schema.addConstraint(makeMinItemsConstraint(itr->second));
        }

        if ((itr = object.find("minLength")) != object.end()) {
            schema.addConstraint(makeMinLengthConstraint(itr->second));
        }

        if ((itr = object.find("minProperties")) != object.end()) {
            schema.addConstraint(makeMinPropertiesConstraint(itr->second));
        }

        if ((itr = object.find("not")) != object.end()) {
            schema.addConstraint(makeNotConstraint(itr->second, deref));
        }

        if ((itr = object.find("oneOf")) != object.end()) {
            schema.addConstraint(makeOneOfConstraint(itr->second, deref));
        }

        if ((itr = object.find("pattern")) != object.end()) {
            schema.addConstraint(makePatternConstraint(itr->second));
        }

        {
            // Check for schema keywords that require the creation of a
            // PropertiesConstraint instance.
            const typename AdapterType::Object::const_iterator
                propertiesItr = object.find("properties"),
                patternPropertiesItr = object.find("patternProperties"),
                additionalPropertiesItr = object.find("additionalProperties");
            if (object.end() != propertiesItr ||
                object.end() != patternPropertiesItr ||
                object.end() != additionalPropertiesItr) {
                schema.addConstraint(makePropertiesConstraint(
                    propertiesItr != object.end() ? &propertiesItr->second : NULL,
                    patternPropertiesItr != object.end() ? &patternPropertiesItr->second : NULL,
                    additionalPropertiesItr != object.end() ? &additionalPropertiesItr->second : NULL,
                    deref, &schema));
            }
        }

        if ((itr = object.find("required")) != object.end()) {
            if (version == kDraft3) {
                if (parentSchema && ownName) {
                    if (constraints::Constraint *c = makeRequiredConstraintForSelf(itr->second, *ownName)) {
                        parentSchema->addConstraint(c);
                    }
                } else {
                    throw std::runtime_error("'required' constraint not valid here");
                }
            } else {
                schema.addConstraint(makeRequiredConstraint(itr->second));
            }
        }

        if ((itr = object.find("title")) != object.end()) {

        }

        if ((itr = object.find("type")) != object.end()) {
            schema.addConstraint(makeTypeConstraint(itr->second, deref));
        }

        if ((itr = object.find("uniqueItems")) != object.end()) {
            constraints::Constraint *constraint = makeUniqueItemsConstraint(itr->second);
            if (constraint) {
                schema.addConstraint(constraint);
            }
        }
    }

private:

    /**
     * @brief   Return true if the adapter contains a JSON Reference.
     *
     * A JSON Reference is an object that has a property named '$ref'. All other
     * properties are ignored.
     *
     * @param   node  Adapter to examine
     *
     * @return  true if the adapter contains a JSON Reference, false otherwise
     */
    template<typename AdapterType>
    bool isReference(const AdapterType &node)
    {
        if (node.isObject()) {
            const typename AdapterType::Object object = node.getObject();
            return (object.find("$ref") != object.end());
        }

        return false;
    }

    /**
     * @brief   Resolve a JSON reference if present in the target node.
     *
     * This function allows JSON references to be used with minimal changes to
     * the parser helper functions. If the target node is an object with a $ref
     * property, the referenced JSON value will be retrieved using a callback
     * function provided by the user. Otherwise, a reference to the node itself
     * will be returned.
     *
     * @param   deref
     * @param   schema
     * @param   node
     */
    template<typename AdapterType>
    const AdapterType & resolveReference(
        boost::optional<DereferenceFunction<AdapterType> > deref,
        const Schema &schema,
        const AdapterType &node)
    {
        typedef typename AdapterType::Object Object;

        if (node.isObject()) {
            const Object object = node.getObject();
            const typename Object::const_iterator itr = object.find("$ref");
            if (itr != object.end()) {
                if (itr->second.maybeString()) {
                    if (deref) {
                        return (*deref)(schema.resolveUri(itr->second.asString()));
                    } else {
                        throw std::runtime_error("Dereferencing of JSON References not enabled.");
                    }
                } else {
                    throw std::runtime_error("$ref property expected to contain string value.");
                }
            }
        }

        return node;
    }

    /**
     * @brief   Make a new AllOfConstraint object.
     *
     * @param   node   JSON node containing an array of child schemas.
     * @param   deref  Optional functor for resolving JSON References.
     *
     * @return  pointer to a new AllOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AllOfConstraint* makeAllOfConstraint(
        const AdapterType &node,
        boost::optional<DereferenceFunction<AdapterType> > deref)
    {
        if (!node.maybeArray()) {
            throw std::runtime_error("Expected array value for 'allOf' constraint.");
        }

        constraints::AllOfConstraint::Schemas childSchemas;
        BOOST_FOREACH ( const AdapterType schemaNode, node.asArray() ) {
            if (schemaNode.maybeObject()) {
                childSchemas.push_back(new Schema);
                populateSchema<AdapterType>(schemaNode, childSchemas.back(), deref);
            } else {
                throw std::runtime_error("Expected array element to be an object value in 'allOf' constraint.");
            }
        }

        /// @todo: bypass deep copy of the child schemas
        return new constraints::AllOfConstraint(childSchemas);
    }

    /**
     * @brief   Make a new AnyOfConstraint object.
     *
     * @param   node   JSON node containing an array of child schemas.
     * @param   deref  Optional functor for resolving JSON References.
     *
     * @return  pointer to a new AnyOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AnyOfConstraint* makeAnyOfConstraint(
        const AdapterType &node,
        boost::optional<DereferenceFunction<AdapterType> > deref)
    {
        if (!node.maybeArray()) {
            throw std::runtime_error("Expected array value for 'anyOf' constraint.");
        }

        constraints::AnyOfConstraint::Schemas childSchemas;
        BOOST_FOREACH ( const AdapterType schemaNode, node.asArray() ) {
            if (schemaNode.maybeObject()) {
                childSchemas.push_back(new Schema);
                populateSchema<AdapterType>(schemaNode, childSchemas.back(), deref);
            } else {
                throw std::runtime_error("Expected array element to be an object value in 'anyOf' constraint.");
            }
        }

        /// @todo: bypass deep copy of the child schemas
        return new constraints::AnyOfConstraint(childSchemas);
    }

    /**
     * @brief   Make a new DependenciesConstraint object.
     *
     * The dependencies for a property can be defined several ways. When parsing
     * a Draft 4 schema, the following can be used:
     *  - an array that lists the name of each property that must be present
     *    if the dependent property is present
     *  - an object that specifies a schema which must be satisfied if the
     *    dependent property is present
     *
     * When parsing a Draft 3 schema, in addition to the formats above, the
     * following format can be used:
     *  - a string that names a single property that must be present if the
     *    dependent property is presnet
     *
     * Multiple methods can be used in the same dependency constraint.
     *
     * If the format of any part of the the dependency node does not match one
     * of these formats, an exception will be thrown.
     *
     * @param   node   JSON node containing an object that defines a mapping of
     *                 properties to their dependencies.
     * @param   deref  Optional functor for resolving JSON References.
     *
     * @return  pointer to a new DependencyConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::DependenciesConstraint* makeDependenciesConstraint(
        const AdapterType &node,
        boost::optional<DereferenceFunction<AdapterType> > deref)
    {
        if (!node.maybeObject()) {
            throw std::runtime_error("Expected object value for 'dependencies' constraint.");
        }

        constraints::DependenciesConstraint::PropertyDependenciesMap pdm;
        constraints::DependenciesConstraint::PropertyDependentSchemasMap pdsm;

        // Process each of the dependency mappings defined by the object
        BOOST_FOREACH ( const typename AdapterType::ObjectMember member, node.asObject() ) {

            // First, we attempt to parse the value of the dependency mapping
            // as an array of strings. If the Adapter type does not support
            // strict types, then an empty string or empty object will be cast
            // to an array, and the resulting dependency list will be empty.
            // This is equivalent to using an empty object, but does mean that
            // if the user provides an actual string then this error will not
            // be detected.
            if (member.second.maybeArray()) {
                // Parse an array of dependency names
                constraints::DependenciesConstraint::Dependencies &dependencies = pdm[member.first];
                BOOST_FOREACH( const AdapterType dependencyName, member.second.asArray() ) {
                    if (dependencyName.maybeString()) {
                        dependencies.insert(dependencyName.getString());
                    } else {
                        throw std::runtime_error("Expected string value in dependency list of property '" +
                            member.first + "' in 'dependencies' constraint.");
                    }
                }

            // If the value of dependency mapping could not be processed as an
            // array, we'll try to process it as an object instead. Note that
            // strict type comparison is used here, since we've already
            // exercised the flexibility by loosely-typed Adapter types. If the
            // value of the dependency mapping is an object, then we'll try to
            // process it as a dependent schema.
            } else if (member.second.isObject()) {
                // Parse dependent subschema
                Schema &childSchema = pdsm[member.first];
                populateSchema<AdapterType>(member.second, childSchema, deref);

            // If we're supposed to be parsing a Draft3 schema, then the value
            // of the dependency mapping can also be a string containing the
            // name of a single dependency.
            } else if (version == kDraft3 && member.second.isString()) {
                pdm[member.first].insert(member.second.getString());

            // All other types result in an exception being thrown.
            } else {
                throw std::runtime_error("Invalid dependencies definition.");
            }
        }

        return new constraints::DependenciesConstraint(pdm, pdsm);
    }

    /**
     * @brief   Make a new EnumConstraint object.
     *
     * @param   node  JSON node containing an array of values permitted by the
     *                constraint.
     *
     * @return  pointer to a new EnumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::EnumConstraint* makeEnumConstraint(
        const AdapterType &node)
    {
        // Make a copy of each value in the enum array
        constraints::EnumConstraint::Values values;
        BOOST_FOREACH( const AdapterType value, node.getArray() ) {
            values.push_back(value.freeze());
        }

        /// @todo This will make another copy of the values while constructing
        /// the EnumConstraint. Move semantics in C++11 should make it possible
        /// to avoid these copies without complicating the implementation of the
        /// EnumConstraint class.
        return new constraints::EnumConstraint(values);
    }

    /**
     * @brief   Make a new ItemsConstraint object.
     *
     * @param   items            Optional pointer to a JSON node containing
     *                           an object mapping property names to schemas.
     * @param   additionalItems  Optional pointer to a JSON node containing
     *                           an additional properties schema or a boolean
     *                           value.
     * @param   deref            Optional functor for resolving JSON References.
     *
     * @return  pointer to a new ItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ItemsConstraint* makeItemsConstraint(
        const AdapterType *items,
        const AdapterType *additionalItems,
        boost::optional<DereferenceFunction<AdapterType> > deref)
    {
        // Construct a Schema object for the the additionalItems constraint,
        // if the additionalItems property is present
        boost::scoped_ptr<Schema> additionalItemsSchema;
        if (additionalItems) {
            if (additionalItems->maybeBool()) {
                // If the value of the additionalItems property is a boolean
                // and is set to true, then additional array items do not need
                // to satisfy any constraints.
                if (additionalItems->asBool()) {
                    additionalItemsSchema.reset(new Schema());
                }
            } else if (additionalItems->maybeObject()) {
                // If the value of the additionalItems property is an object,
                // then it should be parsed into a Schema object, which will be
                // used to validate additional array items.
                additionalItemsSchema.reset(new Schema());
                populateSchema<AdapterType>(*additionalItems, *additionalItemsSchema, deref);
            } else {
                // Any other format for the additionalItems property will result
                // in an exception being thrown.
                throw std::runtime_error("Expected bool or object value for 'additionalItems'");
            }
        } else {
            // The default value for the additionalItems property is an empty
            // object, which means that additional array items do not need to
            // satisfy any constraints.
            additionalItemsSchema.reset(new Schema());
        }

        // Construct a Schema object for each item in the items array, if an
        // array is provided, or a single Schema object, in an object value is
        // provided. If the items constraint is not provided, then array items
        // will be validated against the additionalItems schema.
        constraints::ItemsConstraint::Schemas itemSchemas;
        if (items) {
            if (items->isArray()) {
                // If the items constraint contains an array, then it should
                // contain a list of child schemas which will be used to
                // validate the values at the corresponding indexes in a target
                // array.
                BOOST_FOREACH( const AdapterType v, items->getArray() ) {
                    itemSchemas.push_back(new Schema());
                    Schema &childSchema = itemSchemas.back();
                    populateSchema<AdapterType>(v, childSchema, deref);
                }

                // Create an ItemsConstraint object using the appropriate
                // overloaded constructor.
                if (additionalItemsSchema) {
                    return new constraints::ItemsConstraint(itemSchemas, *additionalItemsSchema);
                } else {
                    return new constraints::ItemsConstraint(itemSchemas);
                }

            } else if (items->isObject()) {
                // If the items constraint contains an object value, then it
                // should contain a Schema that will be used to validate all
                // items in a target array. Any schema defined by the
                // additionalItems constraint will be ignored.
                Schema childSchema;
                populateSchema<AdapterType>(*items, childSchema, deref);
                if (additionalItemsSchema) {
                    return new constraints::ItemsConstraint(childSchema, *additionalItemsSchema);
                } else {
                    return new constraints::ItemsConstraint(childSchema);
                }

            } else if (items->maybeObject()) {
                // If a loosely-typed Adapter type is being used, then we'll
                // assume that an empty schema has been provided.
                Schema childSchema;
                if (additionalItemsSchema) {
                    return new constraints::ItemsConstraint(childSchema, *additionalItemsSchema);
                } else {
                    return new constraints::ItemsConstraint(childSchema);
                }

            } else {
                // All other formats will result in an exception being thrown.
                throw std::runtime_error("Expected array or object value for 'items'.");
            }
        }

        Schema emptySchema;
        if (additionalItemsSchema) {
            return new constraints::ItemsConstraint(emptySchema, *additionalItemsSchema);
        }

        return new constraints::ItemsConstraint(emptySchema);
    }

    /**
     * @brief   Make a new MaximumConstraint object.
     *
     * @param   node              JSON node containing the maximum value.
     * @param   exclusiveMaximum  Optional pointer to a JSON boolean value that
     *                            indicates whether maximum value is excluded
     *                            from the range of permitted values.
     *
     * @return  pointer to a new MaximumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaximumConstraint* makeMaximumConstraint(
        const AdapterType &node,
        const AdapterType *exclusiveMaximum)
    {
        bool exclusiveMaximumValue = false;
        if (exclusiveMaximum) {
            if (exclusiveMaximum->maybeBool()) {
                exclusiveMaximumValue = exclusiveMaximum->asBool();
            } else {
                throw std::runtime_error("Expected boolean value for exclusiveMaximum constraint.");
            }
        }

        if (node.maybeDouble()) {
            double maximumValue = node.asDouble();
            return new constraints::MaximumConstraint(maximumValue, exclusiveMaximumValue);
        }

        throw std::runtime_error("Expected numeric value for maximum constraint.");
    }

    /**
     * @brief   Make a new MaxItemsConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum number of items that may be contaned by an array.
     *
     * @return  pointer to a new MaxItemsConstraint that belongs to the caller.
     */
    template<typename AdapterType>
    constraints::MaxItemsConstraint* makeMaxItemsConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                return new constraints::MaxItemsConstraint(value);
            }
        }

        throw std::runtime_error("Expected positive integer value for maxItems constraint.");
    }

    /**
     * @brief   Make a new MaxLengthConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum length of a string.
     *
     * @return  pointer to a new MaxLengthConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MaxLengthConstraint* makeMaxLengthConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                return new constraints::MaxLengthConstraint(value);
            }
        }

        throw std::runtime_error("Expected a positive integer value for maxLength constraint.");
    }

    /**
     * @brief   Make a new MaxPropertiesConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                maximum number of properties that may be contained by an
     *                object.
     *
     * @return  pointer to a new MaxPropertiesConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::MaxPropertiesConstraint* makeMaxPropertiesConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                return new constraints::MaxPropertiesConstraint(value);
            }
        }

        throw std::runtime_error("Expected a positive integer for 'maxProperties' constraint.");
    }

    /**
     * @brief  Make a new MinimumConstraint object.
     *
     * @param  node              JSON node containing an integer, representing
     *                           the minimum value.
     *
     * @param  exclusiveMaximum  Optional pointer to a JSON boolean value that
     *                           indicates whether the minimum value is
     *                           excluded from the range of permitted values.
     *
     * @return  pointer to a new MinimumConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinimumConstraint* makeMinimumConstraint(
        const AdapterType &node,
        const AdapterType *exclusiveMinimum)
    {
        bool exclusiveMinimumValue = false;
        if (exclusiveMinimum) {
            if (exclusiveMinimum->maybeBool()) {
                exclusiveMinimumValue = exclusiveMinimum->asBool();
            } else {
                throw std::runtime_error("Expected boolean value for 'exclusiveMinimum' constraint.");
            }
        }

        if (node.maybeDouble()) {
            double minimumValue = node.asDouble();
            return new constraints::MinimumConstraint(minimumValue, exclusiveMinimumValue);
        }

        throw std::runtime_error("Expected numeric value for 'minimum' constraint.");
    }

    /**
     * @brief  Make a new MinItemsConstraint object.
     *
     * @param  node  JSON node containing an integer value representing the
     *               minimum number of items that may be contained by an array.
     *
     * @return  pointer to a new MinItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinItemsConstraint* makeMinItemsConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                return new constraints::MinItemsConstraint(value);
            }
        }

        throw std::runtime_error("Expected a positive integer value for 'minItems' constraint.");
    }

    /**
     * @brief  Make a new MinLengthConstraint object.
     *
     * @param  node  JSON node containing an integer value representing the
     *               minimum length of a string.
     *
     * @return  pointer to a new MinLengthConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::MinLengthConstraint* makeMinLengthConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                return new constraints::MinLengthConstraint(value);
            }
        }

        throw std::runtime_error("Expected a positive integer value for 'minLength' constraint.");
    }


    /**
     * @brief   Make a new MaxPropertiesConstraint object.
     *
     * @param   node  JSON node containing an integer value representing the
     *                minimum number of properties that may be contained by an
     *                object.
     *
     * @return  pointer to a new MinPropertiesConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::MinPropertiesConstraint* makeMinPropertiesConstraint(
        const AdapterType &node)
    {
        if (node.maybeInteger()) {
            int64_t value = node.asInteger();
            if (value >= 0) {
                return new constraints::MinPropertiesConstraint(value);
            }
        }

        throw std::runtime_error("Expected a positive integer for 'minProperties' constraint.");
    }

    /**
     * @brief   Make a new NotConstraint object.
     *
     * @param   node  JSON node containing a schema.
     *
     * @return  pointer to a new NotConstraint object that belongs to the caller
     */
    template<typename AdapterType>
    constraints::NotConstraint* makeNotConstraint(
        const AdapterType &node,
        boost::optional<DereferenceFunction<AdapterType> > deref)
    {
        if (node.maybeObject()) {
            Schema childSchema;
            populateSchema<AdapterType>(node, childSchema, deref);
            return new constraints::NotConstraint(childSchema);
        }

        throw std::runtime_error("Expected object value for 'not' constraint.");
    }

    /**
     * @brief   Make a new OneOfConstraint object.
     *
     * @param   node   JSON node containing an array of child schemas.
     * @param   deref  Optional functor for resolving JSON References.
     *
     * @return  pointer to a new OneOfConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::OneOfConstraint* makeOneOfConstraint(
        const AdapterType &node,
        boost::optional<DereferenceFunction<AdapterType> > deref)
    {
        constraints::OneOfConstraint::Schemas childSchemas;
        BOOST_FOREACH ( const AdapterType schemaNode, node.getArray() ) {
            childSchemas.push_back(new Schema);
            populateSchema<AdapterType>(
                schemaNode, childSchemas.back(),
                deref);
        }

        /// @todo: bypass deep copy of the child schemas
        return new constraints::OneOfConstraint(childSchemas);
    }

    /**
     * @brief   Make a new PatternConstraint object.
     *
     * @param   node   JSON node containing a pattern string
     * @param   deref  Optional functor for resolving JSON References.
     *
     * @return  pointer to a new PatternConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::PatternConstraint* makePatternConstraint(
        const AdapterType &node)
    {
        return new constraints::PatternConstraint(node.getString());
    }

    /**
     * @brief   Make a new Properties object.
     *
     * @param   properties            Optional pointer to a JSON node containing
     *                                an object mapping property names to
     *                                schemas.
     * @param   patternProperties     Optional pointer to a JSON node containing
     *                                an object mapping pattern property names
     *                                to schemas.
     * @param   additionalProperties  Optional pointer to a JSON node containing
     *                                an additional properties schema or a
     *                                boolean value.
     * @param   deref                 Optional functor for resolving JSON
     *                                References.
     * @param   parentSchema          Optional pointer to the Schema of the
     *                                parent object, to support the 'required'
     *                                keyword in Draft 3 of JSON Schema.
     *
     * @return  pointer to a new Properties that belongs to the caller
     */
    template<typename AdapterType>
    constraints::PropertiesConstraint* makePropertiesConstraint(
        const AdapterType *properties,
        const AdapterType *patternProperties,
        const AdapterType *additionalProperties,
        boost::optional<DereferenceFunction<AdapterType> > deref,
        Schema *parentSchema)
    {
        typedef typename AdapterType::ObjectMember Member;
        typedef constraints::PropertiesConstraint::PropertySchemaMap PSM;

        // Populate a PropertySchemaMap for each of the properties defined by
        // the 'properties' keyword.
        PSM propertySchemas;
        if (properties) {
            BOOST_FOREACH( const Member m, properties->getObject() ) {
                const std::string &propertyName = m.first;
                Schema &childSchema = propertySchemas[propertyName];
                populateSchema<AdapterType>(
                    m.second, childSchema,    // Required
                    deref,
                    parentSchema, &propertyName);               // Optional
            }
        }

        // Populate a PropertySchemaMap for each of the properties defined by
        // the 'patternProperties' keyword
        PSM patternPropertySchemas;
        if (patternProperties) {
            BOOST_FOREACH( const Member m, patternProperties->getObject() ) {
                const std::string &propertyName = m.first;
                Schema &childSchema = patternPropertySchemas[propertyName];
                populateSchema<AdapterType>(
                    m.second, childSchema,    // Required
                    deref,
                    parentSchema, &propertyName);               // Optional
            }
        }

        // Populate an additionalItems schema if required
        boost::scoped_ptr<Schema> additionalPropertiesSchema;
        if (additionalProperties) {
            // If additionalProperties has been set, check for a boolean value.
            // Setting 'additionalProperties' to true allows the values of
            // additional properties to take any form. Setting it false
            // prohibits the use of additional properties.
            // If additionalProperties is instead an object, it should be
            // parsed as a schema. If additionalProperties has any other type,
            // then the schema is not valid.
            if (additionalProperties->isBool() ||
                additionalProperties->maybeBool()) {
                // If it has a boolean value that is 'true', then an empty
                // schema should be used.
                if (additionalProperties->getBool()) {
                    additionalPropertiesSchema.reset(new Schema());
                }
            } else if (additionalProperties->isObject()) {
                // If additionalProperties is an object, it should be used as
                // a child schema.
                additionalPropertiesSchema.reset(new Schema());
                populateSchema<AdapterType>(*additionalProperties,
                    *additionalPropertiesSchema, deref);
            } else {
                // All other types are invalid
                throw std::runtime_error("Invalid type for 'additionalProperties' constraint.");
            }
        } else {
            // If an additionalProperties constraint is not provided, then the
            // default value is an empty schema.
            additionalPropertiesSchema.reset(new Schema());
        }

        if (additionalPropertiesSchema) {
            // If an additionalProperties schema has been created, construct a
            // new PropertiesConstraint object using that schema.
            return new constraints::PropertiesConstraint(
                propertySchemas, patternPropertySchemas,
                *additionalPropertiesSchema);
        }

        return new constraints::PropertiesConstraint(
                propertySchemas, patternPropertySchemas);
    }

    /**
     * @brief   Make a new RequiredConstraint.
     *
     * This function is used to create new RequiredContraint objects for
     * Draft 3 schemas.
     *
     * @param   node  Node containing a boolean value.
     * @param   name  Name of the required attribute.
     *
     * @return  pointer to a new RequiredConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::RequiredConstraint* makeRequiredConstraintForSelf(
        const AdapterType &node,
        const std::string &name)
    {
        if (!node.maybeBool()) {
            throw std::runtime_error("Expected boolean value for 'required' attribute.");
        }

        if (node.getBool()) {
            constraints::RequiredConstraint::RequiredProperties requiredProperties;
            requiredProperties.insert(name);
            return new constraints::RequiredConstraint(requiredProperties);
        }

        return NULL;
    }

    /**
     * @brief   Make a new RequiredConstraint.
     *
     * This function is used to create new RequiredContraint objects for
     * Draft 4 schemas.
     *
     * @param   node  Node containing an array of strings.
     *
     * @return  pointer to a new RequiredConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::RequiredConstraint* makeRequiredConstraint(
        const AdapterType &node)
    {
        constraints::RequiredConstraint::RequiredProperties requiredProperties;
        BOOST_FOREACH( const AdapterType v, node.getArray() ) {
            if (!v.isString()) {
                // @todo throw exception
            }
            requiredProperties.insert(v.getString());
        }

        return new constraints::RequiredConstraint(requiredProperties);
    }

    /**
     * @brief   Make a new TypeConstraint object.
     *
     * @param   node   Node containing the name of a JSON type.
     * @param   deref  Optional functor for resolving JSON References.
     *
     * @return  pointer to a new TypeConstraint object.
     */
    template<typename AdapterType>
    constraints::TypeConstraint* makeTypeConstraint(
        const AdapterType &node,
        boost::optional<DereferenceFunction<AdapterType> > deref)
    {
        typedef constraints::TypeConstraint TC;

        TC::JsonTypes jsonTypes;
        TC::Schemas schemas;

        if (node.isString()) {
            const TC::JsonType jsonType = TC::jsonTypeFromString(node.getString());
            if (jsonType == TC::kAny && version == kDraft4) {
                throw std::runtime_error("'any' type is not supported in version 4 schemas.");
            }
            jsonTypes.insert(jsonType);

        } else if (node.isArray()) {
            BOOST_FOREACH( const AdapterType v, node.getArray() ) {
                if (v.isString()) {
                    const TC::JsonType jsonType = TC::jsonTypeFromString(v.getString());
                    if (jsonType == TC::kAny && version == kDraft4) {
                        throw std::runtime_error("'any' type is not supported in version 4 schemas.");
                    }
                    jsonTypes.insert(jsonType);
                } else if (v.isObject() && version == kDraft3) {
                    // Schema!
                    schemas.push_back(new Schema());
                    populateSchema<AdapterType>(v, schemas.back(),
                        deref);
                } else {
                    throw std::runtime_error("Type name should be a string.");
                }
            }
        } else if (node.isObject() && version == kDraft3) {
            schemas.push_back(new Schema());
            populateSchema<AdapterType>(node, schemas.back(), deref);
        } else {
            throw std::runtime_error("Type name should be a string.");
        }

        return new constraints::TypeConstraint(jsonTypes, schemas);
    }

    /**
     * @brief   Make a new UniqueItemsConstraint object.
     *
     * @param   node  Node containing a boolean value.
     *
     * @return  pointer to a new UniqueItemsConstraint object that belongs to
     *          the caller, or NULL if the boolean value is false.
     */
    template<typename AdapterType>
    constraints::UniqueItemsConstraint* makeUniqueItemsConstraint(
        const AdapterType &node)
    {
        if (node.isBool() || node.maybeBool()) {
            // If the boolean value is true, this function will return a pointer
            // to a new UniqueItemsConstraint object. If it is value, then the
            // constraint is redundant, so NULL is returned instead.
            if (node.getBool()) {
                return new constraints::UniqueItemsConstraint();
            } else {
                return NULL;
            }
        }

        throw std::runtime_error("Expected boolean value for 'uniqueItems' constraint.");
    }

};

}  // namespace valijson

#endif
