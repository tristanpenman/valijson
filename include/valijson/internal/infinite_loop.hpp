#pragma once

#include <map>
#include <utility>
#include <vector>

#include <valijson/constraints/concrete_constraints.hpp>
#include <valijson/constraints/constraint_visitor.hpp>
#include <valijson/subschema.hpp>

namespace valijson {
namespace internal {

/**
 * @brief  Collects the subschemas applied by constraints
 *
 * Subschemas are separated into those that are applied to the same value
 * as the constraint, such as the subschemas of 'allOf' or 'not', and those
 * that are applied to other values, such as the subschemas of 'items' or
 * 'properties'.
 */
class SubschemaCollector: public constraints::ConstraintVisitor
{
    /// Functor that adds the subschema passed as its last argument, for
    /// use with the applyTo...() functions of constraints
    struct Collect
    {
        std::vector<const Subschema *> &subschemas;

        template<typename KeyType>
        bool operator()(const KeyType &, const Subschema *subschema) const
        {
            add(subschemas, subschema);
            return true;
        }
    };

public:
    /// Subschemas applied to the same value as the constraint
    std::vector<const Subschema *> sameValue;

    /// Subschemas applied to other values, such as array items
    std::vector<const Subschema *> otherValues;

    bool visit(const AllOfConstraint &constraint) override
    {
        constraint.applyToSubschemas(collect(sameValue));
        return true;
    }

    bool visit(const AnyOfConstraint &constraint) override
    {
        constraint.applyToSubschemas(collect(sameValue));
        return true;
    }

    bool visit(const ConditionalConstraint &constraint) override
    {
        add(sameValue, constraint.getIfSubschema());
        add(sameValue, constraint.getThenSubschema());
        add(sameValue, constraint.getElseSubschema());
        return true;
    }

    bool visit(const ContainsConstraint &constraint) override
    {
        add(otherValues, constraint.getSubschema());
        return true;
    }

    bool visit(const DependenciesConstraint &constraint) override
    {
        constraint.applyToSchemaDependencies(collect(sameValue));
        return true;
    }

    bool visit(const LinearItemsConstraint &constraint) override
    {
        constraint.applyToItemSubschemas(collect(otherValues));
        add(otherValues, constraint.getAdditionalItemsSubschema());
        return true;
    }

    bool visit(const NotConstraint &constraint) override
    {
        add(sameValue, constraint.getSubschema());
        return true;
    }

    bool visit(const OneOfConstraint &constraint) override
    {
        constraint.applyToSubschemas(collect(sameValue));
        return true;
    }

    bool visit(const PropertiesConstraint &constraint) override
    {
        constraint.applyToProperties(collect(otherValues));
        constraint.applyToPatternProperties(collect(otherValues));
        add(otherValues, constraint.getAdditionalPropertiesSubschema());
        return true;
    }

    bool visit(const PropertyNamesConstraint &constraint) override
    {
        add(otherValues, constraint.getSubschema());
        return true;
    }

    bool visit(const SingularItemsConstraint &constraint) override
    {
        add(otherValues, constraint.getItemsSubschema());
        return true;
    }

    bool visit(const TypeConstraint &constraint) override
    {
        constraint.applyToSchemaTypes(collect(sameValue));
        return true;
    }

    // Constraints without subschemas
    bool visit(const ConstConstraint &) override { return true; }
    bool visit(const EnumConstraint &) override { return true; }
    bool visit(const FormatConstraint &) override { return true; }
    bool visit(const MaximumConstraint &) override { return true; }
    bool visit(const MaxItemsConstraint &) override { return true; }
    bool visit(const MaxLengthConstraint &) override { return true; }
    bool visit(const MaxPropertiesConstraint &) override { return true; }
    bool visit(const MinimumConstraint &) override { return true; }
    bool visit(const MinItemsConstraint &) override { return true; }
    bool visit(const MinLengthConstraint &) override { return true; }
    bool visit(const MinPropertiesConstraint &) override { return true; }
    bool visit(const MultipleOfDoubleConstraint &) override { return true; }
    bool visit(const MultipleOfIntConstraint &) override { return true; }
    bool visit(const PatternConstraint &) override { return true; }
    bool visit(const PolyConstraint &) override { return true; }
    bool visit(const RequiredConstraint &) override { return true; }
    bool visit(const UniqueItemsConstraint &) override { return true; }

private:
    static void add(std::vector<const Subschema *> &subschemas,
            const Subschema *subschema)
    {
        if (subschema) {
            subschemas.push_back(subschema);
        }
    }

    static Collect collect(std::vector<const Subschema *> &subschemas)
    {
        return Collect{subschemas};
    }
};


/**
 * @brief  Determine whether validation against a schema might not terminate
 *
 * Validation would never terminate if a subschema, through constraints that
 * apply subschemas to the same value, such as 'allOf' or 'not', and JSON
 * References, ends up being applied to the same value again. For example, the
 * schema {"allOf": [{"$ref": "#"}]} applies itself to every value. The JSON
 * Schema specification leaves the behaviour for such schemas undefined.
 *
 * Applying a subschema to a value nested within the current value, such as
 * through 'properties', is not a problem, as the nesting of values is finite.
 *
 * @param  root  root schema to check
 *
 * @return \c true if there is a cycle of subschemas applied to the same value
 */
inline bool containsInfiniteLoop(const Subschema &root)
{
    // Find all subschemas reachable from the root, and the subschemas that
    // each of them applies to the same value
    std::map<const Subschema *, std::vector<const Subschema *>> sameValue;
    std::vector<const Subschema *> pending{&root};
    while (!pending.empty()) {
        const Subschema *subschema = pending.back();
        pending.pop_back();
        if (sameValue.count(subschema)) {
            continue;
        }

        SubschemaCollector collector;
        Subschema::ApplyFunction fn(
                [&collector](const constraints::Constraint &constraint) {
                    return constraint.accept(collector);
                });
        subschema->apply(fn);

        pending.insert(pending.end(), collector.sameValue.begin(),
                collector.sameValue.end());
        pending.insert(pending.end(), collector.otherValues.begin(),
                collector.otherValues.end());
        sameValue.emplace(subschema, std::move(collector.sameValue));
    }

    // Look for a cycle among the subschemas applied to the same value, using
    // an iterative depth-first search
    enum class State { kInProgress, kDone };
    std::map<const Subschema *, State> states;
    for (const auto &start : sameValue) {
        if (states.count(start.first)) {
            continue;
        }

        // Each entry is a subschema, and the index of the next subschema that
        // it applies to the same value
        std::vector<std::pair<const Subschema *, size_t>> stack{
                {start.first, 0}};
        states[start.first] = State::kInProgress;
        while (!stack.empty()) {
            const Subschema *subschema = stack.back().first;
            const std::vector<const Subschema *> &next = sameValue[subschema];
            if (stack.back().second == next.size()) {
                states[subschema] = State::kDone;
                stack.pop_back();
                continue;
            }

            const Subschema *nextSubschema = next[stack.back().second++];
            const auto itr = states.find(nextSubschema);
            if (itr == states.end()) {
                states[nextSubschema] = State::kInProgress;
                stack.emplace_back(nextSubschema, 0);
            } else if (itr->second == State::kInProgress) {
                return true;
            }
        }
    }

    return false;
}

} // namespace internal
} // namespace valijson
