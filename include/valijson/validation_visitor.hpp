#pragma once
#ifndef __VALIJSON_VALIDATION_VISITOR_HPP
#define __VALIJSON_VALIDATION_VISITOR_HPP

#include <cmath>

#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/variant/get.hpp>

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
    bool validateSchema(const Subschema &subschema)
    {
        // Wrap the validationCallback() function below so that it will be
        // passed a reference to a constraint (_1), and a reference to the
        // visitor (*this).
        Subschema::ApplyFunction fn(boost::bind(validationCallback, _1, *this));

        // Perform validation against each constraint defined in the schema
        if (results == NULL) {
            // The applyStrict() function will return immediately if the
            // callback function returns false
            if (!subschema.applyStrict(fn)) {
                return false;
            }
        } else {
            // The apply() function will iterate over all constraints in the
            // schema, even if the callback function returns false. Once
            // iteration is complete, the apply() function will return true
            // only if all invokations of the callback function returned true.
            if (!subschema.apply(fn)) {
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
        bool validated = true;

        constraint.applyToSubschemas(ValidateSubschemas(target, context,
                true, false, *this, results, NULL, &validated));

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
        unsigned int numValidated = 0;

        constraint.applyToSubschemas(ValidateSubschemas(target, context, false,
                true, *this, results, &numValidated, NULL));

        if (numValidated == 0 && results) {
            results->pushError(context, "Failed to validate against any child "
                    "schemas allowed by anyOf constraint.");
        }

        return numValidated > 0;
    }

    /**
     * @brief   Validate current node against a 'dependencies' constraint
     *
     * A 'dependencies' constraint can be used to specify property-based or
     * schema-based dependencies that must be fulfilled when a particular
     * property is present in an object.
     *
     * Property-based dependencies define a set of properties that must be
     * present in addition to a particular property, whereas a schema-based
     * dependency defines an additional schema that the current document must
     * validate against.
     *
     * @param   constraint  DependenciesConstraint that the current node
     *                      must validate against
     *
     * @return  \c true if validation passes; \c false otherwise
     */
    virtual bool visit(const DependenciesConstraint &constraint)
    {
        // Ignore non-objects
        if ((strictTypes && !target.isObject()) || (!target.maybeObject())) {
            return true;
        }

        // Object to be validated
        const typename AdapterType::Object object = target.asObject();

        // Cleared if validation fails
        bool validated = true;

        // Iterate over all dependent properties defined by this constraint,
        // invoking the DependentPropertyValidator functor once for each
        // set of dependent properties
        constraint.applyToPropertyDependencies(ValidatePropertyDependencies(
                object, context, results, &validated));
        if (!results && !validated) {
            return false;
        }

        // Iterate over all dependent schemas defined by this constraint,
        // invoking the DependentSchemaValidator function once for each schema
        // that must be validated if a given property is present
        constraint.applyToSchemaDependencies(ValidateSchemaDependencies(
                object, context, *this, results, &validated));
        if (!results && !validated) {
            return false;
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
     * @brief   Validate a value against a LinearItemsConstraint
     
     * A LinearItemsConstraint represents an 'items' constraint that specifies,
     * for each item in array, an individual sub-schema that the item must
     * validate against. The LinearItemsConstraint class also captures the
     * presence of an 'additionalItems' constraint, which specifies a default
     * sub-schema that should be used if an array contains more items than
     * there are sub-schemas in the 'items' constraint.
     *
     * If the current value is not an array, validation always succeeds.
     *
     * @param  constraint  SingularItemsConstraint to validate against
     *
     * @returns  \c true if validation is successful; \c false otherwise
     */
    virtual bool visit(const LinearItemsConstraint &constraint)
    {
        // Ignore values that are not arrays
        if ((strictTypes && !target.isArray()) || (!target.maybeArray())) {
            return true;
        }

        // Sub-schema to validate against when number of items in array exceeds
        // the number of sub-schemas provided by the 'items' constraint
        const Subschema * const additionalItemsSubschema =
                constraint.getAdditionalItemsSubschema();

        // Track how many items are validated using 'items' constraint
        unsigned int numValidated = 0;

        // Array to validate
        const typename AdapterType::Array arr = target.asArray();
        const size_t arrSize = arr.size();

        // Track validation status
        bool validated = true;

        // Validate as many items as possible using 'items' sub-schemas
        const size_t itemSubschemaCount = constraint.getItemSubschemaCount();
        if (itemSubschemaCount > 0) {
            if (!additionalItemsSubschema) {
                if (arrSize > itemSubschemaCount) {
                    if (results) {
                        results->pushError(context,
                                "Array contains more items than allowed by "
                                "items constraint.");
                        validated = false;
                    } else {
                        return false;
                    }
                }
            }

            constraint.applyToItemSubschemas(ValidateItems(arr, context,
                    results != NULL, strictTypes, results, &numValidated,
                    &validated));

            if (!results && !validated) {
                return false;
            }
        }

        // Validate remaining items using 'additionalItems' sub-schema
        if (numValidated < arrSize) {
            if (additionalItemsSubschema) {
                // Begin validation from the first item not validated against
                // an sub-schema provided by the 'items' constraint
                unsigned int index = numValidated;
                typename AdapterType::Array::const_iterator begin = arr.begin();
                begin.advance(numValidated);
                for (typename AdapterType::Array::const_iterator itr = begin;
                        itr != arr.end(); ++itr) {

                    // Update context for current array item
                    std::vector<std::string> newContext = context;
                    newContext.push_back("[" +
                            boost::lexical_cast<std::string>(index) + "]");

                    ValidationVisitor<AdapterType> validator(*itr, newContext,
                            strictTypes, results);

                    if (!validator.validateSchema(*additionalItemsSubschema)) {
                        if (results) {
                            results->pushError(context,
                                    "Failed to validate item #" +
                                    boost::lexical_cast<std::string>(index) +
                                    " against additional items schema.");
                            validated = false;
                        } else {
                            return false;
                        }
                    }

                    index++;
                }

            } else if (results) {
                results->pushError(context, "Cannot validate item #" +
                    boost::lexical_cast<std::string>(numValidated) + " or "
                    "greater using 'items' constraint or 'additionalItems' "
                    "constraint.");
                validated = false;

            } else {
                return false;
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
        if ((strictTypes && !target.isNumber()) || !target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        if (constraint.exclusiveMaximum) {
            if (target.asDouble() >= constraint.maximum) {
                if (results) {
                    results->pushError(context, "Expected number less than " +
                        boost::lexical_cast<std::string>(constraint.maximum));
                }
                return false;
            }
        } else {
            if (target.asDouble() > constraint.maximum) {
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
        if ((strictTypes && !target.isArray()) || !target.maybeArray()) {
            return true;
        }

        if (target.asArray().size() <= constraint.maxItems) {
            return true;
        }

        if (results) {
            results->pushError(context, "Array should contain no more than " +
                boost::lexical_cast<std::string>(constraint.maxItems) +
                " elements.");
        }

        return false;
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
        if ((strictTypes && !target.isString()) || !target.maybeString()) {
            return true;
        }

        const std::string s = target.asString();
        const int len = utils::u8_strlen(s.c_str());
        if (len <= constraint.maxLength) {
            return true;
        }

        if (results) {
            results->pushError(context, "String should be no more than " +
                boost::lexical_cast<std::string>(constraint.maxLength) +
                " characters in length.");
        }

        return false;
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
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        if (target.asObject().size() <= constraint.maxProperties) {
            return true;
        }

        if (results) {
            results->pushError(context, "Object should have no more than" +
                boost::lexical_cast<std::string>(constraint.maxProperties) +
                " properties.");
        }

        return false;
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
        if ((strictTypes && !target.isNumber()) || !target.maybeDouble()) {
            // Ignore values that are not numbers
            return true;
        }

        if (constraint.exclusiveMinimum) {
            if (target.asDouble() <= constraint.minimum) {
                if (results) {
                    results->pushError(context,
                        "Expected number greater than " +
                        boost::lexical_cast<std::string>(constraint.minimum));
                }
                return false;
            }
        } else {
            if (target.asDouble() < constraint.minimum) {
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
        if ((strictTypes && !target.isArray()) || !target.maybeArray()) {
            return true;
        }

        if (target.asArray().size() >= constraint.minItems) {
            return true;
        }

        if (results) {
            results->pushError(context, "Array should contain no fewer than " +
                boost::lexical_cast<std::string>(constraint.minItems) +
                " elements.");
        }

        return false;
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
        if ((strictTypes && !target.isString()) || !target.maybeString()) {
            return true;
        }

        const std::string s = target.asString();
        const int len = utils::u8_strlen(s.c_str());
        if (len >= constraint.minLength) {
            return true;
        }

        if (results) {
            results->pushError(context, "String should be no fewer than " +
                boost::lexical_cast<std::string>(constraint.minLength) +
                " characters in length.");
        }

        return false;
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

        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        if (target.asObject().size() >= constraint.minProperties) {
            return true;
        }

        if (results) {
            results->pushError(context, "Object should have no fewer than" +
                boost::lexical_cast<std::string>(constraint.minProperties) +
                " properties.");
        }

        return false;
    }

    /**
     * @brief   Validate against the multipleOf or divisibleBy constraints
     *          represented by a MultipleOfConstraint object.
     *
     * @param   constraint  Constraint that the target must validate against.
     *
     * @return  true if the constraint is satisfied, false otherwise.
     */
    virtual bool visit(const MultipleOfConstraint &constraint)
    {
        const int64_t *multipleOfInteger = boost::get<int64_t>(&constraint.value);
        if (multipleOfInteger) {
            int64_t i = 0;
            if (target.maybeInteger()) {
                if (!target.asInteger(i)) {
                    if (results) {
                        results->pushError(context, "Value could not be converted "
                            "to an integer for multipleOf check");
                    }
                    return false;
                }
            } else if (target.maybeDouble()) {
                double d;
                if (!target.asDouble(d)) {
                    if (results) {
                        results->pushError(context, "Value could not be converted "
                            "to a double for multipleOf check");
                    }
                    return false;
                }
                i = static_cast<int64_t>(d);
            } else {
                return true;
            }

            if (i == 0) {
                return true;
            }

            if (i % *multipleOfInteger != 0) {
                if (results) {
                    results->pushError(context, "Value should be a multiple of " +
                        boost::lexical_cast<std::string>(*multipleOfInteger));
                }
                return false;
            }

            return true;
        }

        const double *multipleOfDouble = boost::get<double>(&constraint.value);
        if (multipleOfDouble) {
            double d = 0.;
            if (target.maybeDouble()) {
                if (!target.asDouble(d)) {
                    if (results) {
                        results->pushError(context, "Value could not be converted "
                            "to a number to check if it is a multiple of " +
                            boost::lexical_cast<std::string>(*multipleOfDouble));
                    }
                    return false;
                }
            } else if (target.maybeInteger()) {
                int64_t i = 0;
                if (!target.asInteger(i)) {
                    if (results) {
                        results->pushError(context, "Value could not be converted "
                            "to a number to check if it is a multiple of " +
                            boost::lexical_cast<std::string>(*multipleOfDouble));
                    }
                    return false;
                }
                d = static_cast<double>(i);
            } else {
                return true;
            }

            if (d == 0) {
                return true;
            }

            const double r = remainder(d, *multipleOfDouble);

            if (fabs(r) > std::numeric_limits<double>::epsilon()) {
                if (results) {
                    results->pushError(context, "Value should be a multiple of " +
                        boost::lexical_cast<std::string>(*multipleOfDouble));
                }
                return false;
            }

            return true;
        }

        return false;
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

        ValidationResults newResults;
        ValidationResults *childResults = (results) ? &newResults : NULL;

        constraint.applyToSubschemas(ValidateSubschemas(target, context,
                true, true, *this, childResults, &numValidated, NULL));

        if (numValidated == 0) {
            if (results) {
                ValidationResults::Error childError;
                while (childResults->popError(childError)) {
                    results->pushError(childError.context, childError.description);
                }
                results->pushError(context, "Failed to validate against any child schemas allowed by oneOf constraint.");
            }
            return false;
        } else if (numValidated != 1) {
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
        if ((strictTypes && !target.isString()) || !target.maybeString()) {
            return true;
        }

        const boost::regex r(constraint.pattern, boost::regex::perl);
        if (!boost::regex_search(target.asString(), r)) {
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
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            return true;
        }

        bool validated = true;

        const typename AdapterType::Object obj = target.asObject();

        // Validate each property in the target object
        BOOST_FOREACH( const typename AdapterType::ObjectMember m, obj ) {

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
                results->pushError(context, "Failed to match property name '" +
                        propertyName + "' to any names in 'properties' or "
                        "regexes in 'patternProperties'");
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
        if ((strictTypes && !target.isObject()) || !target.maybeObject()) {
            if (results) {
                results->pushError(context, "Object required to validate 'required' properties.");
            }
            return false;
        }

        bool validated = true;
        const typename AdapterType::Object object = target.asObject();
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
     * @brief  Validate a value against a SingularItemsConstraint
     *
     * A SingularItemsConstraint represents an 'items' constraint that specifies
     * a sub-schema against which all items in an array must validate. If the
     * current value is not an array, validation always succeeds.
     *
     * @param  constraint  SingularItemsConstraint to validate against
     *
     * @returns  \c true if validation is successful; \c false otherwise
     */
    virtual bool visit(const SingularItemsConstraint &constraint)
    {
        // Ignore values that are not arrays
        if (!target.isArray()) {
            return true;
        }

        // Schema against which all items must validate
        const Subschema *itemsSubschema = constraint.getItemsSubschema();

        // Default items sub-schema accepts all values
        if (!itemsSubschema) {
            return true;
        }

        // Track whether validation has failed
        bool validated = true;

        unsigned int index = 0;
        BOOST_FOREACH( const AdapterType &item, target.getArray() ) {
            // Update context for current array item
            std::vector<std::string> newContext = context;
            newContext.push_back("[" +
                    boost::lexical_cast<std::string>(index) + "]");

            // Create a validator for the current array item
            ValidationVisitor<AdapterType> validationVisitor(item,
                    newContext, strictTypes, results);

            // Perform validation
            if (!validationVisitor.validateSchema(*itemsSubschema)) {
                if (results) {
                    results->pushError(context,
                            "Failed to validate item #" +
                            boost::lexical_cast<std::string>(index) +
                            " in array.");
                    validated = false;
                } else {
                    return false;
                }
            }

            index++;
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

        BOOST_FOREACH( const Subschema *subschema, constraint.schemas ) {
            if (validateSchema(*subschema)) {
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
        if ((strictTypes && !target.isArray()) || !target.maybeArray()) {
            return true;
        }

        bool validated = true;

        const typename AdapterType::Array targetArray = target.asArray();
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

    struct ValidatePropertyDependencies
    {
        ValidatePropertyDependencies(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                ValidationResults *results,
                bool *validated)
          : object(object),
            context(context),
            results(results),
            validated(validated) { }

        template<typename StringType, typename ContainerType>
        bool operator()(
                const StringType &propertyName,
                const ContainerType &dependencyNames) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (object.find(propertyNameKey) == object.end()) {
                return true;
            }

            typedef typename ContainerType::value_type ValueType;
            BOOST_FOREACH( const ValueType &dependencyName, dependencyNames ) {
                const std::string dependencyNameKey(dependencyName.c_str());
                if (object.find(dependencyNameKey) == object.end()) {
                    if (validated) {
                        *validated = false;
                    }
                    if (results) {
                        results->pushError(context, "Missing dependency '" +
                                dependencyNameKey + "'.");
                    } else {
                        return false;
                    }
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &object;
        const std::vector<std::string> &context;
        ValidationResults * const results;
        bool * const validated;
    };

    /**
     * @brief  Functor to validate against sub-schemas in 'items' constraint
     */
    struct ValidateItems
    {
        ValidateItems(
                const typename AdapterType::Array &arr,
                const std::vector<std::string> &context,
                bool continueOnFailure,
                bool strictTypes,
                ValidationResults *results,
                unsigned int *numValidated,
                bool *validated)
          : arr(arr),
            context(context),
            continueOnFailure(continueOnFailure),
            strictTypes(strictTypes),
            results(results),
            numValidated(numValidated),
            validated(validated) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            // Check that there are more elements to validate
            if (index >= arr.size()) {
                return false;
            }

            // Update context
            std::vector<std::string> newContext = context;
            newContext.push_back(
                    "[" + boost::lexical_cast<std::string>(index) + "]");

            // Find array item
            typename AdapterType::Array::const_iterator itr = arr.begin();
            itr.advance(index);

            // Validate current array item
            ValidationVisitor validator(*itr, newContext, strictTypes, results);
            if (validator.validateSchema(*subschema)) {
                if (numValidated) {
                    (*numValidated)++;
                }

                return true;
            }

            if (validated) {
                *validated = false;
            }

            if (results) {
                results->pushError(newContext,
                    "Failed to validate item #" +
                    boost::lexical_cast<std::string>(index) +
                    " against corresponding item schema.");
            }

            return continueOnFailure;
        }

    private:
        const typename AdapterType::Array &arr;
        const std::vector<std::string> &context;
        bool continueOnFailure;
        bool strictTypes;
        ValidationResults * const results;
        unsigned int * const numValidated;
        bool * const validated;

    };

    struct ValidateSchemaDependencies
    {
        ValidateSchemaDependencies(
                const typename AdapterType::Object &object,
                const std::vector<std::string> &context,
                ValidationVisitor &validationVisitor,
                ValidationResults *results,
                bool *validated)
          : object(object),
            context(context),
            validationVisitor(validationVisitor),
            results(results),
            validated(validated) { }

        template<typename StringType>
        bool operator()(
                const StringType &propertyName,
                const Subschema *schemaDependency) const
        {
            const std::string propertyNameKey(propertyName.c_str());
            if (object.find(propertyNameKey) == object.end()) {
                return true;
            }

            if (!validationVisitor.validateSchema(*schemaDependency)) {
                if (validated) {
                    *validated = false;
                }
                if (results) {
                    results->pushError(context,
                            "Failed to validate against dependent schema.");
                } else {
                    return false;
                }
            }

            return true;
        }

    private:
        const typename AdapterType::Object &object;
        const std::vector<std::string> &context;
        ValidationVisitor &validationVisitor;
        ValidationResults * const results;
        bool * const validated;
    };

    struct ValidateSubschemas
    {
        ValidateSubschemas(
                const AdapterType &adapter,
                const std::vector<std::string> &context,
                bool continueOnSuccess,
                bool continueOnFailure,
                ValidationVisitor &validationVisitor,
                ValidationResults *results,
                unsigned int *numValidated,
                bool *validated)
          : adapter(adapter),
            context(context),
            continueOnSuccess(continueOnSuccess),
            continueOnFailure(continueOnFailure),
            validationVisitor(validationVisitor),
            results(results),
            numValidated(numValidated),
            validated(validated) { }

        bool operator()(unsigned int index, const Subschema *subschema) const
        {
            if (validationVisitor.validateSchema(*subschema)) {
                if (numValidated) {
                    (*numValidated)++;
                }

                return continueOnSuccess;
            }

            if (validated) {
                *validated = false;
            }

            if (results) {
                results->pushError(context,
                    "Failed to validate against child schema #" +
                    boost::lexical_cast<std::string>(index) + ".");
            }

            return continueOnFailure;
        }

    private:
        const AdapterType &adapter;
        const std::vector<std::string> &context;
        bool continueOnSuccess;
        bool continueOnFailure;
        ValidationVisitor &validationVisitor;
        ValidationResults * const results;
        unsigned int * const numValidated;
        bool * const validated;
    };

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
