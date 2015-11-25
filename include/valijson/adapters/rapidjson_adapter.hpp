/**
 * @file
 *
 * @brief   Adapter implementation for the RapidJson parser library.
 *
 * Include this file in your program to enable support for RapidJson.
 *
 * This file defines the following classes (not in this order):
 *  - RapidJsonAdapter
 *  - RapidJsonArray
 *  - RapidJsonArrayValueIterator
 *  - RapidJsonFrozenValue
 *  - RapidJsonObject
 *  - RapidJsonObjectMember
 *  - RapidJsonObjectMemberIterator
 *  - RapidJsonValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is RapidJsonAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#ifndef __VALIJSON_ADAPTERS_RAPIDJSON_ADAPTER_HPP
#define __VALIJSON_ADAPTERS_RAPIDJSON_ADAPTER_HPP

#include <string>
#include <boost/bind.hpp>
#include <boost/optional.hpp>
#include <boost/iterator/iterator_facade.hpp>

#include <rapidjson/document.h>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/basic_adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>

namespace valijson {
namespace adapters {

template<class ValueType = rapidjson::Value> class RapidJsonAdapter;
template<class ValueType = rapidjson::Value> class RapidJsonArrayValueIterator;
template<class ValueType = rapidjson::Value> class RapidJsonObjectMemberIterator;

template<class ValueType = rapidjson::Value>
class RapidJsonObjectMember : public std::pair<std::string, RapidJsonAdapter<ValueType> >
{ };

/**
 * @brief   Light weight wrapper for a RapidJson array value.
 *
 * This class is light weight wrapper for a RapidJson array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to an underlying
 * RapidJson value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
template<class ValueType = rapidjson::Value>
class RapidJsonArray
{
public:

    typedef RapidJsonArrayValueIterator<ValueType> const_iterator;
    typedef RapidJsonArrayValueIterator<ValueType> iterator;

    /// Construct a RapidJsonArray referencing an empty array singleton.
    RapidJsonArray()
      : value(emptyArray()) { }

    /**
     * @brief   Construct a RapidJsonArray referencing a specific RapidJson
     *          value.
     *
     * @param   value   reference to a RapidJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    RapidJsonArray(const ValueType &value)
      : value(value)
    {
        if (!value.IsArray()) {
            throw std::runtime_error("Value is not an array.");
        }
    }

    /// Return an iterator for the first element in the array.
    iterator begin() const;

    /// Return an iterator for one-past the last element of the array.
    iterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return value.Size();
    }

private:

    /**
     * @brief   Return a reference to a RapidJson value that is an empty array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const ValueType & emptyArray()
    {
        static const ValueType array(rapidjson::kArrayType);
        return array;
    }

    /// Reference to the contained value
    const ValueType &value;
};

/**
 * @brief  Light weight wrapper for a RapidJson object.
 *
 * This class is light weight wrapper for a RapidJson object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * RapidJson value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
template <class ValueType = rapidjson::Value>
class RapidJsonObject
{
public:

    typedef RapidJsonObjectMemberIterator<ValueType> const_iterator;
    typedef RapidJsonObjectMemberIterator<ValueType> iterator;

    /// Construct a RapidJsonObject referencing an empty object singleton.
    RapidJsonObject()
      : value(emptyObject()) { }

    /**
     * @brief   Construct a RapidJsonObject referencing a specific RapidJson
     *          value.
     *
     * @param   value  reference to a RapidJson value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    RapidJsonObject(const ValueType &value)
      : value(value)
    {
        if (!value.IsObject()) {
            throw std::runtime_error("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the pointer value returned by the underlying RapidJson implementation.
     */
    iterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the pointer value returned by the underlying RapidJson implementation.
     */
    iterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   property   property name to search for
     */
    iterator find(const std::string &property) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return value.MemberEnd() - value.MemberBegin();
    }

private:

    /**
     * @brief   Return a reference to a RapidJson value that is empty object.
     *
     * Note that the value returned by this function is a singleton.
     */
    static const ValueType & emptyObject()
    {
        static ValueType object(rapidjson::kObjectType);
        return object;
    }

    /// Reference to the contained object
    const ValueType &value;
};

/**
 * @brief   Stores an independent copy of a RapidJson value.
 *
 * This class allows a RapidJson value to be stored independent of its original
 * document. RapidJson makes this a bit harder than usual, because RapidJson
 * values are associated with a custom memory allocator. As such, RapidJson
 * values have to be copied recursively, referencing a custom allocator held
 * by this class.
 *
 * @see FrozenValue
 */
template<class ValueType = rapidjson::Value>
class RapidJsonFrozenValue: public FrozenValue
{
public:

