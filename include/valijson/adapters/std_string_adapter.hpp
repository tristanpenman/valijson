/**
 * @file
 *
 * @brief   Adapter implementation that wraps a single std::string value
 *
 * This allows property names to be validated against a schema as though they are a generic JSON
 * value, while allowing the rest of Valijson's API to expose property names as plain std::string
 * values.
 *
 * This was added while implementing draft 7 support. This included support for a constraint
 * called propertyNames, which can be used to ensure that the property names in an object
 * validate against a subschema.
 */

#pragma once

#include <string>

#include <valijson/adapters/adapter.hpp>
#include <valijson/adapters/frozen_value.hpp>
#include <valijson/adapters/basic_adapter.hpp>

namespace valijson {
namespace adapters {

class StdStringAdapter;
class StdStringArrayValueIterator;
class StdStringObjectMemberIterator;

typedef std::pair<std::string, StdStringAdapter> StdStringObjectMember;

class StdStringArray
{
public:
    typedef StdStringArrayValueIterator const_iterator;
    typedef StdStringArrayValueIterator iterator;

    StdStringArray() = default;

    StdStringArrayValueIterator begin() const;

    StdStringArrayValueIterator end() const;

    static size_t size()
    {
        return 0;
    }
};

class StdStringObject
{
public:
    typedef StdStringObjectMemberIterator const_iterator;
    typedef StdStringObjectMemberIterator iterator;

    StdStringObject() = default;

    StdStringObjectMemberIterator begin() const;

    StdStringObjectMemberIterator end() const;

    StdStringObjectMemberIterator find(const std::string &propertyName) const;

    static size_t size()
    {
        return 0;
    }
};

class StdStringFrozenValue: public FrozenValue
{
public:
    explicit StdStringFrozenValue(std::string source)
      : value(std::move(source)) { }

    FrozenValue * clone() const override
    {
        return new StdStringFrozenValue(value);
    }

    bool equalTo(const Adapter &other, bool strict) const override;

private:
    std::string value;
};

class StdStringAdapter: public Adapter
{
public:
    typedef StdStringArray Array;
    typedef StdStringObject Object;
    typedef StdStringObjectMember ObjectMember;

    explicit StdStringAdapter(const std::string &value)
      : m_value(value) { }

    bool applyToArray(ArrayValueCallback fn) const override
    {
        return maybeArray();
    }

    bool applyToObject(ObjectMemberCallback fn) const override
    {
        return maybeObject();
    }

    StdStringArray asArray() const
    {
        if (maybeArray()) {
            return {};
        }

        throw std::runtime_error("String value cannot be cast to array");
    }

    bool asBool() const override
    {
        return true;
    }

    bool asBool(bool &result) const override
    {
        result = true;
        return true;
    }

    double asDouble() const override
    {
        return 0;
    }

    bool asDouble(double &result) const override
    {
        result = 0;
        return true;
    }

    int64_t asInteger() const override
    {
        return 0;
    }

    bool asInteger(int64_t &result) const override
    {
        result = 0;
        return true;
    };

    StdStringObject asObject() const
    {
        if (maybeObject()) {
            return {};
        }

        throw std::runtime_error("String value cannot be cast to object");
    }

    std::string asString() const override
    {
        return m_value;
    }

    bool asString(std::string &result) const override
    {
        result = m_value;
        return true;
    }

    bool equalTo(const Adapter &other, bool strict) const override
    {
        if (strict && !other.isString()) {
            return false;
        }

        return m_value == other.asString();
    }

    FrozenValue* freeze() const override
    {
        return new StdStringFrozenValue(m_value);
    }

    static StdStringArray getArray()
    {
        throw std::runtime_error("Not supported");
    }

    size_t getArraySize() const override
    {
        throw std::runtime_error("Not supported");
    }

    bool getArraySize(size_t &result) const override
    {
        throw std::runtime_error("Not supported");
    }

    bool getBool() const override
    {
        throw std::runtime_error("Not supported");
    }

    bool getBool(bool &result) const override
    {
        throw std::runtime_error("Not supported");
    }

    double getDouble() const override
    {
        throw std::runtime_error("Not supported");
    }

    bool getDouble(double &result) const override
    {
        throw std::runtime_error("Not supported");
    }

    int64_t getInteger() const override
    {
        throw std::runtime_error("Not supported");
    }

    bool getInteger(int64_t &result) const override
    {
        throw std::runtime_error("Not supported");
    }

