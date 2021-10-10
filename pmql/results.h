#pragma once

#include "bitmap.h"
#include "ops.h"


namespace pmql::op {


/// Storage for intermediate operation evaluation results.
/// Contains extra information that indicates whether a result is up-to-date or must be re-evaluated.
/// The idea behind caching is to save time when invoking the same expression on a sequence of variables.
/// Results are:
/// * Marked as up-to-date on assignment (if caching is enabled).
/// * Invalidated when a variable substitution changes.
/// @tparam Store type that can store a calculation result (see Store contract).
template<typename Store>
class Results
{
    /// Initial operation result value that indicates that a calculation was not performed yet.
    static const Result<Store> NOT_READY;

    /// Defines collection for storing operation results.
    using Outcomes = std::vector<Result<Store>>;

    /// Describes what operations must be invalidated when each variable is changed (AND mask).
    const Bitmap::List d_invalidations;

    /// If set to true, operation result validity is tracked.
    /// If set to false, always indicates that an operation must be re-evaluated.
    const bool d_cache;

    /// Operation result validity map.
    Bitmap d_valid;

    /// Operation results storage.
    Outcomes d_outcomes;

public:
    /// Operation result reference that tracks its validity.
    class Handle;

    /// Defines value type for iteration.
    using value_type = typename Outcomes::value_type;

    /// Defines value iterator type.
    using const_iterator = typename Outcomes::const_iterator;

    /// Construct operation result container.
    /// @param ops valid list of operations.
    /// @param cache if set to true, result validity tracking is enabled.
    explicit Results(const List &ops, bool cache = true);

    /// Construct begin iterator.
    /// @return begin iterator.
    const_iterator begin() const;

    /// Construct end iterator.
    /// @return end iterator.
    const_iterator end() const;

    /// Construct a reference to an operation result.
    /// @return operation result accessor.
    Handle operator[](Id op);

    /// Marks all operations, that depend on a variable, as outdated.
    /// Does nothing if validity tracking is disabled.
    /// @param var variable index (not an operation identifier).
    void invalidate(Id var);

    /// Checks if an operation result is up to date.
    /// @param op operation identifier.
    /// @return true if validity tracking is enabled and the result is up to date.
    bool valid(Id op) const;
};

template<typename Store>
/* static */ const Result<Store> Results<Store>::NOT_READY = error<err::Kind::EXPR_NOT_READY>();


/// Single operation result reference that supports validity tracking.
/// @tparam Store type that can store a calculation result (see Store contract).
template<typename Store>
class Results<Store>::Handle
{
    /// Parent operation result collection.
    Results<Store> &d_owner;

    /// Operation identifier.
    const Id d_op;

public:
    /// Construct operation result reference.
    /// @param owner parent result collection reference.
    /// @param op operation identifier.
    Handle(Results<Store> &owner, Id op);

    /// Set operation result and mark it as valid in the parent result collection.
    /// @param result new operation result value.
    /// @return reference to self.
    Handle &operator=(Result<Store> &&result);

    /// Check if operation result is up to date.
    /// @return true if operation result is up to date and can be used without re-evaluation.
    operator bool() const;

    /// Get stored operation result value.
    /// If validity tracking is enabled and the result is out of date, error object is returned.
    /// @return stored operation result reference.
    operator const Result<Store> &() const;
};


/// Contains utils for calculating validity maps for pmql expressions.
namespace bitmap {


/// Mark all operation that depend on a particular variable for given operation list.
/// @param ops valid operation list.
/// @param op identifier of the operation to process recursively.
/// @param var variable's operation identifier.
/// @param bitmap operation bitmap to populate (must be pre-filled with 0s).
/// @return true if current operation tree includes the variable.
inline bool relations(const List &ops, Id op, Id var, Bitmap &bitmap)
{
    if (bitmap[op])
    {
        return true;
    }

    return std::visit(
        [&ops, id = op, var, &bitmap] (const auto &op) mutable -> bool
        {
            using Op = std::decay_t<decltype(op)>;
            if constexpr (std::is_same_v<Var, Op>)
            {
                return bitmap[id] = id == var;
            }
            else if constexpr (std::is_same_v<Const, Op>)
            {
                return false;
            }
            else // unary, binary, ternary, extension
            {
                op.refers([&ops, id, var, &bitmap] (auto ref)
                {
                    bitmap[id] = bitmap[id] || relations(ops, ref, var, bitmap);
                });

                return bitmap[id];
            }
        },
        ops[op]);
}

/// Construct invalidation maps for all variables in given operation list.
/// Invalidation map indicates which operations needs to be re-evaluated when a variable value changes.
/// @param ops operations list.
/// @param inverse if set to false, operations to invalidate are marked with 1s and the rest - with 0s.
/// @return list of invalidations maps for all variables, in order of definition.
inline Bitmap::List invalidations(const List &ops, bool inverse = true)
{
    Bitmap::List bitmaps;

    for (size_t id = 0; id < ops.size(); ++id)
    {
        std::visit(
            [&bitmaps, &ops, id, inverse] (const auto &op) mutable
            {
                if constexpr (std::is_same_v<Var, std::decay_t<decltype(op)>>)
                {
                    bitmaps.emplace_back(Bitmap {ops.size(), false});
                    relations(ops, ops.size() - 1, id, bitmaps.back());

                    if (inverse)
                    {
                        bitmaps.back() = ~bitmaps.back();
                    }
                }
            },
            ops[id]);
    }

    return bitmaps;
}


} // namespace bitmap


template<typename Store>
Results<Store>::Results(const op::List &ops, bool cache /*= true*/)
    : d_invalidations(bitmap::invalidations(ops))
    , d_cache(cache)
    , d_valid(ops.size(), false)
    , d_outcomes(ops.size(), NOT_READY)
{
}

template<typename Store>
typename Results<Store>::const_iterator Results<Store>::begin() const
{
    return d_outcomes.begin();
}

template<typename Store>
typename Results<Store>::const_iterator Results<Store>::end() const
{
    return d_outcomes.end();
}

template<typename Store>
typename Results<Store>::Handle Results<Store>::operator[](Id op)
{
    return {*this, op};
}

template<typename Store>
void Results<Store>::invalidate(size_t var)
{
    if (!d_cache || var >= d_invalidations.size())
    {
        return;
    }

    d_valid &= d_invalidations[var];
}

template<typename Store>
bool Results<Store>::valid(Id op) const
{
    return d_cache && d_valid[op];
}


template<typename Store>
Results<Store>::Handle::Handle(Results<Store> &owner, Id op)
    : d_owner(owner)
    , d_op(op)
{
}

template<typename Store>
typename Results<Store>::Handle &Results<Store>::Handle::operator=(Result<Store> &&result)
{
    d_owner.d_outcomes[d_op] = std::move(result);
    if (d_owner.d_cache)
    {
        d_owner.d_valid[d_op] = true;
    }

    return *this;
}

template<typename Store>
Results<Store>::Handle::operator bool() const
{
    return d_owner.valid(d_op);
}

template<typename Store>
Results<Store>::Handle::operator const Result<Store> &() const
{
    return !d_owner.d_cache || bool(*this) ? d_owner.d_outcomes[d_op] : Results<Store>::NOT_READY;
}


} // namespace pmql::op
