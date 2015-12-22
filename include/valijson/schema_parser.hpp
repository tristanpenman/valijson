#ifndef __VALIJSON_SCHEMA_PARSER_HPP
#define __VALIJSON_SCHEMA_PARSER_HPP

#include <stdexcept>
#include <iostream>

#include <boost/foreach.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits.hpp>

#include <valijson/adapters/adapter.hpp>
#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/internal/json_pointer.hpp>
#include <valijson/internal/json_reference.hpp>
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
        kDraft3,      ///< @deprecated JSON Schema v3 has been superseded by v4
        kDraft4
    };

    /// Version of JSON Schema that should be expected when parsing
    const Version version;

    /**
     * @brief  Construct a new SchemaParser for a given version of JSON Schema
     *
     * @param  version  Version of JSON Schema that will be expected
     */
    SchemaParser(const Version version = kDraft4)
      : version(version) { }

    /**
     * @brief  Struct to contain templated function type for fetching documents
     */
    template<typename AdapterType>
    struct FetchDocumentFunction {
        /// Functor for dereferencing JSON References
        typedef boost::function<boost::shared_ptr<const AdapterType>(
                const std::string &uri)> Type;
    };

    /**
     * @brief  Populate a Schema object from JSON Schema document
     *
     * When processing Draft 3 schemas, the parentSchema and ownName pointers
     * should be set in contexts where a 'required' constraint would be valid.
     * These are used to add a RequiredConstraint object to the Schema that
     * contains the required property.
     *
     * @param  node          Reference to node to parse
     * @param  schema        Reference to Schema to populate
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     */
    template<typename AdapterType>
    void populateSchema(
        const AdapterType &node,
        Schema &schema,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type>
                fetchDoc = boost::none)
    {
        populateSchema(node, node, schema, boost::none, "", fetchDoc, NULL, NULL);
    }

