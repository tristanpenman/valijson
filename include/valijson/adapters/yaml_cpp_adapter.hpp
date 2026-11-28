/**
 * @file
 *
 * @brief   Adapter implementation for the yaml-cpp parser library.
 *
 * Include this file in your program to enable support for yaml-cpp.
 *
 * This file defines the following classes (not in this order):
 *  - YamlCppAdapter
 *  - YamlCppArray
 *  - YamlCppArrayValueIterator
 *  - YamlCppFrozenValue
 *  - YamlCppObject
 *  - YamlCppObjectMember
 *  - YamlCppObjectMemberIterator
 *  - YamlCppValue
 *
 * Due to the dependencies that exist between these classes, the ordering of
 * class declarations and definitions may be a bit confusing. The best place to
 * start is YamlCppAdapter. This class definition is actually very small,
 * since most of the functionality is inherited from the BasicAdapter class.
 * Most of the classes in this file are provided as template arguments to the
 * inherited BasicAdapter class.
 */

#pragma once

#include <charconv>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <yaml-cpp/yaml.h>

#include <utility>
#include <valijson/internal/adapter.hpp>
#include <valijson/internal/basic_adapter.hpp>
#include <valijson/internal/double_parser.hpp>
#include <valijson/internal/frozen_value.hpp>
#include <valijson/exceptions.hpp>

namespace valijson {
namespace adapters {

class YamlCppAdapter;
class YamlCppArrayValueIterator;
class YamlCppObjectMemberIterator;

typedef std::pair<std::string, YamlCppAdapter> YamlCppObjectMember;

/**
 * @brief  Light weight wrapper for a YamlCpp array value.
 *
 * This class is light weight wrapper for a YamlCpp array. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * YamlCpp value, assumed to be an array, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class YamlCppArray
{
  public:
    typedef YamlCppArrayValueIterator const_iterator;
    typedef YamlCppArrayValueIterator iterator;

    /// Construct a YamlCppArray referencing an empty array.
    YamlCppArray() : m_value(emptyArray) {}

    /**
     * @brief   Construct a YamlCppArray referencing a specific
     * YamlCpp value.
     *
     * @param   value   reference to a YamlCpp value
     *
     * Note that this constructor will throw an exception if the value is not
     * an array.
     */
    YamlCppArray(const YAML::Node &value) : m_value(value)
    {
        if (!value.IsSequence()) {
            throwRuntimeError("Value is not an array.");
        }
    }

    /**
     * @brief   Return an iterator for the first element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying YamlCpp implementation.
     */
    YamlCppArrayValueIterator begin() const;

    /**
     * @brief   Return an iterator for one-past the last element of the array.
     *
     * The iterator return by this function is effectively the iterator
     * returned by the underlying YamlCpp implementation.
     */
    YamlCppArrayValueIterator end() const;

    /// Return the number of elements in the array
    size_t size() const
    {
        return m_value.size();
    }

  private:
    /**
     * @brief   Return a reference to a YamlCpp value that is an empty
     * array.
     *
     * Note that the value returned by this function is a singleton.
     */
    static inline const YAML::Node emptyArray =
        YAML::Node(YAML::NodeType::Sequence);

    /// Reference to the contained value
    const YAML::Node m_value;
};

/**
 * @brief  Light weight wrapper for a YamlCpp object.
 *
 * This class is light weight wrapper for a YamlCpp object. It provides a
 * minimum set of container functions and typedefs that allow it to be used as
 * an iterable container.
 *
 * An instance of this class contains a single reference to the underlying
 * YamlCpp value, assumed to be an object, so there is very little overhead
 * associated with copy construction and passing by value.
 */
class YamlCppObject
{
  public:
    typedef YamlCppObjectMemberIterator const_iterator;
    typedef YamlCppObjectMemberIterator iterator;

    /// Construct a YamlCppObject referencing an empty object singleton.
    YamlCppObject() : m_value(emptyObject) {}