    RapidJsonFrozenValue(const char *str)
    {
        value.SetString(str, allocator);
    }

    RapidJsonFrozenValue(const std::string &str)
    {
        value.SetString(str.c_str(), (unsigned int)str.length(), allocator);
    }

    /**
     * @brief   Make a copy of a RapidJson value
     *
     * @param   source  the RapidJson value to be copied
     */
    explicit RapidJsonFrozenValue(const ValueType &source)
    {
        if (!copy(source, value, allocator)) {
            throw std::runtime_error("Failed to copy ValueType");
        }
    }

    virtual FrozenValue * clone() const
    {
        return new RapidJsonFrozenValue(value);
    }

    virtual bool equalTo(const Adapter &other, bool strict) const;

private:

    /**
     * @brief   Recursively copy a RapidJson value using a separate allocator
     *
     * @param   source      value to copy from
     * @param   dest        value to copy into
     * @param   allocator   reference to an allocator held by this class
     *
     * @tparam  Allocator   type of RapidJson Allocator to be used
     *
     * @returns true if copied successfully, false otherwise.
     */
    template<typename Allocator>
    static bool copy(const ValueType &source,
                     ValueType &dest,
                     Allocator &allocator)
    {
        switch (source.GetType()) {
        case rapidjson::kNullType:
            dest.SetNull();
            return true;
        case rapidjson::kFalseType:
            dest.SetBool(false);
            return true;
        case rapidjson::kTrueType:
            dest.SetBool(true);
            return true;
        case rapidjson::kObjectType:
            dest.SetObject();
            for (typename ValueType::ConstMemberIterator itr = source.MemberBegin();
                itr != source.MemberEnd(); ++itr) {
                ValueType name(itr->name.GetString(), itr->name.GetStringLength(), allocator);
                ValueType value;
                copy(itr->value, value, allocator);
                dest.AddMember(name, value, allocator);
            }
            return true;
        case rapidjson::kArrayType:
            dest.SetArray();
            for (typename ValueType::ConstValueIterator itr = source.Begin(); itr != source.End(); ++itr) {
                ValueType value;
                copy(*itr, value, allocator);
                dest.PushBack(value, allocator);
            }
            return true;
        case rapidjson::kStringType:
            dest.SetString(source.GetString(), source.GetStringLength(), allocator);
            return true;
        case rapidjson::kNumberType:
            if (source.IsInt()) {
                dest.SetInt(source.GetInt());
            } else if (source.IsUint()) {
                dest.SetUint(source.GetUint());
            } else if (source.IsInt64()) {
                dest.SetInt64(source.GetInt64());
            } else if (source.IsUint64()) {
                dest.SetUint64(source.GetUint64());
            } else {
                dest.SetDouble(source.GetDouble());
            }
            return true;
        default:
            break;
        }

        return false;
    }

    /// Local memory allocator for RapidJson value
    typename ValueType::AllocatorType allocator;

    /// Local RapidJson value
    ValueType value;
};

