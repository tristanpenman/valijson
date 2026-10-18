/**
 * @file
 *
 * @brief Adapter implementation for Glaze's generic JSON DOM type --
 *		  glz::generic on Glaze >= 6.0.0 (where glz::json_t survives
 *only as a deprecated alias for it), or glz::json_t on older releases that
 *predate the glz::generic refactor.
 *
 * Include this file in your program to enable support for Glaze's generic
 * JSON DOM type when validating documents with valijson.
 *
 * This file defines the following classes (not in this order):
 *  - GlazeAdapter
 *  - GlazeArray
 *  - GlazeArrayValueIterator
 *  - GlazeFrozenValue
 *  - GlazeObject
 *  - GlazeObjectMember
 *  - GlazeObjectMemberIterator
 *  - GlazeValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place
 * to start is GlazeAdapter. This class definition is actually very
 * small, since most of the functionality is inherited from the BasicAdapter
 * class. Most of the classes in this file are provided as template
 * arguments to the inherited BasicAdapter class.
 *
 * Supporting both old and new Glaze releases
 * -------------------------------------------
 * Prior to Glaze v6.0.0, the generic JSON DOM type was a concrete struct
 * named glz::json_t, declared in <glaze/json/json_t.hpp>, with:
 *
 *   using array_t  = std::vector<json_t>;
 *   using object_t = std::map<std::string, json_t, std::less<>>;
 *
 * As of v6.0.0, that type was renamed/generalised to glz::generic_json
 * (aliased as glz::generic), declared in <glaze/json/generic.hpp>, backed
 * by an insertion-order-preserving glz::ordered_small_map instead of
 * std::map, and glz::json_t became a *deprecated* alias for glz::generic
 * kept only for source compatibility.
 *
 * <glaze/json/generic.hpp> only exists on the newer releases, so its
 * presence is used below as a reliable, precise signal for which type to
 * use -- this is more accurate than sniffing for an unrelated header (e.g.
 * <glaze/yaml.hpp>, which was introduced independently, long after the
 * json_t -> generic split, and so does not track the refactor). Using
 * glz::generic directly (rather than the glz::json_t alias) on newer Glaze
 * also avoids the deprecation warning that name now carries.
 *
 * Rather than branching throughout this file, the adapter is written
 * entirely in terms of the API that both json_t and generic_json share:
 * holds<T>(), get<T>(), get_if<T>(), the array_t/object_t/null_t member
 * typedefs, and assignment from array_t/object_t. Both container types
 * (std::map and glz::ordered_small_map) also support the same begin() /
 * end() / size() / find() surface used below, and both dereference their
 * iterators to a key/value pair via ->first / ->second, so the iterator
 * wrappers need no version-specific code either.
 *
 * Numbers
 * -------
 * In the default (f64) number mode used by both glz::json_t and
 * glz::generic, every JSON number -- whether or not it has a fractional
 * part -- is stored internally as a `double`; there is no separate integer
 * storage the way there is with, say, nlohmann::json.
 *
 * To support both the JSON Schema "integer" and "number" types, this
 * adapter classifies a stored double as an integer if it has no fractional
 * component and lies within the representable range of int64_t, and as a
 * "double" otherwise. This mirrors the way JSON Schema itself defines the
 * "integer" type (a JSON number with a zero fractional part), and is
 * analogous to how valijson's adapters for other double-only-number
 * libraries (e.g. picojson) behave.
 *
 * If you are using one of Glaze's integer-precise generic aliases instead
 * (glz::generic_i64 / glz::generic_u64, or their *_sorted counterparts,
 * only available on Glaze >= 6.0.0), change the GlazeDocument alias
 * below to the one you're using; getInteger()/isInteger() can then be
 * simplified to check holds<int64_t>() / holds<uint64_t>() directly rather
 * than inferring integer-ness from a double.
 */

#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <utility>

#include <glaze/glaze.hpp>

