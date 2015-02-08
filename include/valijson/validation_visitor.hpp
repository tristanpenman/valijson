#ifndef __VALIJSON_VALIDATION_VISITOR_HPP
#define __VALIJSON_VALIDATION_VISITOR_HPP

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>

#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/constraints/constraint_visitor.hpp>
#include <valijson/validation_results.hpp>

#include <valijson/utils/utf8_utils.hpp>

namespace valijson {

class ValidationResults;

/**
 * @brief   Implementation of the ConstraintVisitor interface that validates a
 *          target document.
 *
 * @tparam  AdapterType  Adapter type for the target document.
 */
template<typename AdapterType>
class ValidationVisitor: public constraints::ConstraintVisitor
{
public:

    /**
     * @brief  Construct a new validator for a given target value and context.
     *
     * @param  target       Target value to be validated
     * @param  context      Current context for validation error descriptions,
     *                      only used if results is set.
     * @param  strictTypes  Use strict type comparison
     * @param  results      Optional pointer to ValidationResults object, for
     *                      recording error descriptions. If this pointer is set
     *                      to NULL, validation errors will caused validation to
     *                      stop immediately.
     */
    ValidationVisitor(const AdapterType &target,
                      const std::vector<std::string> &context,
                      const bool strictTypes,
                      ValidationResults *results)
      : target(target),
        context(context),
        strictTypes(strictTypes),
        results(results) { }