    /**
     * @brief   Construct a YamlCppObject referencing a specific
     * YamlCpp value.
     *
     * @param   value  reference to a YamlCpp value
     *
     * Note that this constructor will throw an exception if the value is not
     * an object.
     */
    YamlCppObject(const YAML::Node &value) : m_value(value)
    {
        if (!value.IsMap()) {
            throwRuntimeError("Value is not an object.");
        }
    }

    /**
     * @brief   Return an iterator for this first object member
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying YamlCpp
     * implementation.
     */
    YamlCppObjectMemberIterator begin() const;

    /**
     * @brief   Return an iterator for an invalid object member that indicates
     *          the end of the collection.
     *
     * The iterator return by this function is effectively a wrapper around
     * the iterator value returned by the underlying YamlCpp
     * implementation.
     */
    YamlCppObjectMemberIterator end() const;

    /**
     * @brief   Return an iterator for the object member with the specified
     *          property name.
     *
     * If an object member with the specified name does not exist, the iterator
     * returned will be the same as the iterator returned by the end() function.
     *
     * @param   propertyName  property name to search for
     */
    YamlCppObjectMemberIterator find(const std::string &propertyName) const;

    /// Returns the number of members belonging to this object.
    size_t size() const
    {
        return m_value.size();
    }

  private:
    /**
     * @brief   Return a reference to a YamlCpp value that is empty object.
     *
     * Note that the value returned by this function is a singleton.
     */
    static inline const YAML::Node emptyObject =
        YAML::Node(YAML::NodeType::Map);

    /// Reference to the contained object
    const YAML::Node m_value;
};

/**
 * @brief   Stores an independent copy of a YamlCpp value.
 *
 * This class allows a YamlCpp value to be stored independent of its
 * original document.
 *
 * @see FrozenValue
 */
class YamlCppFrozenValue : public FrozenValue
{
  public:
    /**
     * @brief  Make a copy of a YamlCpp value
     *
     * @param  source  the YamlCpp value to be copied
     */
    explicit YamlCppFrozenValue(YAML::Node source)
        : m_value(YAML::Clone(source))
    {
    }

