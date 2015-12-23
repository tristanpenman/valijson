#pragma once
#ifndef __VALIJSON_CONSTRAINTS_BASIC_CONSTRAINT_HPP
#define __VALIJSON_CONSTRAINTS_BASIC_CONSTRAINT_HPP

#include <valijson/constraints/constraint.hpp>
#include <valijson/constraints/constraint_visitor.hpp>

namespace valijson {
namespace constraints {

/**
 * @brief   Template class that implements the accept() and clone() functions of
 *          the Constraint interface.
 *
 * @tparam  ConstraintType   name of the concrete constraint type, which must
 *                           provide a copy constructor.
 */
template<typename ConstraintType>
struct BasicConstraint: Constraint
{
    virtual ~BasicConstraint<ConstraintType>() { }

    virtual bool accept(ConstraintVisitor &visitor) const
    {
        return visitor.visit(*static_cast<const ConstraintType*>(this));
    }

    virtual Constraint * clone(CustomAlloc allocFn, CustomFree freeFn) const
    {
        void *ptr = allocFn(sizeof(ConstraintType));
        if (!ptr) {
            throw std::runtime_error(
                    "Failed to allocate memory for cloned constraint");
        }

        try {
            return new (ptr) ConstraintType(
                    *static_cast<const ConstraintType*>(this));
        } catch (...) {
            freeFn(ptr);
            throw;
        }
    }
};

} // namespace constraints
} // namespace valijson

#endif