/**
 * @brief   Light weight wrapper for a RapidJson value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a RapidJson value. This class is responsible
 * for the mechanics of actually reading a RapidJson value, whereas the
 * BasicAdapter class is responsible for the semantics of type comparisons
 * and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
template<class ValueType = rapidjson::Value>
class RapidJsonValue
{
public:

    /// Construct a wrapper for the empty object singleton
    RapidJsonValue()
      : value(emptyObject()) { }

    /// Construct a wrapper for a specific RapidJson value
    RapidJsonValue(const ValueType &value)
      : value(value) { }

    /**
     * @brief   Create a new RapidJsonFrozenValue instance that contains the
     *          value referenced by this RapidJsonValue instance.
     *
     * @returns pointer to a new RapidJsonFrozenValue instance, belonging to
     *          the caller.
     */
    FrozenValue * freeze() const
    {
        return new RapidJsonFrozenValue<ValueType>(value);
    }

    /**
     * @brief   Optionally return a RapidJsonArray instance.
     *
     * If the referenced RapidJson value is an array, this function will return
     * a boost::optional containing a RapidJsonArray instance referencing the
     * array.
     *
     * Otherwise it will return boost::none.
     */
    boost::optional<RapidJsonArray<ValueType> > getArrayOptional() const
    {
        if (value.IsArray()) {
            return boost::make_optional(RapidJsonArray<ValueType>(value));
        }

        return boost::none;
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced RapidJson value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (value.IsArray()) {
            result = value.Size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        if (value.IsBool()) {
            result = value.GetBool();
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        if (value.IsDouble()) {
            result = value.GetDouble();
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        if (value.IsInt()) {
            result = value.GetInt();
            return true;
        } else if (value.IsInt64()) {
            result = value.GetInt64();
            return true;
        } else if (value.IsUint()) {
            result = static_cast<int64_t>(value.GetUint());
            return true;
        } else if (value.IsUint64()) {
            result = static_cast<int64_t>(value.GetUint64());
            return true;
        }

        return false;
    }

    /**
     * @brief   Optionally return a RapidJsonObject instance.
     *
     * If the referenced RapidJson value is an object, this function will return
     * a boost::optional containing a RapidJsonObject instance referencing the
     * object.
     *
     * Otherwise it will return boost::none.
     */
    boost::optional<RapidJsonObject<ValueType> > getObjectOptional() const
    {
        if (value.IsObject()) {
            return boost::make_optional(RapidJsonObject<ValueType>(value));
        }

        return boost::none;
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced RapidJson value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (value.IsObject()) {
            result = value.MemberEnd() - value.MemberBegin();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (value.IsString()) {
            result.assign(value.GetString(), value.GetStringLength());
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
        return value.IsArray();
    }

    bool isBool() const
    {
        return value.IsBool();
    }

    bool isDouble() const
    {
        return value.IsDouble();
    }

    bool isInteger() const
    {
        return value.IsInt() || value.IsInt64() || value.IsUint() ||
               value.IsUint64();
    }

    bool isNull() const
    {
        return value.IsNull();
    }

    bool isNumber() const
    {
        return value.IsNumber();
    }

    bool isObject() const
    {
        return value.IsObject();
    }

    bool isString() const
    {
        return value.IsString();
    }

private:

    /// Return a reference to an empty object singleton
    static const ValueType & emptyObject()
    {
        static const ValueType object(rapidjson::kObjectType);
        return object;
    }

    /// Reference to the contained RapidJson value.
    const ValueType &value;
};

/**
 * @brief   An implementation of the Adapter interface supporting RapidJson.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
template<class ValueType>
class RapidJsonAdapter:
    public BasicAdapter<RapidJsonAdapter<ValueType>,
                        RapidJsonArray<ValueType>,
                        RapidJsonObjectMember<ValueType>,
                        RapidJsonObject<ValueType>,
                        RapidJsonValue<ValueType> >
{
public:

    /// Construct a RapidJsonAdapter that contains an empty object
    RapidJsonAdapter()
      : BasicAdapter<RapidJsonAdapter<ValueType>,
                        RapidJsonArray<ValueType>,
                        RapidJsonObjectMember<ValueType>,
                        RapidJsonObject<ValueType>,
                        RapidJsonValue<ValueType> >() { }

    /// Construct a RapidJsonAdapter containing a specific RapidJson value
    RapidJsonAdapter(const ValueType &value)
      : BasicAdapter<RapidJsonAdapter<ValueType>,
                        RapidJsonArray<ValueType>,
                        RapidJsonObjectMember<ValueType>,
                        RapidJsonObject<ValueType>,
                        RapidJsonValue<ValueType> >(value) { }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * RapidJsonAdapter representing a value stored in the array. It has been
 * implemented using the boost iterator_facade template.
 *
 * @see RapidJsonArray
 */
template<class ValueType>
class RapidJsonArrayValueIterator:
    public boost::iterator_facade<
        RapidJsonArrayValueIterator<ValueType>, // name of derived type
        RapidJsonAdapter<ValueType>,            // value type
        boost::bidirectional_traversal_tag,     // bi-directional iterator
        RapidJsonAdapter<ValueType> >           // type returned when dereferenced
{
public:

    /**
     * @brief   Construct a new RapidJsonArrayValueIterator using an existing
     *          RapidJson iterator.
     *
     * @param   itr  RapidJson iterator to store
     */
    RapidJsonArrayValueIterator(
        const typename ValueType::ConstValueIterator &itr)
      : itr(itr) { }

    /// Returns a RapidJsonAdapter that contains the value of the current
    /// element.
    RapidJsonAdapter<ValueType> dereference() const
    {
        return RapidJsonAdapter<ValueType>(*itr);
    }

    /**
     * @brief   Compare this iterator against another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the same
     * underlying object.
     *
     * @param   other  iterator to compare against
     *
     * @returns true if the iterators are equal, false otherwise.
     */
    bool equal(const RapidJsonArrayValueIterator &other) const
    {
        return itr == other.itr;
    }

    void increment()
    {
        itr++;
    }

    void decrement()
    {
        itr--;
    }

    void advance(std::ptrdiff_t n)
    {
        itr += n;
    }

    std::ptrdiff_t difference(const RapidJsonArrayValueIterator &other)
    {
        return std::distance(itr, other.itr);
    }

private:

    typename ValueType::ConstValueIterator itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of RapidJsonObjectMember representing one of the members of the object. It
 * has been implemented using the boost iterator_facade template.
 *
 * @see RapidJsonObject
 * @see RapidJsonObjectMember
 */
template<class ValueType>
class RapidJsonObjectMemberIterator:
    public boost::iterator_facade<
        RapidJsonObjectMemberIterator<ValueType>, // name of derived type
        RapidJsonObjectMember<ValueType>,         // value type
        boost::bidirectional_traversal_tag,       // bi-directional iterator
        RapidJsonObjectMember<ValueType> >        // type returned when dereferenced
{
public:

    /**
     * @brief   Construct an iterator from a RapidJson iterator.
     *
     * @param   itr  RapidJson iterator to store
     */
    RapidJsonObjectMemberIterator(
        const typename ValueType::ConstMemberIterator &itr)
      : itr(itr) { }

    /**
     * @brief   Returns a RapidJsonObjectMember that contains the key and value
     *          belonging to the object member identified by the iterator.
     */
    RapidJsonObjectMember<ValueType> dereference() const
    {
        return RapidJsonObjectMember<ValueType>(
            std::string(itr->name.GetString(), itr->name.GetStringLength()),
            itr->value);
    }

    /**
     * @brief   Compare this iterator with another iterator.
     *
     * Note that this directly compares the iterators, not the underlying
     * values, and assumes that two identical iterators will point to the same
     * underlying object.
     *
     * @param   other  Iterator to compare with
     *
     * @returns true if the underlying iterators are equal, false otherwise
     */
    bool equal(const RapidJsonObjectMemberIterator &other) const
    {
        return itr == other.itr;
    }

    void increment()
    {
        itr++;
    }

    void decrement()
    {
        itr--;
    }

    std::ptrdiff_t difference(const RapidJsonObjectMemberIterator &other)
    {
        return std::distance(itr, other.itr);
    }

private:

    /// Iternal copy of the original RapidJson iterator
    typename ValueType::ConstMemberIterator itr;
};

/// RapidJson specialisation of the AdapterTraits template struct.
template<>
struct AdapterTraits<valijson::adapters::RapidJsonAdapter<rapidjson::Value> >
{
    typedef rapidjson::Document DocumentType;

    static std::string adapterName()
    {
        return "RapidJsonAdapter";
    }
};

template<class ValueType>
inline bool RapidJsonFrozenValue<ValueType>::equalTo(const Adapter &other, bool strict) const
{
    return RapidJsonAdapter<ValueType>(value).equalTo(other, strict);
}

template<class ValueType>
inline typename RapidJsonArray<ValueType>::iterator RapidJsonArray<ValueType>::begin() const
{
    return value.Begin();
}

template<class ValueType>
inline typename RapidJsonArray<ValueType>::iterator RapidJsonArray<ValueType>::end() const
{
    return value.End();
}

template<class ValueType>
inline typename RapidJsonObject<ValueType>::iterator RapidJsonObject<ValueType>::begin() const
{
    return value.MemberBegin();
}

template<class ValueType>
inline typename RapidJsonObject<ValueType>::iterator RapidJsonObject<ValueType>::end() const
{
    return value.MemberEnd();
}

template<class ValueType>
inline typename RapidJsonObject<ValueType>::iterator RapidJsonObject<ValueType>::find(
    const std::string &propertyName) const
{
    // Hack to support older versions of rapidjson where pointers are used as
    // the built in iterator type. In those versions, the FindMember function
    // would return a null pointer when the requested member could not be
    // found. After calling FindMember on an empty object, we compare the
    // result against what we would expect if a non-null-pointer iterator was
    // returned.
    const ValueType empty(rapidjson::kObjectType);
    const typename ValueType::ConstMemberIterator maybeEnd = empty.FindMember("");
    if (maybeEnd != empty.MemberBegin() + 1) {
        // In addition to the pointer-based iterator issue, RapidJson's internal
        // string comparison code seemed to rely on the query string being
        // initialised to a length greater than or equal to that of the
        // properties being compared. We get around this by implementing our
        // own linear scan.
        const size_t propertyNameLength = propertyName.length();
        for (typename ValueType::ConstMemberIterator itr = value.MemberBegin();
                itr != value.MemberEnd(); ++itr) {
            const size_t memberNameLength = itr->name.GetStringLength();
            if (memberNameLength == propertyNameLength &&
                    strncmp(itr->name.GetString(), propertyName.c_str(),
                        itr->name.GetStringLength()) == 0) {
                return itr;
            }
        }

        return value.MemberEnd();
    }

    return value.FindMember(propertyName.c_str());      // Times are good.
}

}  // namespace adapters
}  // namespace valijson

#endif