private:

    /**
     * @brief  Populate a Schema object from JSON Schema document
     *
     * When processing Draft 3 schemas, the parentSchema and ownName pointers
     * should be set in contexts where a 'required' constraint would be valid.
     * These are used to add a RequiredConstraint object to the Schema that
     * contains the required property.
     *
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document
     * @param  node          Reference to node to parse
     * @param  schema        Reference to Schema to populate
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3.
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3.
     */
    template<typename AdapterType>
    void populateSchema(
        const AdapterType &rootNode,
        const AdapterType &node,
        Schema &schema,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type>
                fetchDoc = boost::none,
        Schema *parentSchema = NULL,
        const std::string *ownName = NULL)
    {
        BOOST_STATIC_ASSERT_MSG((boost::is_convertible<AdapterType,
            const valijson::adapters::Adapter &>::value),
            "SchemaParser::populateSchema must be invoked with an "
            "appropriate Adapter implementation");

        // Check for JSON References and process them accordingly
        if (node.isObject()) {
            typename AdapterType::Object object = node.getObject();
            const typename AdapterType::Object::const_iterator itr =
                    object.find("$ref");
            if (itr != object.end()) {
                if (!itr->second.maybeString()) {
                    throw std::runtime_error(
                            "$ref property expected to contain string value.");
                }
                const std::string &jsonRef = itr->second.asString();
                populateSchemaUsingJsonReference(jsonRef, rootNode, node,
                        schema, currentScope, nodePath, fetchDoc,
                        parentSchema, ownName);
                return;
            }
        }

        const typename AdapterType::Object object = node.asObject();
        typename AdapterType::Object::const_iterator itr(object.end());

        if ((itr = object.find("id")) != object.end()) {
            if (itr->second.maybeString()) {
                schema.setId(itr->second.asString());
            }
        }

        if ((itr = object.find("allOf")) != object.end()) {
            schema.addConstraint(makeAllOfConstraint(rootNode, itr->second,
                    currentScope, nodePath + "/allOf", fetchDoc));
        }

        if ((itr = object.find("anyOf")) != object.end()) {
            schema.addConstraint(makeAnyOfConstraint(rootNode, itr->second,
                    currentScope, nodePath + "/anyOf", fetchDoc));
        }

        if ((itr = object.find("dependencies")) != object.end()) {
            schema.addConstraint(makeDependenciesConstraint(rootNode,
                    itr->second, currentScope, nodePath + "/dependencies",
                    fetchDoc));
        }

        if ((itr = object.find("description")) != object.end()) {
            if (itr->second.maybeString()) {
                schema.setDescription(itr->second.asString());
            } else {
                throw std::runtime_error(
                        "'description' attribute should have a string value");
            }
        }

        if ((itr = object.find("divisibleBy")) != object.end()) {
            if (version == kDraft3) {
                schema.addConstraint(makeMultipleOfConstraint(itr->second));
            } else {
                throw std::runtime_error("'divisibleBy' constraint not valid after draft 3");
            }
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
                schema.addConstraint(makeItemsConstraint(rootNode,
                    itemsItr != object.end() ? &itemsItr->second : NULL,
                    additionalitemsItr != object.end() ? &additionalitemsItr->second : NULL,
                    currentScope, nodePath + "/items",
                    nodePath + "/additionalItems", fetchDoc));
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

        if ((itr = object.find("multipleOf")) != object.end()) {
            if (version == kDraft3) {
                throw std::runtime_error("'multipleOf' constraint not available in draft 3");
            } else {
                schema.addConstraint(makeMultipleOfConstraint(itr->second));
            }
        }

        if ((itr = object.find("not")) != object.end()) {
            schema.addConstraint(makeNotConstraint(rootNode, itr->second,
                    currentScope, nodePath + "/not", fetchDoc));
        }

        if ((itr = object.find("oneOf")) != object.end()) {
            schema.addConstraint(makeOneOfConstraint(rootNode, itr->second,
                    currentScope, nodePath + "/oneOf", fetchDoc));
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
                schema.addConstraint(makePropertiesConstraint(rootNode,
                    propertiesItr != object.end() ? &propertiesItr->second : NULL,
                    patternPropertiesItr != object.end() ? &patternPropertiesItr->second : NULL,
                    additionalPropertiesItr != object.end() ? &additionalPropertiesItr->second : NULL,
                    currentScope, nodePath + "/properties",
                    nodePath + "/patternProperties",
                    nodePath + "/additionalProperties", fetchDoc, &schema));
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
            if (itr->second.maybeString()) {
                schema.setTitle(itr->second.asString());
            } else {
                throw std::runtime_error(
                        "'title' attribute should have a string value");
            }
        }

        if ((itr = object.find("type")) != object.end()) {
            schema.addConstraint(makeTypeConstraint(rootNode, itr->second,
                    currentScope, nodePath + "/type", fetchDoc));
        }

        if ((itr = object.find("uniqueItems")) != object.end()) {
            constraints::Constraint *constraint = makeUniqueItemsConstraint(itr->second);
            if (constraint) {
                schema.addConstraint(constraint);
            }
        }
    }

    /**
     * @brief  Populate a schema using a JSON Reference
     *
     * Allows JSON references to be used with minimal changes to the parser
     * helper functions.
     *
     * @param  jsonRef       String containing JSON Reference value
     * @param  rootNode      Reference to the node from which JSON References
     *                       will be resolved when they refer to the current
     *                       document; used for recursive parsing of schemas
     * @param  node          Reference to node to parse
     * @param  schema        Reference to Schema to populate
     * @param  currentScope  URI for current resolution scope
     * @param  nodePath      JSON Pointer representing path to current node
     * @param  fetchDoc      Function to fetch remote JSON documents (optional)
     * @param  parentSchema  Optional pointer to the parent schema, used to
     *                       support required keyword in Draft 3.
     * @param  ownName       Optional pointer to a node name, used to support
     *                       the 'required' keyword in Draft 3.
    */
    template<typename AdapterType>
    void populateSchemaUsingJsonReference(
        const std::string &jsonRef,
        const AdapterType &rootNode,
        const AdapterType &node,
        Schema &schema,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type>
                fetchDoc,
        Schema *parentSchema = NULL,
        const std::string *ownName = NULL)
    {
        // Returns a document URI if the reference points somewhere
        // other than the current document
        const boost::optional<std::string> documentUri =
                internal::json_reference::getJsonReferenceUri(jsonRef);

        // Extract JSON Pointer from JSON Reference
        const std::string jsonPointer =
                internal::json_reference::getJsonReferencePointer(jsonRef);

        if (documentUri) {
            // Resolve reference against remote document
            if (!fetchDoc) {
                throw std::runtime_error(
                        "Support for JSON References not enabled.");
            }

            // Returns a shared pointer to the remote document that was
            // retrieved, or null if retrieval failed. The resulting document
            // must remain in scope until populateSchema returns.
            boost::shared_ptr<const AdapterType> docPtr =
                    (*fetchDoc)(*documentUri);

            // Can't proceed without the remote document
            if (!docPtr) {
                throw std::runtime_error(
                        "Failed to fetch referenced schema document.");
            }

            const AdapterType &ref = internal::json_pointer::resolveJsonPointer(
                    *docPtr, jsonPointer);

            // Resolve reference against retrieved document
            populateSchema<AdapterType>(ref, ref, schema, currentScope,
                    nodePath, fetchDoc, parentSchema, ownName);

        } else {
            const AdapterType &ref = internal::json_pointer::resolveJsonPointer(
                    rootNode, jsonPointer);

            // Resolve reference against current document
            populateSchema<AdapterType>(rootNode, ref, schema, currentScope,
                    nodePath, fetchDoc, parentSchema, ownName);
        }
    }

    /**
     * @brief   Make a new AllOfConstraint object
     *
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     *
     * @return  pointer to a new AllOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AllOfConstraint* makeAllOfConstraint(
        const AdapterType &rootNode,
        const AdapterType &node,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type > fetchDoc)
    {
        if (!node.maybeArray()) {
            throw std::runtime_error("Expected array value for 'allOf' constraint.");
        }

        constraints::AllOfConstraint::Schemas childSchemas;
        int index = 0;
        BOOST_FOREACH ( const AdapterType schemaNode, node.asArray() ) {
            if (schemaNode.maybeObject()) {
                const std::string childPath = nodePath + "/" +
                        boost::lexical_cast<std::string>(index);
                childSchemas.push_back(new Schema);
                populateSchema<AdapterType>(rootNode, schemaNode,
                        childSchemas.back(), currentScope, childPath, fetchDoc);
                index++;
            } else {
                throw std::runtime_error("Expected array element to be an object value in 'allOf' constraint.");
            }
        }

        /// @todo: bypass deep copy of the child schemas
        return new constraints::AllOfConstraint(childSchemas);
    }

    /**
     * @brief   Make a new AnyOfConstraint object
     *
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     *
     * @return  pointer to a new AnyOfConstraint object that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::AnyOfConstraint* makeAnyOfConstraint(
        const AdapterType &rootNode,
        const AdapterType &node,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type > fetchDoc)
    {
        if (!node.maybeArray()) {
            throw std::runtime_error("Expected array value for 'anyOf' constraint.");
        }

        constraints::AnyOfConstraint::Schemas childSchemas;
        int index = 0;
        BOOST_FOREACH ( const AdapterType schemaNode, node.asArray() ) {
            if (schemaNode.maybeObject()) {
                const std::string childPath = nodePath + "/" +
                        boost::lexical_cast<std::string>(index);
                childSchemas.push_back(new Schema);
                populateSchema<AdapterType>(rootNode, schemaNode,
                        childSchemas.back(), currentScope, childPath, fetchDoc);
                index++;
            } else {
                throw std::runtime_error("Expected array element to be an object value in 'anyOf' constraint.");
            }
        }

        /// @todo: bypass deep copy of the child schemas
        return new constraints::AnyOfConstraint(childSchemas);
    }

    /**
     * @brief   Make a new DependenciesConstraint object
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
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an object that defines a
     *                        mapping of properties to their dependencies.
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     *
     * @return  pointer to a new DependencyConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::DependenciesConstraint* makeDependenciesConstraint(
        const AdapterType &rootNode,
        const AdapterType &node,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type >
                fetchDoc)
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
                populateSchema<AdapterType>(rootNode, member.second,
                        childSchema, currentScope, nodePath, fetchDoc);

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
     * @param   rootNode             Reference to the node from which JSON
     *                               References will be resolved when they refer
     *                               to the current document; used for recursive
     *                               parsing of schemas
     * @param   items                Optional pointer to a JSON node containing
     *                               an object mapping property names to 
     *                               schemas.
     * @param   additionalItems      Optional pointer to a JSON node containing
     *                               an additional properties schema or a
     *                               boolean value.
     * @param   currentScope         URI for current resolution scope
     * @param   itemsPath            JSON Pointer representing the path to
     *                               the 'items' node
     * @param   additionalItemsPath  JSON Pointer representing the path to
     *                               the 'additionalItems' node
     * @param   fetchDoc             Function to fetch remote JSON documents
     *                               (optional)
     *
     * @return  pointer to a new ItemsConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::ItemsConstraint* makeItemsConstraint(
        const AdapterType &rootNode,
        const AdapterType *items,
        const AdapterType *additionalItems,
        boost::optional<std::string> currentScope,
        const std::string &itemsPath,
        const std::string &additionalItemsPath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type >
                fetchDoc)
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
                populateSchema<AdapterType>(
                        rootNode, *additionalItems, *additionalItemsSchema,
                        currentScope, additionalItemsPath, fetchDoc);
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
                int index = 0;
                BOOST_FOREACH( const AdapterType v, items->getArray() ) {
                    const std::string childPath = itemsPath + "/" +
                            boost::lexical_cast<std::string>(index);
                    itemSchemas.push_back(new Schema());
                    Schema &childSchema = itemSchemas.back();
                    populateSchema<AdapterType>(rootNode, v, childSchema,
                            currentScope, childPath, fetchDoc);
                    index++;
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
                populateSchema<AdapterType>(rootNode, *items, childSchema,
                        currentScope, itemsPath, fetchDoc);
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
     * @param   rootNode          Reference to the node from which JSON
     *                            References will be resolved when they refer
     *                            to the current document; used for recursive
     *                            parsing of schemas
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
     * @brief   Make a new MultipleOfConstraint object.
     *
     * @param   node  JSON node containing an numeric value that a value must
     *                be divisible by.
     *
     * @return  pointer to a new MultipleOfConstraint that belongs to the
     *          caller
     */
    template<typename AdapterType>
    constraints::Constraint* makeMultipleOfConstraint(
        const AdapterType &node)
    {
        // Allow both integral and double types to be provided
        if (node.maybeInteger()) {
            return new constraints::MultipleOfIntegerConstraint(
                node.asInteger());
        } else if (node.maybeDouble()) {
            return new constraints::MultipleOfDecimalConstraint(
                node.asDouble());
        }

        throw std::runtime_error("Expected an numeric value for 'multipleOf' constraint.");
    }

    /**
     * @brief   Make a new NotConstraint object
     *
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing a schema
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     *
     * @return  pointer to a new NotConstraint object that belongs to the caller
     */
    template<typename AdapterType>
    constraints::NotConstraint* makeNotConstraint(
        const AdapterType &rootNode,
        const AdapterType &node,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type > fetchDoc)
    {
        if (node.maybeObject()) {
            Schema childSchema;
            populateSchema<AdapterType>(rootNode, node, childSchema,
                    currentScope, nodePath, fetchDoc);
            return new constraints::NotConstraint(childSchema);
        }

        throw std::runtime_error("Expected object value for 'not' constraint.");
    }

    /**
     * @brief   Make a new OneOfConstraint object
     *
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          JSON node containing an array of child schemas
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)
     *
     * @return  pointer to a new OneOfConstraint that belongs to the caller
     */
    template<typename AdapterType>
    constraints::OneOfConstraint* makeOneOfConstraint(
        const AdapterType &rootNode,
        const AdapterType &node,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type >
                fetchDoc)
    {
        constraints::OneOfConstraint::Schemas childSchemas;
        int index = 0;
        BOOST_FOREACH ( const AdapterType schemaNode, node.getArray() ) {
            const std::string childPath = nodePath + "/" +
                    boost::lexical_cast<std::string>(index);
            childSchemas.push_back(new Schema);
            populateSchema<AdapterType>(rootNode, schemaNode,
                childSchemas.back(), currentScope, childPath, fetchDoc);
            index++;
        }

        /// @todo: bypass deep copy of the child schemas
        return new constraints::OneOfConstraint(childSchemas);
    }

    /**
     * @brief   Make a new PatternConstraint object.
     *
     * @param   node      JSON node containing a pattern string
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
     * @param   rootNode                  Reference to the node from which JSON
     *                                    References will be resolved when they
     *                                    refer to the current document; used
     *                                    for recursive parsing of schemas
     * @param   properties                Optional pointer to a JSON node
     *                                    containing an object mapping property
     *                                    names to schemas.
     * @param   patternProperties         Optional pointer to a JSON node
     *                                    containing an object mapping pattern
     *                                    property names to schemas.
     * @param   additionalProperties      Optional pointer to a JSON node
     *                                    containing an additional properties
     *                                    schema or a boolean value.
     * @param   currentScope              URI for current resolution scope
     * @param   propertiesPath            JSON Pointer representing the path to
     *                                    the 'properties' node
     * @param   patternPropertiesPath     JSON Pointer representing the path to
     *                                    the 'patternProperties' node
     * @param   additionalPropertiesPath  JSON Pointer representing the path to
     *                                    the 'additionalProperties' node
     * @param   fetchDoc                  Function to fetch remote JSON
     *                                    documents (optional)
     * @param   parentSchema              Optional pointer to the Schema of the
     *                                    parent object, needed to support the
     *                                    'required' keyword in Draft 3.
     *
     * @return  pointer to a new Properties that belongs to the caller
     */
    template<typename AdapterType>
    constraints::PropertiesConstraint* makePropertiesConstraint(
        const AdapterType &rootNode,
        const AdapterType *properties,
        const AdapterType *patternProperties,
        const AdapterType *additionalProperties,
        boost::optional<std::string> currentScope,
        const std::string &propertiesPath,
        const std::string &patternPropertiesPath,
        const std::string &additionalPropertiesPath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type >
                fetchDoc,
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
                const std::string childPath = propertiesPath + "/" +
                        propertyName;
                Schema &childSchema = propertySchemas[propertyName];
                populateSchema<AdapterType>(
                        rootNode, m.second, childSchema, currentScope,
                        childPath, fetchDoc, parentSchema, &propertyName);
            }
        }

        // Populate a PropertySchemaMap for each of the properties defined by
        // the 'patternProperties' keyword
        PSM patternPropertySchemas;
        if (patternProperties) {
            BOOST_FOREACH( const Member m, patternProperties->getObject() ) {
                const std::string &propertyName = m.first;
                const std::string childPath = patternPropertiesPath + "/" +
                        propertyName;
                Schema &childSchema = patternPropertySchemas[propertyName];
                populateSchema<AdapterType>(
                        rootNode, m.second, childSchema, currentScope,
                        childPath, fetchDoc, parentSchema, &propertyName);
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
                populateSchema<AdapterType>(rootNode, *additionalProperties,
                        *additionalPropertiesSchema, currentScope,
                        additionalPropertiesPath, fetchDoc);
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
     * @brief   Make a new TypeConstraint object
     *
     * @param   rootNode      Reference to the node from which JSON References
     *                        will be resolved when they refer to the current
     *                        document; used for recursive parsing of schemas
     * @param   node          Node containing the name of a JSON type
     * @param   currentScope  URI for current resolution scope
     * @param   nodePath      JSON Pointer representing path to current node
     * @param   fetchDoc      Function to fetch remote JSON documents (optional)

     *
     * @return  pointer to a new TypeConstraint object.
     */
    template<typename AdapterType>
    constraints::TypeConstraint* makeTypeConstraint(
        const AdapterType &rootNode,
        const AdapterType &node,
        boost::optional<std::string> currentScope,
        const std::string &nodePath,
        boost::optional<typename FetchDocumentFunction<AdapterType>::Type >
                fetchDoc)
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
            int index = 0;
            BOOST_FOREACH( const AdapterType v, node.getArray() ) {
                if (v.isString()) {
                    const TC::JsonType jsonType = TC::jsonTypeFromString(v.getString());
                    if (jsonType == TC::kAny && version == kDraft4) {
                        throw std::runtime_error("'any' type is not supported in version 4 schemas.");
                    }
                    jsonTypes.insert(jsonType);
                } else if (v.isObject() && version == kDraft3) {
                    const std::string childPath = nodePath + "/" +
                            boost::lexical_cast<std::string>(index);
                    schemas.push_back(new Schema());
                    populateSchema<AdapterType>(rootNode, v, schemas.back(),
                            currentScope, childPath, fetchDoc);
                } else {
                    throw std::runtime_error("Type name should be a string.");
                }
                index++;
            }
        } else if (node.isObject() && version == kDraft3) {
            schemas.push_back(new Schema());
            populateSchema<AdapterType>(rootNode, node, schemas.back(),
                    currentScope, nodePath, fetchDoc);
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
