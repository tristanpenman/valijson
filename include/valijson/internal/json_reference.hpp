#ifndef __VALIJSON_JSON_REFERENCE_HPP
#define __VALIJSON_JSON_REFERENCE_HPP

#include <stdexcept>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include <valijson/adapters/adapter.hpp>

namespace valijson {
namespace internal {
namespace json_reference {

/**
 * @brief   Extract JSON Pointer portion of a JSON Reference
 *
 * @param   jsonRef  JSON Reference to extract from
 *
 * @return  string containing JSON Pointer
 *
 * @throw   std::runtime_error if the string does not contain a JSON Pointer
 */
inline std::string getJsonReferencePointer(const std::string &jsonRef)
{
    // Attempt to extract JSON Pointer if '#' character is present. Note
    // that a valid pointer would contain at least a leading forward
    // slash character.
    const size_t ptrPos = jsonRef.find("#");
    if (ptrPos != std::string::npos) {
        return jsonRef.substr(ptrPos + 1);
    }

    throw std::runtime_error(
            "JSON Reference value does not contain a valid JSON Pointer");
}

/**
 * @brief   Return reference to part of document referenced by JSON Pointer
 *
 * @param   node         node to use as root for JSON Pointer resolution
 * @param   jsonPointer  string containing JSON Pointer
 *
 * @return  reference to an instance AdapterType in the specified document
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(const AdapterType &node,
        std::string jsonPointer)
{
    // TODO: This function will probably need to implement support for
    // fetching documents referenced by JSON Pointers, similar to the
    // populateSchema function.

    // Check for leading forward slash
    if (jsonPointer.find("/") != 0) {
        throw std::runtime_error(
                "JSON Pointer must begin with reference to root node");
    }

    // Remove leading slash
    jsonPointer = jsonPointer.substr(1);

    // Recursion bottoms out here
    if (jsonPointer.empty()) {
        return node;
    }

    // Extract next directive
    const std::string directive = jsonPointer.substr(1, jsonPointer.find("/"));
    if (directive.empty()) {
        throw std::runtime_error(
                "JSON Pointer contains zero-length directive");
    }

    // Remove directive from remainder of JSON pointer
    jsonPointer = jsonPointer.substr(directive.length());

    if (node.isArray()) {
        try {
            // Fragment must be non-negative integer
            const uint64_t index = boost::lexical_cast<uint64_t>(jsonPointer);
            typedef typename AdapterType::Array Array;
            typename Array::const_iterator itr = node.asArray().begin();
            itr.advance(index);
            // TODO: Check for array bounds
            return resolveJsonPointer(*itr, jsonPointer);

        } catch (boost::bad_lexical_cast &) {
            throw std::runtime_error("Invalid array index in JSON Reference: " +
                    directive);
        }

    } else if (node.maybeObject()) {
        typedef typename AdapterType::Object Object;
        typename Object::const_iterator itr = node.asObject().find(directive);
        if (itr == node.asObject().end()) {
            throw std::runtime_error("Could not find element");
        }
        return resolveJsonPointer(itr->second, jsonPointer);

    }

    throw std::runtime_error("Directive applied to simple value type");
}

} // namespace json_reference
} // namespace internal
} // namespace valijson

#endif
