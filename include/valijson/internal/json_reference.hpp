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
 * This function takes both a string reference and an iterator to the beginning
 * of the substring that is being resolved. This iterator is expected to point
 * to the beginning of a reference token, whose length will be determined by
 * searching for the next delimiter ('/' or '\0'). A reference token must be
 * at least one character in length to be considered valid.
 *
 * Once the next reference token has been identified, it will be used either as
 * an array index or as an the name an object member. The validity of a
 * reference token depends on the type of the node currently being traversed,
 * and the applicability of the token to that node. For example, an array can
 * only be dereferenced by a non-negative integral index.
 *
 * Once the next node has been identified, the length of the remaining portion
 * of the JSON Pointer will be used to determine whether recursion should
 * terminate.
 *
 * @param   node            current node in recursive evaluation of JSON Pointer
 * @param   jsonPointer     string containing complete JSON Pointer
 * @param   jsonPointerItr  string iterator pointing the beginning of the next
 *                          reference token
 *
 * @return  an instance of AdapterType that wraps the dereferenced node
 */
template<typename AdapterType>
inline AdapterType resolveJsonPointer(
        const AdapterType &node,
        const std::string &jsonPointer,
        const std::string::const_iterator jsonPointerItr)
{
    // TODO: This function will probably need to implement support for
    // fetching documents referenced by JSON Pointers, similar to the
    // populateSchema function.

    const std::string::const_iterator jsonPointerEnd = jsonPointer.end();

    // Terminate recursion if all reference tokens have been consumed
    if (jsonPointerItr == jsonPointerEnd) {
        return node;
    }

    // Reference tokens must begin with a leading slash
    if (*jsonPointerItr != '/') {
        throw std::runtime_error("Expected reference token to begin with "
                "leading slash; remaining tokens: " +
                std::string(jsonPointerItr, jsonPointerEnd));
    }

    // Find iterator that points to next slash or newline character; this is
    // one character past the end of the current reference token
    std::string::const_iterator jsonPointerNext =
            std::find(jsonPointerItr + 1, jsonPointerEnd, '/');

    // Extract the next reference token
    const std::string referenceToken(jsonPointerItr + 1, jsonPointerNext);

    // Empty reference tokens should be ignored
    if (referenceToken.empty()) {
        return resolveJsonPointer(node, jsonPointer, jsonPointerNext);

    } else if (node.isArray()) {
        try {
            // Fragment must be non-negative integer
            const uint64_t index = boost::lexical_cast<uint64_t>(jsonPointer);
            typedef typename AdapterType::Array Array;
            typename Array::const_iterator itr = node.asArray().begin();

            // TODO: Check for array bounds
            itr.advance(index);

            // Recursively process the remaining tokens
            return resolveJsonPointer(*itr, jsonPointer, jsonPointerNext);

        } catch (boost::bad_lexical_cast &) {
            throw std::runtime_error("Expected reference token to contain a "
                    "non-negative integer to identify an element in the "
                    "current array; actual token: " + referenceToken);
        }

    } else if (node.maybeObject()) {
        // Fragment must identify a member of the candidate object
        typedef typename AdapterType::Object Object;
        typename Object::const_iterator itr = node.asObject().find(
                referenceToken);
        if (itr == node.asObject().end()) {
            throw std::runtime_error("Expected reference token to identify an "
                    "element in the current object; "
                    "actual token: " + referenceToken);
        }

        // Recursively process the remaining tokens
        return resolveJsonPointer(itr->second, jsonPointer, jsonPointerNext);
    }

    throw std::runtime_error("Expected end of JSON Pointer, but at least "
            "one reference token has not been processed; remaining tokens: " +
            std::string(jsonPointerNext, jsonPointerEnd));
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