#include <valijson/exceptions.hpp>
#include <valijson/internal/adapter.hpp>
#include <valijson/internal/basic_adapter.hpp>
#include <valijson/internal/frozen_value.hpp>
#include <valijson/schema.hpp>
#include <valijson/schema_parser.hpp>
#include <valijson/validator.hpp>

namespace valijson {
namespace adapters {

#if __has_include(<glaze/json/generic.hpp>)
using GlazeDocument = glz::generic;
#else
using GlazeDocument = glz::json_t;
#endif

using GlazeArrayT = GlazeDocument::array_t;
using GlazeObjectT = GlazeDocument::object_t;
using GlazeNullT = GlazeDocument::null_t;

class GlazeAdapter;

typedef std::pair<std::string, GlazeAdapter> GlazeObjectMember;

namespace glaze_json_detail {

/// Construct a GlazeDocument value wrapping an empty array.
inline GlazeDocument makeEmptyArray()
{
    GlazeDocument value;
    value = GlazeArrayT{};
    return value;
}

/// Construct a GlazeDocument value wrapping an empty object.
inline GlazeDocument makeEmptyObject()
{
    GlazeDocument value;
    value = GlazeObjectT{};
    return value;
}

/**
 * @brief   Determine whether a double value should be treated as an
 *		  integer for the purposes of JSON Schema validation.
 *
 * A value is considered integral if it has no fractional component and
 * falls within the representable range of int64_t.
 */
inline bool isIntegral(double d)
{
    if (!std::isfinite(d)) {
        return false;
    }

    if (d != std::trunc(d)) {
        return false;
    }

    return d >= static_cast<double>((std::numeric_limits<int64_t>::min)()) &&
           d <= static_cast<double>((std::numeric_limits<int64_t>::max)());
}

} // namespace glaze_json_detail

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an
 * instance of GlazeAdapter representing a value stored in the array.
 *
 * @see GlazeArray
 */
template <class ValueType> class GlazeArrayValueIterator
{
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = ValueType;
    using difference_type = ValueType;
    using pointer = ValueType *;
    using reference = ValueType &;

    /**
     * @brief   Construct a new GlazeArrayValueIterator using an
     *		  existing GlazeArrayT iterator.
     *
     * @param   itr  GlazeArrayT iterator to store
     */
    GlazeArrayValueIterator(const GlazeArrayT::const_iterator &itr) : m_itr(itr)
    {
    }

    /// Returns a GlazeAdapter that contains the value of the current
    /// element.
    ValueType operator*() const
    {
        return ValueType(*m_itr);
    }

    DerefProxy<ValueType> operator->() const
    {
        return DerefProxy<ValueType>(**this);
    }

    /**
     * @brief   Compare this iterator against another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the
     * same underlying object.
     *
     * @param   other  iterator to compare against
     *
     * @returns true   if the iterators are equal, false otherwise.
     */
    bool operator==(const GlazeArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const GlazeArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const GlazeArrayValueIterator &operator++()
    {
        m_itr++;

        return *this;
    }

    GlazeArrayValueIterator operator++(int)
    {
        GlazeArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const GlazeArrayValueIterator &operator--()
    {
        m_itr--;

        return *this;
    }

    void advance(std::ptrdiff_t n)
    {
        m_itr += n;
    }

  private:
    GlazeArrayT::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an
 * instance of GlazeObjectMember representing one of the members of the
 * object.
 *
 * @see GlazeObject
 * @see GlazeObjectMember
 */
template <class ValueType> class GlazeObjectMemberIterator
{
  public:
    using iterator_category = std::bidirectional_iterator_tag;
    using value_type = ValueType;
    using difference_type = ValueType;
    using pointer = ValueType *;
    using reference = ValueType &;

    /**
     * @brief   Construct an iterator from a GlazeObjectT iterator.
     *
     * @param   itr  GlazeObjectT iterator to store
     */
    GlazeObjectMemberIterator(const GlazeObjectT::const_iterator &itr)
        : m_itr(itr)
    {
    }

    /**
     * @brief   Returns a GlazeObjectMember that contains the key and
     * value belonging to the object member identified by the iterator.
     */
    ValueType operator*() const
    {
        return ValueType(m_itr->first, m_itr->second);
    }

    DerefProxy<ValueType> operator->() const
    {
        return DerefProxy<ValueType>(**this);
    }

    /**
     * @brief   Compare this iterator with another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the
     * same underlying object.
     *
     * @param   other  Iterator to compare with
     *
     * @returns true if the underlying iterators are equal, false otherwise
     */
    bool operator==(const GlazeObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const GlazeObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const GlazeObjectMemberIterator &operator++()
    {
        m_itr++;

        return *this;
    }

    GlazeObjectMemberIterator operator++(int)
    {
        GlazeObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    const GlazeObjectMemberIterator &operator--()
    {
        m_itr--;

        return *this;
    }

  private:
    /// Internal copy of the original GlazeObjectT iterator
    GlazeObjectT::const_iterator m_itr;
};

/**
 * @brief  Light weight wrapper for a Glaze JSON array value.
 *
 * This class is light weight wrapper for a Glaze JSON array. It provides a
 * minimum set of container functions and typedefs that allow it to be used
 * as an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * GlazeDocument value, assumed to be an array, so there is very little
 * overhead associated with copy construction and passing by value.
 */
template <class ValueType> class GlazeArray
{
  public:
    typedef GlazeArrayValueIterator<ValueType> const_iterator;
    typedef GlazeArrayValueIterator<ValueType> iterator;

    /// Construct a GlazeArray referencing an empty array.
    GlazeArray() : m_value(emptyArray) {}

    /**
     * @brief   Construct a GlazeArray referencing a specific
     *		  GlazeDocument value.
     *
     * @param   value   reference to a GlazeDocument value
     *
     * Note that this constructor will throw an exception if the value is
     * not an array.
     */
    GlazeArray(const GlazeDocument &value) : m_value(value)
    {
        if (!value.holds<GlazeArrayT>()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator returned by this function is effectively the iterator
     * returned by the underlying std::vector implementation.
     */
    GlazeArrayValueIterator<ValueType> begin() const
    {
        return m_value.get<GlazeArrayT>().begin();
    }

    /**
     * @brief   Return an iterator for one-past the last element of the
     *		  array.
     *
     * The iterator returned by this function is effectively the iterator
     * returned by the underlying std::vector implementation.
     */
    GlazeArrayValueIterator<ValueType> end() const
    {
        return m_value.get<GlazeArrayT>().end();
    }

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_value.get<GlazeArrayT>().size();
    }

  private:
    /// Shared GlazeDocument instance used to represent an empty array.
    inline static const GlazeDocument emptyArray =
        glaze_json_detail::makeEmptyArray();

    /// Reference to the contained value
    const GlazeDocument &m_value;
};

/**
 * @brief  Light weight wrapper for a Glaze JSON object.
 *
 * This class is light weight wrapper for a Glaze JSON object. It provides a
 * minimum set of container functions and typedefs that allow it to be used
 * as an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * GlazeDocument value, assumed to be an object, so there is very little
 * overhead associated with copy construction and passing by value.
 */
class GlazeObject
{
  public:
    typedef GlazeObjectMemberIterator<GlazeObjectMember> const_iterator;
    typedef GlazeObjectMemberIterator<GlazeObjectMember> iterator;

    /// Construct a GlazeObject referencing an empty object singleton.
    GlazeObject() : m_value(emptyObject) {}

    /**
     * @brief   Construct a GlazeObject referencing a specific
     *		  GlazeDocument value.
     *
     * @param   value  reference to a GlazeDocument value
     *
     * Note that this constructor will throw an exception if the value is
     * not an object.
     */
    GlazeObject(const GlazeDocument &value) : m_value(value)
    {
        if (!value.holds<GlazeObjectT>()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator returned by this function is effectively a wrapper
     * around the iterator value returned by the underlying
     * glz::ordered_small_map implementation.
     */
    GlazeObjectMemberIterator<GlazeObjectMember> begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that
     *		  indicates the end of the collection.
     *
     * The iterator returned by this function is effectively a wrapper
     * around the iterator value returned by the underlying
     * glz::ordered_small_map implementation.
     */
    GlazeObjectMemberIterator<GlazeObjectMember> end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *		  property name.
     *
     * If an object member with the specified name does not exist, the
     * iterator returned will be the same as the iterator returned by the
     * end() function.
     *
     * @param   propertyName  property name to search for
     */
    GlazeObjectMemberIterator<GlazeObjectMember>
    find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_value.get<GlazeObjectT>().size();
    }

  private:
    /// Shared GlazeDocument instance used to represent an empty object.
    inline static const GlazeDocument emptyObject =
        glaze_json_detail::makeEmptyObject();

    /// Reference to the contained object
    const GlazeDocument &m_value;
};

/**
 * @brief   Stores an independent copy of a Glaze JSON value.
 *
 * This class allows a GlazeDocument value to be stored independent of its
 * original document. GlazeDocument makes this easy to do, as it does not
 * perform any custom memory management.
 *
 * @see FrozenValue
 */
class GlazeFrozenValue : public FrozenValue
{
  public:
    /**
     * @brief  Make a copy of a GlazeDocument value
     *
     * @param  source  the GlazeDocument value to be copied
     */
    explicit GlazeFrozenValue(GlazeDocument source) : m_value(std::move(source))
    {
    }

    FrozenValue *clone() const override
    {
        return new GlazeFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

  private:
    /// Stored GlazeDocument value
    GlazeDocument m_value;
};

/**
 * @brief   Light weight wrapper for a Glaze JSON value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a GlazeDocument value. This class is
 * responsible for the mechanics of actually reading a GlazeDocument value,
 * whereas the BasicAdapter class is responsible for the semantics of type
 * comparisons and conversions.
 *
 * The functions that need to be provided by this class are defined
 * implicitly by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
template <class ValueType> class GlazeValue
{
  public:
    /// Construct a wrapper for the empty object singleton
    GlazeValue() : m_value(emptyObject) {}

    /// Construct a wrapper for a specific GlazeDocument value
    GlazeValue(const GlazeDocument &value) : m_value(value) {}

    /**
     * @brief   Create a new GlazeFrozenValue instance that contains the
     *		  value referenced by this GlazeValue instance.
     *
     * @returns pointer to a new GlazeFrozenValue instance, belonging to
     *		  the caller.
     */
    FrozenValue *freeze() const
    {
        return new GlazeFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a GlazeArray instance.
     *
     * If the referenced GlazeDocument value is an array, this function will
     * return a std::optional containing a GlazeArray instance
     * referencing the array.
     *
     * Otherwise it will return an empty optional.
     */
    std::optional<GlazeArray<ValueType>> getArrayOptional() const
    {
        if (m_value.holds<GlazeArrayT>()) {
            return std::make_optional(GlazeArray<ValueType>(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced GlazeDocument value is an array, this function will
     * retrieve the number of elements in the array and store it in the
     * output variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false
     *		  otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.holds<GlazeArrayT>()) {
            result = m_value.get<GlazeArrayT>().size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (m_value.holds<bool>()) {
            result = m_value.get<bool>();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (!m_value.holds<double>()) {
            return false;
        }

        const double d = m_value.get<double>();

        // Numbers that have no fractional component are exposed via
        // getInteger() instead -- see the notes at the top of this file
        // for why GlazeDocument requires this distinction to be
        // synthesised rather than read directly from the stored type.
        if (!std::isfinite(d) || glaze_json_detail::isIntegral(d)) {
            return false;
        }

        result = d;
        return true;
    }

    bool getInteger(int64_t &result) const
    {
        if (!m_value.holds<double>()) {
            return false;
        }

        const double d = m_value.get<double>();
        if (!glaze_json_detail::isIntegral(d)) {
            return false;
        }

        result = static_cast<int64_t>(d);
        return true;
    }

    /**
     * @brief   Optionally return a GlazeObject instance.
     *
     * If the referenced GlazeDocument value is an object, this function will
     * return a std::optional containing a GlazeObject instance
     * referencing the object.
     *
     * Otherwise it will return an empty optional.
     */
    std::optional<GlazeObject> getObjectOptional() const;

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced GlazeDocument value is an object, this function will
     * retrieve the number of members in the object and store it in the
     * output variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false
     *		  otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.holds<GlazeObjectT>()) {
            result = m_value.get<GlazeObjectT>().size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (m_value.holds<std::string>()) {
            result = m_value.get<std::string>();
            return true;
        }

        return false;
    }

    static bool hasStrictTypes()
    {
        return true;
    }

    bool isArray() const
    {
        return m_value.holds<GlazeArrayT>();
    }

    bool isBool() const
    {
        return m_value.holds<bool>();
    }

    bool isDouble() const
    {
        if (!m_value.holds<double>()) {
            return false;
        }

        const double d = m_value.get<double>();
        return std::isfinite(d) && !glaze_json_detail::isIntegral(d);
    }

    bool isInteger() const
    {
        if (!m_value.holds<double>()) {
            return false;
        }

        return glaze_json_detail::isIntegral(m_value.get<double>());
    }

    bool isNull() const
    {
        if (m_value.holds<GlazeNullT>()) {
            return true;
        }

        // NaN and Infinity cannot be represented in JSON; treat any such
        // value stored in the variant as null, mirroring the way other
        // valijson adapters handle non-finite doubles.
        if (m_value.holds<double>()) {
            return !std::isfinite(m_value.get<double>());
        }

        return false;
    }

    bool isNumber() const
    {
        if (!m_value.holds<double>()) {
            return false;
        }

        return std::isfinite(m_value.get<double>());
    }

    bool isObject() const
    {
        return m_value.holds<GlazeObjectT>();
    }

    bool isString() const
    {
        return m_value.holds<std::string>();
    }

  private:
    /// Shared GlazeDocument instance used to represent an empty object
    inline static const GlazeDocument emptyObject =
        glaze_json_detail::makeEmptyObject();

    /// Reference to the contained GlazeDocument value.
    const GlazeDocument &m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting
 *		  GlazeDocument (a.k.a. glz::json_t).
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave
 * consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class GlazeAdapter : public BasicAdapter<GlazeAdapter, GlazeArray<GlazeAdapter>,
                                         GlazeObjectMember, GlazeObject,
                                         GlazeValue<GlazeAdapter>>
{
  public:
    /// Construct a GlazeAdapter that contains an empty object
    GlazeAdapter() : BasicAdapter() {}

    /// Construct a GlazeAdapter containing a specific GlazeDocument
    /// value (this also accepts glz::json_t, which is an alias for the
    /// same type)
    GlazeAdapter(const GlazeDocument &value)
        : BasicAdapter(GlazeValue<GlazeAdapter>{value})
    {
    }
};

template <class ValueType>
std::optional<GlazeObject> GlazeValue<ValueType>::getObjectOptional() const
{
    if (m_value.holds<GlazeObjectT>()) {
        return std::make_optional(GlazeObject(m_value));
    }

    return {};
}

/// Specialisation of the AdapterTraits template struct for GlazeAdapter.
template <> struct AdapterTraits<valijson::adapters::GlazeAdapter>
{
    typedef GlazeDocument DocumentType;

    static std::string adapterName()
    {
        return "GlazeAdapter";
    }
};

inline bool GlazeFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return GlazeAdapter(m_value).equalTo(other, strict);
}

inline GlazeObjectMemberIterator<GlazeObjectMember> GlazeObject::begin() const
{
    return m_value.get<GlazeObjectT>().begin();
}

inline GlazeObjectMemberIterator<GlazeObjectMember> GlazeObject::end() const
{
    return m_value.get<GlazeObjectT>().end();
}

inline GlazeObjectMemberIterator<GlazeObjectMember>
GlazeObject::find(const std::string &propertyName) const
{
    return m_value.get<GlazeObjectT>().find(propertyName);
}

} // namespace adapters
} // namespace valijson