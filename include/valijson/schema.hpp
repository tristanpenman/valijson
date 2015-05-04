#ifndef __VALIJSON_SCHEMA_HPP
#define __VALIJSON_SCHEMA_HPP

#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

#include <valijson/constraints/constraint.hpp>

namespace valijson {

/**
 * @brief  Class that holds a list of Constraints that together form a schema.
 *
 * This class maintains an internal list of Constraint objects that define a
 * schema. It provides useful functionality such as the ability to easily make
 * independent copies of a schema.
 *
 * Schemas can be modified after construction by adding more constraints, or
 * by setting a schema title.
 */
class Schema
{
public:

    /// Typedef the Constraint class into the local namespace for convenience
    typedef constraints::Constraint Constraint;

    /// Typedef for a function that can be applied to each of the Constraint
    /// instances owned by a Schema.
    typedef boost::function<bool (const Constraint &)> ApplyFunction;

    /**
     * @brief  Construct a new Schema object with no constraints, using the
     *         default scope.
     *
     * The constructed Schema object will have no constraints.
     */
    Schema() { }

    /**
     * @brief  Construct a new Schema object with no constraints, and inherit
     *         the scope of another Schema.
     *
     * The functions getScope(), getCanonicalURI() and getInheritedURI() use
     * the scope provided by this constructor, unless the URI for this schema
     * specifies its own scope.
     *
     * @param  parentScope  Scope to inherit.
     */
    Schema(const std::string &parentScope)
      : parentScope(parentScope) {}

    /**
     * @brief  Construct a new Schema based an existing Schema object.
     *
     * The constructed Schema object will contain a copy of each constraint
     * defined in the referenced Schema.
     *
     * @param  schema  schema to copy constraints from
     */
    Schema(const Schema &schema)
      : constraints(schema.constraints),
        parentScope(schema.parentScope),
        title(schema.title) { }

    /**
     * @brief  Add a constraint to the schema.
     *
     * A copy of the referenced Constraint object will be added to the Schema.
     *
     * @param  constraint  Reference to the constraint to copy.
     */
    void addConstraint(const Constraint &constraint)
    {
        constraints.push_back(constraint.clone());
    }

    /**
     * @brief  Add a constraint to the schema.
     *
     * The schema will take ownership of the provided Constraint.
     *
     * @param  constraint  Pointer to the constraint to take ownership of.
     */
    void addConstraint(Constraint *constraint)
    {
        constraints.push_back(constraint);
    }

    /**
     * @brief  Invoke a function on each constraint in the schema.
     *
     * This function will apply the callback function to each constraint in
     * the schema, even if one of the invokations returns false. If a single
     * invokation returns false, this function will return false.
     *
     * @returns  true if all invokations of the callback function are
     *           successful, false otherwise.
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
     * @brief  Invoke a function on each constraint in the schema.
     *
     * This is a stricter version of the apply() function that will return
     * immediately if any of the invokations return false.
     *
     * @returns  true if all invokations of the callback function are
     *           successful, false otherwise.
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

    std::string getId() const
    {
        if (id) {
            return *id;
        }

        throw std::runtime_error("id has not been set");
    }

    std::string getScope() const
    {
        return std::string();
    }

    std::string getUri() const
    {
        return std::string();
    }

    /**
     * @brief  Get the title for this schema.
     *
     * If the title has not been set, this function will throw an exception.
     *
     * @throw std::runtime_error
     *
     * @return  schema title string
     */
    std::string getTitle() const
    {
        if (title) {
            return *title;
        }

        throw std::runtime_error("Schema does not have a title.");
    }

    /**
     * @brief  Returns a boolean value that indicates whether the id has been
     *         set or not.
     *
     * @return  boolean value
     */
    bool hasId() const
    {
        return id != boost::none;
    }

    /**
     * @brief  Returns a boolean value that indicates whether the schema title
     *         has been set or not.
     *
     * @return  boolean value
     */
    bool hasTitle() const
    {
        return title != boost::none;
    }

    std::string resolveUri(const std::string &relative) const
    {
        return relative;
    }

    void setId(const std::string &id)
    {
        this->id = id;
    }

    /**
     * @brief  Set the title for this schema.
     *
     * The title is not used for validation, but can be used as part of the
     * validation error descriptions that are produced by the Validator and
     * ValidationVisitor classes.
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

    /// Id to apply when resolving the schema URI.
    boost::optional<std::string> id;

    /// Scope inherited from a parent schema, or an empty string by default
    boost::optional<std::string> parentScope;

    /// Title string associated with the schema (optional).
    boost::optional<std::string> title;
};

} // namespace valijson

#endif
