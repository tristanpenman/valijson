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

    virtual Constraint * clone() const
    {
        return new ConstraintType(*static_cast<const ConstraintType*>(this));
    }
};

} // namespace constraints
} // namespace valijson

#endif
