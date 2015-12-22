#pragma once
#ifndef __VALIJSON_SCHEMA_HPP
#define __VALIJSON_SCHEMA_HPP

#include <cstdio>
#include <set>

#include <valijson/subschema.hpp>

namespace valijson {

/**
 * Represents the root of a JSON Schema
 *
 * The root is distinct from other sub-schemas because it is the canonical
 * starting point for validation of a document against a given a JSON Schema.
 */
class Schema: public Subschema
{
public:
    /**
     * @brief  Construct a new Schema instance with no constraints
     */
    Schema() {}

    /**
     * @brief  Clean up and free all memory managed by the Schema
     *
     * Note that any Subschema pointers created and returned by this Schema
     * should be considered invalid.
     */
    virtual ~Schema()
    {
        try {
            while (!subschemaSet.empty()) {
                std::set<Subschema*>::iterator itr = subschemaSet.begin();
                Subschema *subschema = *itr;
                subschemaSet.erase(itr);
                subschema->~Subschema();
                // TODO: Replace with custom free function
                ::operator delete(subschema);
            }
        } catch (const std::exception &e) {
            fprintf(stderr, "Caught an exception while destroying Schema: %s",
                    e.what());
        }
    }

    /**
     * @brief  Copy a constraint to a specific sub-schema
     *
     * @param  constraint  reference to a constraint that will be copied into
     *                     the sub-schema
     * @param  subschema   pointer to the sub-schema that will own the copied
     *                     constraint
     *
     * @throws std::runtime_error if the sub-schema is not owned by this Schema
     *         instance
     */
    void addConstraintToSubschema(const Constraint &constraint,
            const Subschema *subschema)
    {
        if (subschema == this) {
            addConstraint(constraint);
            return;
        }

        Subschema *noConst = const_cast<Subschema*>(subschema);
        if (subschemaSet.find(noConst) == subschemaSet.end()) {
            throw std::runtime_error(
                    "Subschema pointer is not owned by this Schema instance");
        }

        // TODO: Check heirarchy for subschemas that do not belong...

        noConst->addConstraint(constraint);
    }

    /**
     * @brief  Create a new Subschema instance that is owned by this Schema
     *
     * @returns  const pointer to the new Subschema instance
     */
    const Subschema * createSubschema()
    {
        // TODO: Replace with custom malloc function
        void *ptr = ::operator new(sizeof(Subschema));
        if (!ptr) {
            throw std::runtime_error(
                    "Failed to allocate memory for sub-schema");
        }

        Subschema *subschema = NULL;
        try {
            subschema = new (ptr) Subschema();
            if (!subschema) {
                throw std::runtime_error("Failed to construct sub-schema");
            }
            ptr = NULL;
        } catch (...) {
            // TODO: Replace with custom free function
            ::operator delete(ptr);
            throw;
        }

        try {
            if (!subschemaSet.insert(subschema).second) {
                throw std::runtime_error(
                        "Failed to store pointer for new sub-schema");
            }
        } catch (...) {
            subschema->~Subschema();
            // TODO: Replace with custom free function
            ::operator delete(ptr);
            throw;
        }

        return subschema;
    }

    /**
     * @brief  Get a pointer to the root sub-schema of this Schema instance
     */
    const Subschema * root() const
    {
        return this;
    }

private:

    /// Set of Subschema instances owned by this schema
    std::set<Subschema*> subschemaSet;
};

} // namespace valijson

#endif
