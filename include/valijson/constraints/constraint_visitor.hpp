#pragma once
#ifndef __VALIJSON_CONSTRAINTS_CONSTRAINT_VISITOR_HPP
#define __VALIJSON_CONSTRAINTS_CONSTRAINT_VISITOR_HPP

namespace valijson {
namespace constraints {

struct AllOfConstraint;
struct AnyOfConstraint;
struct DependenciesConstraint;
struct EnumConstraint;
struct ItemsConstraint;
struct FormatConstraint;
struct MaximumConstraint;
struct MaxItemsConstraint;
struct MaxLengthConstraint;
struct MaxPropertiesConstraint;
struct MinimumConstraint;
struct MinItemsConstraint;
struct MinLengthConstraint;
struct MinPropertiesConstraint;
struct MultipleOfConstraint;
struct NotConstraint;
struct OneOfConstraint;
struct PatternConstraint;
struct PropertiesConstraint;
struct RequiredConstraint;
struct TypeConstraint;
struct UniqueItemsConstraint;

/// Interface to allow usage of the visitor pattern with Constraints
class ConstraintVisitor
{
protected:

    // Shorten type names for derived classes outside of this namespace
    typedef constraints::AllOfConstraint AllOfConstraint;
    typedef constraints::AnyOfConstraint AnyOfConstraint;
    typedef constraints::DependenciesConstraint DependenciesConstraint;
    typedef constraints::EnumConstraint EnumConstraint;
    typedef constraints::ItemsConstraint ItemsConstraint;
    typedef constraints::MaximumConstraint MaximumConstraint;
    typedef constraints::MaxItemsConstraint MaxItemsConstraint;
    typedef constraints::MaxLengthConstraint MaxLengthConstraint;
    typedef constraints::MaxPropertiesConstraint MaxPropertiesConstraint;
    typedef constraints::MinimumConstraint MinimumConstraint;
    typedef constraints::MinItemsConstraint MinItemsConstraint;
    typedef constraints::MinLengthConstraint MinLengthConstraint;
    typedef constraints::MinPropertiesConstraint MinPropertiesConstraint;
    typedef constraints::MultipleOfConstraint MultipleOfConstraint;
    typedef constraints::NotConstraint NotConstraint;
    typedef constraints::OneOfConstraint OneOfConstraint;
    typedef constraints::PatternConstraint PatternConstraint;
    typedef constraints::PropertiesConstraint PropertiesConstraint;
    typedef constraints::RequiredConstraint RequiredConstraint;
    typedef constraints::TypeConstraint TypeConstraint;
    typedef constraints::UniqueItemsConstraint UniqueItemsConstraint;

public:

    virtual bool visit(const AllOfConstraint &) = 0;
    virtual bool visit(const AnyOfConstraint &) = 0;
    virtual bool visit(const DependenciesConstraint &) = 0;
    virtual bool visit(const EnumConstraint &) = 0;
    virtual bool visit(const ItemsConstraint &) = 0;
    virtual bool visit(const MaximumConstraint &) = 0;
    virtual bool visit(const MaxItemsConstraint &) = 0;
    virtual bool visit(const MaxLengthConstraint &) = 0;
    virtual bool visit(const MaxPropertiesConstraint &) = 0;
    virtual bool visit(const MinimumConstraint &) = 0;
    virtual bool visit(const MinItemsConstraint &) = 0;
    virtual bool visit(const MinLengthConstraint &) = 0;
    virtual bool visit(const MinPropertiesConstraint &) = 0;
    virtual bool visit(const MultipleOfConstraint &) = 0;
    virtual bool visit(const NotConstraint &) = 0;
    virtual bool visit(const OneOfConstraint &) = 0;
    virtual bool visit(const PatternConstraint &) = 0;
    virtual bool visit(const PropertiesConstraint &) = 0;
    virtual bool visit(const RequiredConstraint &) = 0;
    virtual bool visit(const TypeConstraint &) = 0;
    virtual bool visit(const UniqueItemsConstraint &) = 0;

};

}  // namespace constraints
}  // namespace valijson

#endif