    FrozenValue *clone() const override
    {
        return new YamlCppFrozenValue(m_value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

  private:
    /// Stored YamlCpp value
    YAML::Node m_value;
};

/**
 * @brief   Light weight wrapper for a YamlCpp value.
 *
 * This class is passed as an argument to the BasicAdapter template class,
 * and is used to provide access to a YamlCpp value. This class is
 * responsible for the mechanics of actually reading a YamlCpp value,
 * whereas the BasicAdapter class is responsible for the semantics of type
 * comparisons and conversions.
 *
 * The functions that need to be provided by this class are defined implicitly
 * by the implementation of the BasicAdapter template class.
 *
 * @see BasicAdapter
 */
class YamlCppValue
{
  public:
    /// Construct a wrapper for the empty object singleton
    YamlCppValue() : m_value(emptyObject) {}

    /// Construct a wrapper for a specific YamlCpp value
    YamlCppValue(const YAML::Node &value) : m_value(value) {}

    /**
     * @brief   Create a new YamlCppFrozenValue instance that contains the
     *          value referenced by this YamlCppValue instance.
     *
     * @returns pointer to a new YamlCppFrozenValue instance, belonging to
     * the caller.
     */
    FrozenValue *freeze() const
    {
        return new YamlCppFrozenValue(m_value);
    }

    /**
     * @brief   Optionally return a YamlCppArray instance.
     *
     * If the referenced YamlCpp value is an array, this function will
     * return a std::optional containing a YamlCppArray instance
     * referencing the array.
     *
     * Otherwise it will return an empty optional.
     */
    std::optional<YamlCppArray> getArrayOptional() const
    {
        if (m_value.IsSequence()) {
            return std::make_optional(YamlCppArray(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of elements in the array
     *
     * If the referenced YamlCpp value is an array, this function will
     * retrieve the number of elements in the array and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of elements was retrieved, false otherwise.
     */
    bool getArraySize(size_t &result) const
    {
        if (m_value.IsSequence()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getBool(bool &result) const
    {
        const Scalar scalar = resolveScalar();
        if (scalar.type == ScalarType::kBool) {
            result = scalar.boolValue;
            return true;
        }

        return false;
    }

    bool getDouble(double &result) const
    {
        const Scalar scalar = resolveScalar();
        if (scalar.type == ScalarType::kDouble) {
            result = scalar.doubleValue;
            return true;
        }

        return false;
    }

    bool getInteger(int64_t &result) const
    {
        const Scalar scalar = resolveScalar();
        if (scalar.type == ScalarType::kInteger) {
            result = scalar.integerValue;
            return true;
        }

        return false;
    }

    /**
     * @brief   Optionally return a YamlCppObject instance.
     *
     * If the referenced YamlCpp value is an object, this function will
     * return a std::optional containing a YamlCppObject instance
     * referencing the object.
     *
     * Otherwise it will return an empty optional.
     */
    std::optional<YamlCppObject> getObjectOptional() const
    {
        if (m_value.IsMap()) {
            return std::make_optional(YamlCppObject(m_value));
        }

        return {};
    }

    /**
     * @brief   Retrieve the number of members in the object
     *
     * If the referenced YamlCpp value is an object, this function will
     * retrieve the number of members in the object and store it in the output
     * variable provided.
     *
     * @param   result  reference to size_t to set with result
     *
     * @returns true if the number of members was retrieved, false otherwise.
     */
    bool getObjectSize(size_t &result) const
    {
        if (m_value.IsMap()) {
            result = m_value.size();
            return true;
        }

        return false;
    }

    bool getString(std::string &result) const
    {
        if (resolveScalar().type == ScalarType::kString) {
            result = m_value.Scalar();
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
        return m_value.IsSequence();
    }

    bool isBool() const
    {
        return resolveScalar().type == ScalarType::kBool;
    }

    bool isDouble() const
    {
        return resolveScalar().type == ScalarType::kDouble;
    }

    bool isInteger() const
    {
        return resolveScalar().type == ScalarType::kInteger;
    }

    bool isNull() const
    {
        return m_value.IsNull() || resolveScalar().type == ScalarType::kNull;
    }

    bool isNumber() const
    {
        const ScalarType type = resolveScalar().type;
        return type == ScalarType::kInteger || type == ScalarType::kDouble;
    }

    bool isObject() const
    {
        return m_value.IsMap();
    }

    bool isString() const
    {
        return resolveScalar().type == ScalarType::kString;
    }

  private:
    /// Type of a scalar, as resolved by the YAML 1.2 core schema
    enum class ScalarType
    {
        kNotScalar,
        kNull,
        kBool,
        kInteger,
        kDouble,
        kString
    };

    /// Scalar value, with its resolved type
    struct Scalar
    {
        ScalarType type = ScalarType::kNotScalar;
        bool boolValue = false;
        int64_t integerValue = 0;
        double doubleValue = 0;
    };

    /**
     * @brief   Resolve the type and value of the referenced scalar
     *
     * yaml-cpp keeps every scalar as a string, along with its tag. A plain
     * scalar, which has the non-specific tag '?', is resolved with the YAML
     * 1.2 core schema, so 'true', '42' and '4.2' are a boolean, an integer
     * and a number, while any other plain scalar is a string. A scalar
     * created in code, such as YAML::Node(42), has no tag, and is resolved
     * in the same way, as yaml-cpp stores it as a string too. A quoted or
     * block scalar, which has the non-specific tag '!', is always a string.
     * A scalar with a 'str', 'bool', 'int', 'float' or 'null' tag from the
     * YAML tag repository, such as '!!int 42', is resolved as that type, and
     * a scalar with any other tag is a string.
     */
    Scalar resolveScalar() const
    {
        Scalar scalar;
        if (!m_value.IsScalar()) {
            return scalar;
        }

        const std::string &tag = m_value.Tag();
        const std::string &value = m_value.Scalar();
        static const std::string tagPrefix = "tag:yaml.org,2002:";

        // parseBool(), parseInteger() and parseFloat() set the type and value
        // of the scalar when they succeed, and leave it unchanged otherwise.
        // A scalar that does not match its tag is a string.
        if (tag == "?" || tag.empty()) {
            if (isNullString(value)) {
                scalar.type = ScalarType::kNull;
            } else if (!parseBool(value, scalar) && !parseInteger(value, scalar) &&
                    !parseFloat(value, scalar)) {
                scalar.type = ScalarType::kString;
            }
        } else if (tag == tagPrefix + "null") {
            scalar.type = value.empty() || isNullString(value)
                    ? ScalarType::kNull : ScalarType::kString;
        } else if (tag == tagPrefix + "bool") {
            if (!parseBool(value, scalar)) {
                scalar.type = ScalarType::kString;
            }
        } else if (tag == tagPrefix + "int") {
            // An integer that does not fit in an int64_t is a number
            if (!parseInteger(value, scalar)) {
                scalar.type = ScalarType::kString;
            }
        } else if (tag == tagPrefix + "float") {
            if (parseInteger(value, scalar)) {
                if (scalar.type == ScalarType::kInteger) {
                    scalar.type = ScalarType::kDouble;
                    scalar.doubleValue = static_cast<double>(scalar.integerValue);
                }
            } else if (!parseFloat(value, scalar)) {
                scalar.type = ScalarType::kString;
            }
        } else {
            scalar.type = ScalarType::kString;
        }

        return scalar;
    }

    static bool isNullString(const std::string &value)
    {
        return value == "~" || value == "null" || value == "Null" || value == "NULL";
    }

    static bool isDigit(char c, int base)
    {
        if (c >= '0' && c <= '9') {
            return c - '0' < base;
        }

        return base == 16 && ((c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
    }

    static bool parseBool(const std::string &value, Scalar &scalar)
    {
        if (value == "true" || value == "True" || value == "TRUE") {
            scalar.boolValue = true;
        } else if (value == "false" || value == "False" || value == "FALSE") {
            scalar.boolValue = false;
        } else {
            return false;
        }

        scalar.type = ScalarType::kBool;
        return true;
    }

    /**
     * @brief   Parse an integer: [-+]?[0-9]+, 0o[0-7]+ or 0x[0-9a-fA-F]+
     *
     * An integer that does not fit in an int64_t is a number, as it is for
     * adapters for JSON parsers.
     */
    static bool parseInteger(const std::string &value, Scalar &scalar)
    {
        size_t pos = 0;
        int base = 10;
        bool negative = false;
        if (value.size() > 2 && value[0] == '0' && (value[1] == 'o' || value[1] == 'x')) {
            base = value[1] == 'o' ? 8 : 16;
            pos = 2;
        } else if (!value.empty() && (value[0] == '-' || value[0] == '+')) {
            negative = value[0] == '-';
            pos = 1;
        }

        if (pos == value.size()) {
            return false;
        }
        for (size_t i = pos; i < value.size(); i++) {
            if (!isDigit(value[i], base)) {
                return false;
            }
        }

        // from_chars accepts a leading minus sign, but not a plus sign
        const char *begin = value.data() + (negative ? 0 : pos);
        const char *end = value.data() + value.size();
        int64_t integerValue = 0;
        const auto conversion = std::from_chars(begin, end, integerValue, base);
        if (conversion.ec == std::errc() && conversion.ptr == end) {
            scalar.type = ScalarType::kInteger;
            scalar.integerValue = integerValue;
            return true;
        }

        double doubleValue = 0;
        for (size_t i = pos; i < value.size(); i++) {
            const char c = value[i];
            const int digit = c <= '9' ? c - '0' : (c | 0x20) - 'a' + 10;
            doubleValue = doubleValue * base + digit;
        }

        scalar.type = ScalarType::kDouble;
        scalar.doubleValue = negative ? -doubleValue : doubleValue;
        return true;
    }

    /**
     * @brief   Parse a number:
     *          [-+]?(\.[0-9]+|[0-9]+(\.[0-9]*)?)([eE][-+]?[0-9]+)?,
     *          [-+]?\.(inf|Inf|INF) or \.(nan|NaN|NAN)
     */
    static bool parseFloat(const std::string &value, Scalar &scalar)
    {
        if (value == ".nan" || value == ".NaN" || value == ".NAN") {
            scalar.type = ScalarType::kDouble;
            scalar.doubleValue = std::numeric_limits<double>::quiet_NaN();
            return true;
        }

        size_t pos = 0;
        const bool negative = !value.empty() && value[0] == '-';
        if (!value.empty() && (value[0] == '-' || value[0] == '+')) {
            pos = 1;
        }

        const std::string unsignedValue = value.substr(pos);
        if (unsignedValue == ".inf" || unsignedValue == ".Inf" || unsignedValue == ".INF") {
            scalar.type = ScalarType::kDouble;
            scalar.doubleValue = negative ? -std::numeric_limits<double>::infinity()
                                          : std::numeric_limits<double>::infinity();
            return true;
        }

        // Check the syntax, as parseDouble() also accepts other forms, such
        // as 'inf' and hexadecimal numbers
        size_t i = 0;
        size_t digits = 0;
        while (i < unsignedValue.size() && isDigit(unsignedValue[i], 10)) {
            i++;
            digits++;
        }
        if (i < unsignedValue.size() && unsignedValue[i] == '.') {
            i++;
            while (i < unsignedValue.size() && isDigit(unsignedValue[i], 10)) {
                i++;
                digits++;
            }
        }
        if (digits == 0) {
            return false;
        }
        if (i < unsignedValue.size() && (unsignedValue[i] == 'e' || unsignedValue[i] == 'E')) {
            i++;
            if (i < unsignedValue.size() && (unsignedValue[i] == '-' || unsignedValue[i] == '+')) {
                i++;
            }
            const size_t exponentStart = i;
            while (i < unsignedValue.size() && isDigit(unsignedValue[i], 10)) {
                i++;
            }
            if (i == exponentStart) {
                return false;
            }
        }
        if (i != unsignedValue.size()) {
            return false;
        }

        double doubleValue = 0;
        if (!internal::parseDouble(unsignedValue, doubleValue)) {
            return false;
        }

        scalar.type = ScalarType::kDouble;
        scalar.doubleValue = negative ? -doubleValue : doubleValue;
        return true;
    }

    /// Shared YAML map node used to represent an empty object
    static inline const YAML::Node emptyObject =
        YAML::Node(YAML::NodeType::Map);

    /// Reference to the contained YamlCpp value.
    const YAML::Node m_value;
};

/**
 * @brief   An implementation of the Adapter interface supporting YamlCpp.
 *
 * This class is defined in terms of the BasicAdapter template class, which
 * helps to ensure that all of the Adapter implementations behave consistently.
 *
 * @see Adapter
 * @see BasicAdapter
 */
class YamlCppAdapter
    : public BasicAdapter<YamlCppAdapter, YamlCppArray, YamlCppObjectMember,
                          YamlCppObject, YamlCppValue>
{
  public:
    /// Construct a YamlCppAdapter that contains an empty object
    YamlCppAdapter() : BasicAdapter() {}

    /// Construct a YamlCppAdapter containing a specific Nlohmann Json
    /// object
    YamlCppAdapter(const YAML::Node &value) : BasicAdapter(YamlCppValue{value})
    {
    }
};

/**
 * @brief   Class for iterating over values held in a JSON array.
 *
 * This class provides a JSON array iterator that dereferences as an instance of
 * YamlCppAdapter representing a value stored in the array. It has been
 * implemented using the boost iterator_facade template.
 *
 * @see YamlCppArray
 */
class YamlCppArrayValueIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = YamlCppAdapter;
    using difference_type = YamlCppAdapter;
    using pointer = YamlCppAdapter *;
    using reference = YamlCppAdapter &;

    /**
     * @brief   Construct a new YamlCppArrayValueIterator using an existing
     *          YamlCpp iterator.
     *
     * @param   itr  YamlCpp iterator to store
     */
    YamlCppArrayValueIterator(const YAML::Node::const_iterator &itr)
        : m_itr(itr)
    {
    }

    /// Returns a YamlCppAdapter that contains the value of the current
    /// element.
    YamlCppAdapter operator*() const
    {
        return YamlCppAdapter(*m_itr);
    }

    DerefProxy<YamlCppAdapter> operator->() const
    {
        return DerefProxy<YamlCppAdapter>(**this);
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
     * @returns true   if the iterators are equal, false otherwise.
     */
    bool operator==(const YamlCppArrayValueIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const YamlCppArrayValueIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const YamlCppArrayValueIterator &operator++()
    {
        m_itr++;

        return *this;
    }

    YamlCppArrayValueIterator operator++(int)
    {
        YamlCppArrayValueIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

    void advance(std::ptrdiff_t n)
    {
        for (auto i = 0; i < n; ++i)
            m_itr++;
    }

  private:
    YAML::Node::const_iterator m_itr;
};

/**
 * @brief   Class for iterating over the members belonging to a JSON object.
 *
 * This class provides a JSON object iterator that dereferences as an instance
 * of YamlCppObjectMember representing one of the members of the object. It
 * has been implemented using the boost iterator_facade template.
 *
 * @see YamlCppObject
 * @see YamlCppObjectMember
 */
class YamlCppObjectMemberIterator
{
  public:
    using iterator_category = std::forward_iterator_tag;
    using value_type = YamlCppObjectMember;
    using difference_type = YamlCppObjectMember;
    using pointer = YamlCppObjectMember *;
    using reference = YamlCppObjectMember &;

    /**
     * @brief   Construct an iterator from a YamlCpp iterator.
     *
     * @param   itr  YamlCpp iterator to store
     */
    YamlCppObjectMemberIterator(const YAML::Node::const_iterator &itr)
        : m_itr(itr)
    {
    }

    /**
     * @brief   Returns a YamlCppObjectMember that contains the key and
     * value belonging to the object member identified by the iterator.
     */
    YamlCppObjectMember operator*() const
    {
        return YamlCppObjectMember(m_itr->first.as<std::string>(),
                                   m_itr->second);
    }

    DerefProxy<YamlCppObjectMember> operator->() const
    {
        return DerefProxy<YamlCppObjectMember>(**this);
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
    bool operator==(const YamlCppObjectMemberIterator &other) const
    {
        return m_itr == other.m_itr;
    }

    bool operator!=(const YamlCppObjectMemberIterator &other) const
    {
        return !(m_itr == other.m_itr);
    }

    const YamlCppObjectMemberIterator &operator++()
    {
        m_itr++;

        return *this;
    }

    YamlCppObjectMemberIterator operator++(int)
    {
        YamlCppObjectMemberIterator iterator_pre(m_itr);
        ++(*this);
        return iterator_pre;
    }

  private:
    /// Internal copy of the original YamlCpp iterator
    YAML::Node::const_iterator m_itr;
};

/// Specialisation of the AdapterTraits template struct for YamlCppAdapter.
template <> struct AdapterTraits<valijson::adapters::YamlCppAdapter>
{
    typedef YAML::Node DocumentType;

    static std::string adapterName()
    {
        return "YamlCppAdapter";
    }
};

inline bool YamlCppFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return YamlCppAdapter(m_value).equalTo(other, strict);
}

inline YamlCppArrayValueIterator YamlCppArray::begin() const
{
    return m_value.begin();
}

inline YamlCppArrayValueIterator YamlCppArray::end() const
{
    return m_value.end();
}

inline YamlCppObjectMemberIterator YamlCppObject::begin() const
{
    return m_value.begin();
}

inline YamlCppObjectMemberIterator YamlCppObject::end() const
{
    return m_value.end();
}

inline YamlCppObjectMemberIterator
YamlCppObject::find(const std::string &propertyName) const
{
    for (auto itr = begin(); itr != end(); ++itr) {
        if (itr->first == propertyName) {
            return itr;
        }
    }

    return end();
}

} // namespace adapters
} // namespace valijson
