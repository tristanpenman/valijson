#ifndef __VALIJSON_JSON_REFERENCE_HPP
#define __VALIJSON_JSON_REFERENCE_HPP

#include <stdexcept>
#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/optional.hpp>

#include <valijson/adapters/adapter.hpp>

namespace {

/**
 * @brief   Recursively locate the value referenced by a JSON Pointer
 *
 * @param   node            current node in recursive evaluation of JSON Pointer
 * @param   jsonPointer     string containing complete JSON Pointer
 * @param   jsonPointerItr  string iterator pointing the part of the string
 *                          currently being evaluated
 *
 * @return  an instance AdapterType in the specified document
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(
        const AdapterType &node,
        const std::string &jsonPointer,
        std::string::const_iterator jsonPointerItr)
{
    // TODO: This function will probably need to implement support for
    // fetching documents referenced by JSON Pointers, similar to the
    // populateSchema function.

    const std::string::const_iterator jsonPointerEnd = jsonPointer.end();

    // Check for leading forward slash
    if (std::find(jsonPointerItr, jsonPointerEnd, '/') == jsonPointerEnd) {
        throw std::runtime_error(
                "Expected '/' while parsing JSON Pointer.");
    }

    // Proceed past leading slash
    jsonPointerItr++;

    // Recursion bottoms out here
    if (jsonPointerItr == jsonPointerEnd) {
        return node;
    }

    // Find iterator that points to next slash, or end of string
    const std::string::const_iterator jsonPointerNext =
            std::find(jsonPointerItr, jsonPointerEnd, '/');

    // Extract the next 'directive'
    const std::string directive(jsonPointerItr, jsonPointerNext);
    if (directive.empty()) {
        throw std::runtime_error(
                "Expected at least one non-delimiting character in directive.");
    }

    if (node.isArray()) {
        try {
            // Fragment must be non-negative integer
            const uint64_t index = boost::lexical_cast<uint64_t>(jsonPointer);
            typedef typename AdapterType::Array Array;
            typename Array::const_iterator itr = node.asArray().begin();

            // TODO: Check for array bounds
            itr.advance(index);

            if (jsonPointerNext == jsonPointerEnd) {
                // Bottom out recursion since this is the last directive
                return *itr;
            } else {
                // Recursively process the next directive in the JSON Pointer
                return resolveJsonPointer(*itr, jsonPointer, jsonPointerNext);
            }

        } catch (boost::bad_lexical_cast &) {
            throw std::runtime_error("Expected directive to contain "
                    "non-negative integer to allow for array indexing; "
                    "actual value: " + directive);
        }

    } else if (node.maybeObject()) {
        // Fragment must identify a member of the candidate object
        typedef typename AdapterType::Object Object;
        typename Object::const_iterator itr = node.asObject().find(directive);
        if (itr == node.asObject().end()) {
            throw std::runtime_error("Expected directive to identify an "
                    "element in the current object; "
                    "actual value: " + directive);
        }

        if (jsonPointerNext == jsonPointerEnd) {
            // Bottom out recursion since this is the last directive
            return itr->second;
        } else {
            // Recursively process the next directive in the JSON Pointer
            return resolveJsonPointer(itr->second, jsonPointer,
                    jsonPointerNext);
        }
    }

    throw std::runtime_error("Expected end of JSON Pointer, but at least "
            "one directive has not been processed; "
            "actual directive: " + directive);
}

} // end anonymous namespace

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
 * @brief   Return the JSON Value referenced by a JSON Pointer
 *
 * @param   rootNode     node to use as root for JSON Pointer resolution
 * @param   jsonPointer  string containing JSON Pointer
 *
 * @return  an instance AdapterType in the specified document
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(
        const AdapterType &rootNode,
        const std::string &jsonPointer)
{
    return ::resolveJsonPointer(rootNode, jsonPointer, jsonPointer.begin());
}

} // namespace json_reference
} // namespace internal
} // namespace valijson

#endif