    double getNumber() const override
    {
        throw std::runtime_error("Not supported");
    }

    bool getNumber(double &result) const override
    {
        throw std::runtime_error("Not supported");
    }

    size_t getObjectSize() const override
    {
        throw std::runtime_error("Not supported");
    }

    bool getObjectSize(size_t &result) const override
    {
        throw std::runtime_error("Not supported");
    }

    std::string getString() const override
    {
        return m_value;
    }

    bool getString(std::string &result) const override
    {
        result = m_value;
        return true;
    }

    bool hasStrictTypes() const override
    {
        return true;
    }

    bool isArray() const override
    {
        return false;
    }

    bool isBool() const override
    {
        return false;
    }

    bool isDouble() const override
    {
        return false;
    }

    bool isInteger() const override
    {
        return false;
    }

    bool isNull() const override
    {
        return false;
    }

    bool isNumber() const override
    {
        return false;
    }

    bool isObject() const override
    {
        return false;
    }

    bool isString() const override
    {
        return true;
    }

    bool maybeArray() const override
    {
        return false;
    }

    bool maybeBool() const override
    {
        return m_value == "true" || m_value == "false";
    }

    bool maybeDouble() const override
    {
        const char *b = m_value.c_str();
        char *e = nullptr;
        strtod(b, &e);
        return e != b && e == b + m_value.length();
    }

    bool maybeInteger() const override
    {
        std::istringstream i(m_value);
        int64_t x;
        char c;
        if (!(i >> x) || i.get(c)) {
            return false;
        }

        return true;
    }

    bool maybeNull() const override
    {
        return m_value.empty();
    }

    bool maybeObject() const override
    {
        return m_value.empty();
    }

    bool maybeString() const override
    {
        return true;
    }

private:
    const std::string &m_value;
};

class StdStringArrayValueIterator: public std::iterator<std::bidirectional_iterator_tag, StdStringAdapter>
{
public:
    StdStringAdapter operator*() const
    {
        throw std::runtime_error("Not supported");
    }

    DerefProxy<StdStringAdapter> operator->() const
    {
        throw std::runtime_error("Not supported");
    }

    bool operator==(const StdStringArrayValueIterator &other) const
    {
        return true;
    }

    bool operator!=(const StdStringArrayValueIterator &other) const
    {
        return false;
    }

    const StdStringArrayValueIterator& operator++()
    {
        throw std::runtime_error("Not supported");
    }

    StdStringArrayValueIterator operator++(int)
    {
        throw std::runtime_error("Not supported");
    }

    const StdStringArrayValueIterator& operator--()
    {
        throw std::runtime_error("Not supported");
    }

    void advance(std::ptrdiff_t n)
    {
        throw std::runtime_error("Not supported");
    }
};

inline StdStringArrayValueIterator StdStringArray::begin() const
{
    return {};
}

inline StdStringArrayValueIterator StdStringArray::end() const
{
    return {};
}

class StdStringObjectMemberIterator: public std::iterator<std::bidirectional_iterator_tag, StdStringObjectMember>
{
public:
    StdStringObjectMember operator*() const
    {
        throw std::runtime_error("Not supported");
    }

    DerefProxy<StdStringObjectMember> operator->() const
    {
        throw std::runtime_error("Not supported");
    }

    bool operator==(const StdStringObjectMemberIterator &) const
    {
        return true;
    }

    bool operator!=(const StdStringObjectMemberIterator &) const
    {
        return false;
    }

    const StdStringObjectMemberIterator& operator++()
    {
        throw std::runtime_error("Not supported");
    }

    StdStringObjectMemberIterator operator++(int)
    {
        throw std::runtime_error("Not supported");
    }

    const StdStringObjectMemberIterator& operator--()
    {
        throw std::runtime_error("Not supported");
    }
};

inline StdStringObjectMemberIterator StdStringObject::begin() const
{
    return {};
}

inline StdStringObjectMemberIterator StdStringObject::end() const
{
    return {};
}

inline StdStringObjectMemberIterator StdStringObject::find(const std::string &propertyName) const
{
    return {};
}

template<>
struct AdapterTraits<valijson::adapters::StdStringAdapter>
{
    typedef std::string DocumentType;

    static std::string adapterName()
    {
        return "StdStringAdapter";
    }
};

inline bool StdStringFrozenValue::equalTo(const Adapter &other, bool strict) const
{
    return StdStringAdapter(value).equalTo(other, strict);
}

}  // namespace adapters
}  // namespace valijson