    /**
     * @brief  Validate the target against a schema.
     *
     * When a ValidationResults object has been set via the 'results' member
     * variable, validation will proceed as long as no fatal errors occur,
     * with error descriptions added to the ValidationResults object.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the constraints are validated
     * successfully.
     *
     * @param  schema  Schema that the target must validate against
     *
     * @return  true if validation passes, false otherwise
     */
    bool validateSchema(const Schema &schema)
    {
        // Wrap the validationCallback() function below so that it will be
        // passed a reference to a constraint (_1), and a reference to the
        // visitor (*this).
        Schema::ApplyFunction fn(boost::bind(validationCallback, _1, *this));

        // Perform validation against each constraint defined in the schema
        if (results == NULL) {
            // The applyStrict() function will return immediately if the
            // callback function returns false
            if (!schema.applyStrict(fn)) {
                return false;
            }
        } else {
            // The apply() function will iterate over all constraints in the
            // schema, even if the callback function returns false. Once
            // iteration is complete, the apply() function will return true
            // only if all invokations of the callback function returned true.
            if (!schema.apply(fn)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief  Validate the target against an allOf constraint.
     *
     * An allOf constraint provides a set of child schemas against which the
     * target must be validated in order for the constraint to the satifisfied.
     *
     * When a ValidationResults object has been set via the 'results' member
     * variable, validation will proceed as long as no fatal errors occur,
     * with error descriptions added to the ValidationResults object.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the child schemas are validated
     * successfully.
     *
     * @param  constraint  Constraint that the target must validate against
     *
     * @return  true if validation passes, false otherwise
     */
    virtual bool visit(const AllOfConstraint &constraint)
    {
        // Flag used to track validation status if errors are non-fatal
        bool validated = true;

        // Validate against each child schema
        unsigned int index = 0;
        BOOST_FOREACH( const Schema &schema, constraint.schemas ) {

            // Ensure that the target validates against child schema
            if (!validateSchema(schema)) {
                if (results) {
                    validated = false;
                    results->pushError(context,
                        std::string("Failed to validate against child schema #") +
                        boost::lexical_cast<std::string>(index) + " of allOf constraint.");
                } else {
                    return false;
                }
            }

            index++;
        }

        return validated;
    }

    /**
     * @brief   Validate against the anyOf constraint represented by an
     *          AnyOfConstraint object.
     *
     * An anyOf constraint provides a set of child schemas, any of which the
     * target may be validated against in order for the constraint to the
     * satifisfied.
     *
     * Because an anyOf constraint does not require the target to validate
     * against all child schemas, if validation against a single schema fails,
     * the results will not be added to a ValidationResults object. Only if
     * validation fails for all child schemas will an error be added to the
     * ValidationResults object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if validation passes, false otherwise.
     */
    virtual bool visit(const AnyOfConstraint &constraint)
    {
        // Wrap the validationCallback() function below so that it will be
        // passed a reference to a constraint (_1), and a reference to the
        // visitor (*this).
        Schema::ApplyFunction fn(boost::bind(validationCallback, _1, *this));

        BOOST_FOREACH( const Schema &schema, constraint.schemas ) {
            if (schema.apply(fn)) {
                return true;
            }
        }

        if (results) {
            results->pushError(context, "Failed to validate against any child schemas allowed by anyOf constraint.");
        }

        return false;
    }

    /**
     * @brief   Validate against the dependencies constraint represented by a
     *          DependenciesConstraint object.
     *
     * A dependencies constraint can specify either a mapping of attribute names
     * to their dependencies, or a mapping of attribute names to child schemas
     * that must be satisfied if a given attribute is present.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if validation passes, false otherwise.
     */
    virtual bool visit(const DependenciesConstraint &constraint)
    {
        // Ignore non-objects
        if (!target.isObject()) {
            return true;
        }

        // Typedef and reference for conciseness in nested loops
        typedef DependenciesConstraint::PropertyDependenciesMap PDM;
        const PDM &deps = constraint.dependencies;

        typedef DependenciesConstraint::PropertyDependentSchemasMap PDSM;
        const PDSM &depSchemas = constraint.dependentSchemas;

        // Get access to the target as an object
        const typename AdapterType::Object object = target.getObject();

        // Flag used to track validation status if errors are non-fatal
        bool validated = true;

        // For each property in the object, check for a list of dependencies in
        // the constraint object and verify that the dependencies have been
        // satisfied.
        BOOST_FOREACH( const typename AdapterType::ObjectMember m, object ) {

            // Check for this property in the dependency map. If it is not
            // present, we can move on to the next one...
            PDM::const_iterator itr = deps.find(m.first);
            if (itr != deps.end()) {
                BOOST_FOREACH( const std::string &name, itr->second ) {
                    if (object.find(name) == object.end()) {
                        if (!results) {
                            return false;
                        }
                        validated = false;
                        results->pushError(context, "Missing dependency '" + name + "'.");
                    }
                }
            }

            // Check for this property in the dependent schemas map. If it is
            // present then we need to validate the current target against the
            // dependent schema.
            PDSM::const_iterator depSchemasItr = depSchemas.find(m.first);
            if (depSchemasItr != depSchemas.end()) {
                const Schema *schema = depSchemasItr->second;
                if (!validateSchema(*schema)) {
                    if (results) {
                        results->pushError(context, "Failed to validate against dependent schema.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
            }
        }

        return validated;
    }

    /**
     * @brief   Validate against the enum constraint represented by an
     *          EnumConstraint object.
     *
     * Validation succeeds if the target is equal to one of the values provided
     * by the enum constraint.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if validation succeeds, false otherwise.
     */
    virtual bool visit(const EnumConstraint &constraint)
    {
        // Compare the target with each 'frozen' value in the enum constraint.
        BOOST_FOREACH( const adapters::FrozenValue &value, constraint.values ) {
            if (value.equalTo(target, true)) {
                return true;
            }
        }

        if (results) {
            results->pushError(context, "Failed to match against any enum values.");
        }

        return false;
    }

    /**
     * @brief   Validate against the items and additionalItems constraints
     *          represented by an ItemsConstraint object.
     *
     * An items constraint restricts the values in array to those that match a
     * given set of schemas. An item constraint can specify either an ordered
     * list of child schemas that will be used to validate the corresponding
     * value in the target array, or a single schema that will be used to
     * validate all items.
     *
     * If a list of child schemas is used, then the additionalItems constraint
     * will also be considered. If present, the schema derived from the
     * additionalItems constraint will  be used to validate items that do not
     * have a corresponding child schema in the items constraint. If the
     * items constraint was not provided, then the additionalItems schema will
     * be used to validate all items in the array.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if validatation succeeds, false otherwise.
     */
    virtual bool visit(const ItemsConstraint &constraint)
    {
        // Ignore values that are not arrays
        if (!target.isArray()) {
            return true;
        }

        bool validated = true;

        if (constraint.itemSchema) {

            // Validate all items against single schema
            unsigned int index = 0;
            BOOST_FOREACH( const AdapterType arrayItem, target.getArray() ) {
                std::vector<std::string> newContext = context;
                newContext.push_back("[" + boost::lexical_cast<std::string>(index) + "]");
                ValidationVisitor<AdapterType> v(arrayItem,
                    newContext, strictTypes, results);
                if (!v.validateSchema(*constraint.itemSchema)) {
                    if (results) {
                        results->pushError(context, "Failed to validate item #" + boost::lexical_cast<std::string>(index) + " in array.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
                ++index;
            }

        } else if (constraint.itemSchemas) {

            if (!constraint.additionalItemsSchema) {
                // Check that the array length is <= length of the itemsSchema list
                if (target.getArray().size() > constraint.itemSchemas->size()) {
                    if (results) {
                        results->pushError(context, "Array contains more items than allowed by items constraint.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
            }

            // Validate items against the schema with the same index, or
            // additionalItems schema
            unsigned int index = 0;
            BOOST_FOREACH( const AdapterType arrayItem, target.getArray() ) {
                std::vector<std::string> newContext = context;
                newContext.push_back("[" + boost::lexical_cast<std::string>(index) + "]");
                ValidationVisitor<AdapterType> v(arrayItem,
                    newContext, strictTypes, results);
                if (index >= constraint.itemSchemas->size()) {
                    if (constraint.additionalItemsSchema) {
                        if (!v.validateSchema(*constraint.additionalItemsSchema)) {
                            if (results) {
                                results->pushError(context, "Failed to validate item #" +
                                    boost::lexical_cast<std::string>(index) + " against additional items schema.");
                                validated = false;
                            } else {
                                return false;
                            }
                        }
                    } else {
                        results->pushError(context, "Cannot validate item #" +
                            boost::lexical_cast<std::string>(index) + " in array due to missing schema.");
                        validated = false;
                    }
                } else if (!v.validateSchema(constraint.itemSchemas->at(index))) {
                    if (results) {
                        results->pushError(context, "Failed to validate item #" +
                            boost::lexical_cast<std::string>(index) + " against corresponding item schema.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
                ++index;
            }


        } else if (constraint.additionalItemsSchema) {

            // Validate each item against additional items schema
            unsigned int index = 0;
            BOOST_FOREACH( const AdapterType arrayItem, target.getArray() ) {
                std::vector<std::string> newContext = context;
                newContext.push_back("[" + boost::lexical_cast<std::string>(index) + "]");
                ValidationVisitor<AdapterType> v(arrayItem,
                    newContext, strictTypes, results);
                if (!v.validateSchema(*constraint.additionalItemsSchema)) {
                    if (results) {
                        results->pushError(context, "Failed to validate item #" +
                            boost::lexical_cast<std::string>(index) + " against additional items schema.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
                ++index;
            }

        }

        return validated;
    }

    /**
     * @brief   Validate against the maximum and exclusiveMaximum constraints
     *          represented by a MaximumConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if constraints are satisfied, false otherwise.
     */
    virtual bool visit(const MaximumConstraint &constraint)
    {
        if (!target.isNumber()) {
            // Ignore values that are not numbers
            return true;
        }

        if (constraint.exclusiveMaximum) {
            if (target.getNumber() >= constraint.maximum) {
                if (results) {
                    results->pushError(context,
                        "Expected number less than " + boost::lexical_cast<std::string>(constraint.maximum));
                }
                return false;
            }
        } else {
            if (target.getNumber() > constraint.maximum) {
                if (results) {
                    results->pushError(context,
                        "Expected number less than or equal to" +
                            boost::lexical_cast<std::string>(constraint.maximum));
                }
                return false;
            }
        }

        return true;
    }

    /**
     * @brief   Validate against the maxItems constraint represented by a
     *          MaxItemsConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MaxItemsConstraint &constraint)
    {
        if (target.isArray() &&
            target.getArray().size() > constraint.maxItems) {
            if (results) {
                results->pushError(context, "Array should contain no more than " +
                    boost::lexical_cast<std::string>(constraint.maxItems) +
                    " elements.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate against the maxLength constraint represented by a
     *          MaxLengthConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MaxLengthConstraint &constraint)
    {
        if (target.isString()) {
            const std::string s = target.getString();
            const int len = utils::u8_strlen(s.c_str());
            if (len > constraint.maxLength) {
                if (results) {
                    results->pushError(context, "String should be no more than " +
                        boost::lexical_cast<std::string>(constraint.maxLength) +
                        " characters in length.");
                }
                return false;
            }
        }

        return true;
    }

    /**
     * @brief   Validate against the maxProperties constraint represented by a
     *          MaxPropertiesConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MaxPropertiesConstraint &constraint)
    {
        if (target.isObject() &&
            target.getObject().size() > constraint.maxProperties) {
            if (results) {
                results->pushError(context, "Object should have no more than" +
                    boost::lexical_cast<std::string>(constraint.maxProperties) +
                    " properties.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate against the minimum constraint represented by a
     *          MinimumConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MinimumConstraint &constraint)
    {
        if (!target.isNumber()) {
            // Ignore values that are not numbers
            return true;
        }

        if (constraint.exclusiveMinimum) {
            if (target.getNumber() <= constraint.minimum) {
                if (results) {
                    results->pushError(context,
                        "Expected number greater than " +
                        boost::lexical_cast<std::string>(constraint.minimum));
                }
                return false;
            }
        } else {
            if (target.getNumber() < constraint.minimum) {
                if (results) {
                    results->pushError(context,
                        "Expected number greater than or equal to" +
                        boost::lexical_cast<std::string>(constraint.minimum));
                }
                return false;
            }
        }

        return true;
    }

    /**
     * @brief   Validate against the minItems constraint represented by a
     *          MinItemsConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MinItemsConstraint &constraint)
    {
        if (target.isArray() &&
            target.getArray().size() < constraint.minItems) {
            if (results) {
                results->pushError(context, "Array should contain no fewer than " +
                    boost::lexical_cast<std::string>(constraint.minItems) +
                    " elements.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate against the minLength constraint represented by a
     *          MinLengthConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MinLengthConstraint &constraint)
    {
        if (target.isString()) {
            const std::string s = target.getString();
            const int len = utils::u8_strlen(s.c_str());
            if (len < constraint.minLength) {
                if (results) {
                    results->pushError(context, "String should be no fewer than " +
                        boost::lexical_cast<std::string>(constraint.minLength) +
                        " characters in length.");
                }
                return false;
            }
        }

        return true;
    }

    /**
     * @brief   Validate against the minProperties constraint represented by a
     *          MinPropertiesConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MinPropertiesConstraint &constraint)
    {
        if (target.isObject() &&
            target.getObject().size() < constraint.minProperties) {
            if (results) {
                results->pushError(context, "Object should have no fewer than" +
                    boost::lexical_cast<std::string>(constraint.minProperties) +
                    " properties.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate against the multipleOf or divisibleBy constraints
     *          represented by a MultipleOfConstraint object.
     *
     * @todo    Not implemented.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MultipleOfConstraint &constraint)
    {
        return true;
    }

    /**
     * @brief   Validate against the not constraint represented by a
     *          NotConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const NotConstraint &constraint)
    {
        ValidationVisitor<AdapterType> v(target, context, strictTypes, NULL);

        if (v.validateSchema(*constraint.schema)) {
            if (results) {
                results->pushError(context, "Target should not validate against schema specified in 'not' constraint.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate against the oneOf constraint represented by a
     *          OneOfConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const OneOfConstraint &constraint)
    {
        unsigned int numValidated = 0;

        BOOST_FOREACH( const Schema &schema, constraint.schemas ) {
            if (validateSchema(schema)) {
                numValidated++;
            }
        }

        if (numValidated != 1) {
            if (results) {
                results->pushError(context, "Failed to validate against exactly one child schema.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate against the pattern constraint represented by a
     *          PatternConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const PatternConstraint &constraint)
    {
        if (!target.isString()) {
            return true;
        }

        const boost::regex r(constraint.pattern, boost::regex::perl);
        if (!boost::regex_search(target.getString(), r)) {
            if (results) {
                results->pushError(context, "Failed to match regex specified by 'pattern' constraint.");
            }
            return false;
        }

        return true;
    }

    /**
     * @brief   Validate against the properties, patternProperties, and
     *          additionalProperties constraints represented by a
     *          PatternConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const PropertiesConstraint &constraint)
    {
        if (!target.isObject()) {
            return true;
        }

        bool validated = true;

        // Validate each property in the target object
        BOOST_FOREACH( const typename AdapterType::ObjectMember m, target.getObject() ) {

            const std::string propertyName = m.first;
            bool propertyNameMatched = false;

            std::vector<std::string> newContext = context;
            newContext.push_back("[\"" + m.first + "\"]");

            ValidationVisitor<AdapterType> v(m.second,
                newContext, strictTypes, results);

            // Search for matching property name
            PropertiesConstraint::PropertySchemaMap::const_iterator itr =
                constraint.properties.find(propertyName);
            if (itr != constraint.properties.end()) {
                propertyNameMatched = true;
                if (!v.validateSchema(*itr->second)) {
                    if (results) {
                        results->pushError(context,
                            "Failed to validate against schema associated with property name '" +
                            propertyName + "' in properties constraint.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
            }

            // Search for a regex that matches the property name
            for (itr = constraint.patternProperties.begin(); itr != constraint.patternProperties.end(); ++itr) {
                const boost::regex r(itr->first, boost::regex::perl);
                if (boost::regex_search(propertyName, r)) {
                    propertyNameMatched = true;
                    // Check schema
                    if (!v.validateSchema(*itr->second)) {
                        if (results) {
                            results->pushError(context,
                                "Failed to validate against schema associated with regex '" +
                                itr->first + "' in patternProperties constraint.");
                            validated = false;
                        } else {
                            return false;
                        }
                    }
                }
            }

            // If the property name has been matched by a name in 'properties'
            // or a regex in 'patternProperties', then it should not be
            // validated against the 'additionalPatterns' schema.
            if (propertyNameMatched) {
                continue;
            }

            // If an additionalProperties schema has been provided, the values
            // associated with unmatched property names should be validated
            // against that schema.
            if (constraint.additionalProperties) {
                if (v.validateSchema(*constraint.additionalProperties)) {
                    continue;
                } else if (results) {
                    results->pushError(context, "Failed to validate property '" +
                        propertyName + "' against schema in additionalProperties constraint.");
                    validated = false;
                } else {
                    return false;
                }
            } else if (results) {
                results->pushError(context, "Failed to match property name to any names in 'properties' or regexes in 'patternProperties'");
                validated = false;
            } else {
                return false;
            }
        }

        return validated;
    }

    /**
     * @brief   Validate against the required constraint represented by a
     *          RequiredConstraint object.
     *
     * A required constraint specifies a list of properties that must be present
     * in the target.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  true if validation succeeds, false otherwise
     */
    virtual bool visit(const RequiredConstraint &constraint)
    {
        if (!target.isObject()) {
            if (results) {
                results->pushError(context, "Object required to validate 'required' properties.");
            }
            return false;
        }

        bool validated = true;
        const typename AdapterType::Object object = target.getObject();
        BOOST_FOREACH( const std::string &requiredProperty, constraint.requiredProperties ) {
            if (object.find(requiredProperty) == object.end()) {
                if (results) {
                    results->pushError(context, "Missing required property '" + requiredProperty + "'.");
                    validated = false;
                } else {
                    return false;
                }
            }
        }

        return validated;
    }

    /**
     * @brief   Validate against the type constraint represented by a
     *          TypeConstraint object.
     *
     * Checks that the target is of the expected type.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  true if validation succeeds, false otherwise
     */
    virtual bool visit(const TypeConstraint &constraint)
    {
        // Try to match the type to one of the types in the jsonTypes array
        BOOST_FOREACH( const TypeConstraint::JsonType jsonType, constraint.jsonTypes ) {
            switch (jsonType) {
            case TypeConstraint::kAny:
                return true;
            case TypeConstraint::kArray:
                if (target.isArray()) {
                    return true;
                }
                break;
            case TypeConstraint::kBoolean:
                if (target.isBool() || (!strictTypes && target.maybeBool())) {
                    return true;
                }
                break;
            case TypeConstraint::kInteger:
                if (target.isInteger() || (!strictTypes && target.maybeInteger())) {
                    return true;
                }
                break;
            case TypeConstraint::kNull:
                if (target.isNull() || (!strictTypes && target.maybeNull())) {
                    return true;
                }
                break;
            case TypeConstraint::kNumber:
                if (target.isNumber() || (!strictTypes && target.maybeDouble())) {
                    return true;
                }
                break;
            case TypeConstraint::kObject:
                if (target.isObject()) {
                    return true;
                }
                break;
            case TypeConstraint::kString:
                if (target.isString()) {
                    return true;
                }
                break;
            }
        }

        BOOST_FOREACH( const Schema &schema, constraint.schemas ) {
            if (validateSchema(schema)) {
                return true;
            }
        }

        if (results) {
            results->pushError(context, "Value type not permitted by 'type' constraint.");
        }

        return false;
    }

    /**
     * @brief   Validate the uniqueItems constraint represented by a
     *          UniqueItems object.
     *
     * A uniqueItems constraint requires that each of the values in an array
     * are unique. Comparison is performed recursively.
     *
     * @param   constraint  Constraint that the target must validate against
     *
     * @return  true if validation succeeds, false otherwise
     */
    virtual bool visit(const UniqueItemsConstraint &constraint)
    {
        if (!target.isArray()) {
            return true;
        }

        bool validated = true;

        const typename AdapterType::Array targetArray = target.getArray();
        const typename AdapterType::Array::const_iterator end = targetArray.end();
        const typename AdapterType::Array::const_iterator secondLast = --targetArray.end();
        unsigned int outerIndex = 0;
        for (typename AdapterType::Array::const_iterator outerItr = targetArray.begin(); outerItr != secondLast; ++outerItr) {
            unsigned int innerIndex = 0;
            typename AdapterType::Array::const_iterator innerItr(outerItr);
            for (++innerItr; innerItr != end; ++innerItr) {
                if (outerItr->equalTo(*innerItr, true)) {
                    if (results) {
                        results->pushError(context, "Elements at indexes #" +
                            boost::lexical_cast<std::string>(outerIndex) + " and #" +
                            boost::lexical_cast<std::string>(innerIndex) + " violate uniqueness constraint.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
                ++innerIndex;
            }
            ++outerIndex;
        }

        return validated;
    }

private:

    /**
     * @brief  Callback function that passes a visitor to a constraint.
     *
     * @param  constraint  Reference to constraint to be visited
     * @param  visitor     Reference to visitor to be applied
     *
     * @return  true if the visitor returns successfully, false otherwise.
     */
    static bool validationCallback(const constraints::Constraint &constraint,
                                   ValidationVisitor<AdapterType> &visitor)
    {
        return constraint.accept(visitor);
    }

    /// Reference to the JSON value being validated
    const AdapterType &target;

    /// Vector of strings describing the current object context
    const std::vector<std::string> context;

    /// Optional pointer to a ValidationResults object to be populated
    ValidationResults *results;

    /// Option to use strict type comparison
    const bool strictTypes;

};

}  // namespace valijson

#endif
