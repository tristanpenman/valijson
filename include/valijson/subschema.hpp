#pragma once
#ifndef __VALIJSON_SUBSCHEMA_HPP
#define __VALIJSON_SUBSCHEMA_HPP

#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <valijson/constraints/constraint.hpp>

namespace valijson {

/**
 * Represents a sub-schema within a JSON Schema
 *
 * While all JSON Schemas have at least one sub-schema, the root, some will
 * have additional sub-schemas that are defined as part of constraints that are
 * included in the schema. For example, a 'oneOf' constraint maintains a set of
 * references to one or more nested sub-schemas. As per the definition of a 
 * oneOf constraint, a document is valid within that constraint if it validates
 * against one of the nested sub-schemas.
 */
class Subschema
{
public:
    /// Typedef the Constraint class into the local namespace for convenience
    typedef constraints::Constraint Constraint;

    /// Typedef for a function that can be applied to each of the Constraint
    /// instances owned by a Schema.
    typedef boost::function<bool (const Constraint &)> ApplyFunction;

    /**
     * @brief  Construct a new Subschema object with no constraints
     */
    Subschema() { }

    /**
     * @brief  Copy an existing Subschema
     *
     * The constructed Subschema instance will contain a copy of each constraint
     * defined in the referenced Subschemas. Constraints will be copied only
     * as deep as references to other Subschemas - e.g. copies of constraints
     * that refer to sub-schemas, will continue to refer to the same Subschema
     * instances.
     *
     * @param  subschema  Subschema instance to copy constraints from
     */
    Subschema(const Subschema &subschema)
      : constraints(subschema.constraints),
        title(subschema.title) { }

    /**
     * @brief  Add a constraint to this sub-schema
     *
     * The constraint will be copied before being added to the list of
     * constraints for this Subschema. Note that constraints will be copied
     * only as deep as references to other Subschemas - e.g. copies of
     * constraints that refer to sub-schemas, will continue to refer to the
     * same Subschema instances.
     *
     * @param  constraint  Reference to the constraint to copy
     */
    void addConstraint(const Constraint &constraint)
    {
        constraints.push_back(constraint.clone());
    }

    /**
     * @brief  Add a constraint to this sub-schema
     *
     * This Subschema instance will take ownership of Constraint that is
     * pointed to, and will free it when it is no longer needed.
     *
     * @param  constraint  Pointer to the Constraint to take ownership of
     */
    void addConstraint(Constraint *constraint)
    {
        constraints.push_back(constraint);
    }

    /**
     * @brief  Invoke a function on each child Constraint
     *
     * This function will apply the callback function to each constraint in
     * the Subschema, even if one of the invokations returns \c false. However,
     * if one or more invokations of the callback function return \c false,
     * this function will also return \c false.
     *
     * @returns  \c true if all invokations of the callback function are
     *           successful, \c false otherwise
     */
    bool apply(ApplyFunction &applyFunction) const
    {
        bool allTrue = true;
        BOOST_FOREACH( const Constraint &constraint, constraints ) {
            allTrue = allTrue && applyFunction(constraint);
        }

        return allTrue;
    }

    /**
     * @brief  Invoke a function on each child Constraint
     *
     * This is a stricter version of the apply() function that will return
     * immediately if any of the invokations of the callback function return
     * \c false.
     *
     * @returns  \c true if all invokations of the callback function are
     *           successful, \c false otherwise
     */
    bool applyStrict(ApplyFunction &applyFunction) const
    {
        BOOST_FOREACH( const Constraint &constraint, constraints ) {
            if (!applyFunction(constraint)) {
                return false;
            }
        }

        return true;
    }

    /**
     * @brief  Get the description associated with this sub-schema
     *
     * @throws  std::runtime_error if a description has not been set
     *
     * @returns  string containing sub-schema description
     */
    std::string getDescription() const
    {
        if (description) {
            return *description;
        }

        throw std::runtime_error("Schema does not have a description");
    }

    /**
     * @brief  Get the ID associated with this sub-schema
     *
     * @throws  std::runtime_error if an ID has not been set
     *
     * @returns  string containing sub-schema ID
     */
    std::string getId() const
    {
        if (id) {
            return *id;
        }

        throw std::runtime_error("Schema does not have an ID");
    }

    /**
     * @brief  Get the title associated with this sub-schema
     *
     * @throws  std::runtime_error if a title has not been set
     *
     * @returns  string containing sub-schema title
     */
    std::string getTitle() const
    {
        if (title) {
            return *title;
        }

        throw std::runtime_error("Schema does not have a title");
    }

    /**
     * @brief  Check whether this sub-schema has a description
     *
     * @return boolean value
     */
    bool hasDescription() const
    {
        return description != boost::none;
    }

    /**
     * @brief  Check whether this sub-schema has an ID
     *
     * @return  boolean value
     */
    bool hasId() const
    {
        return id != boost::none;
    }

    /**
     * @brief  Check whether this sub-schema has a title
     *
     * @return  boolean value
     */
    bool hasTitle() const
    {
        return title != boost::none;
    }

    /**
     * @brief  Set the description for this sub-schema
     *
     * The description will not be used for validation, but may be used as part
     * of the user interface for interacting with schemas and sub-schemas. As
     * an example, it may be used as part of the validation error descriptions
     * that are produced by the Validator and ValidationVisitor classes.
     *
     * @param  description  new description
     */
    void setDescription(const std::string &description)
    {
        this->description = description;
    }

    void setId(const std::string &id)
    {
        this->id = id;
    }

    /**
     * @brief  Set the title for this sub-schema
     *
     * The title will not be used for validation, but may be used as part
     * of the user interface for interacting with schemas and sub-schema. As an
     * example, it may be used as part of the validation error descriptions 
     * that are produced by the Validator and ValidationVisitor classes.
     *
     * @param  title  new title
     */
    void setTitle(const std::string &title)
    {
        this->title = title;
    }

private:

    /// List of pointers to constraints that apply to this schema.
    boost::ptr_vector<Constraint> constraints;

    /// Schema description (optional)
    boost::optional<std::string> description;

    /// Id to apply when resolving the schema URI
    boost::optional<std::string> id;

    /// Title string associated with the schema (optional)
    boost::optional<std::string> title;
};

} // namespace valijson

#endif
