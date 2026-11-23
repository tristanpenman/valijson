#pragma once

#include <algorithm>
#include <cstddef>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <valijson/internal/ip_address.hpp>
#include <valijson/internal/regex.hpp>

namespace valijson {
namespace internal {
namespace uri {

/**
 * @brief  Check whether a URI is absolute, returns pos just past end if found
 */
inline std::optional<size_t> parseScheme(const std::string &uri)
{
    constexpr auto marker = "://";

    const auto pos = uri.find(marker);

    return pos == std::string::npos
        ? std::nullopt
        : std::make_optional<size_t>(pos + 3);
}

/**
  * @brief  Check whether a URI is absolute
  */
inline bool isUriAbsolute(const std::string &uri)
{
    return parseScheme(uri).has_value();
}

/**
 * @brief  Check whether a URI is a URN
 *
 * This function validates that the URI matches the RFC 8141 spec.
 *
 * Example: urn:isbn:978-3-16-148410-0
 */
inline bool isUrn(const std::string &documentUri) {
  static const internal::regex pattern(
      "^((urn)|(URN)):(?!urn:)([a-zA-Z0-9][a-zA-Z0-9-]{1,31})(:[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;=]+)+(\\?[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}(#[-a-zA-Z0-9\\\\._~%!$&'()\\/*+,;:=]+){0,1}$");

  return internal::regex_match(documentUri, pattern);
}

/**
 * Resolve a relative URI reference within a given scope.
 */
inline std::string resolveRelativeUri(
        const std::string &resolutionScope,
        const std::string &relativeUri)
{
    // Trivial case: There is no relative URI to resolve
    if (relativeUri.empty()) {
        return resolutionScope;
    }

    // Trivial case: The relative URI is actually absolute, or is a URN
    if (isUriAbsolute(relativeUri) || isUrn(relativeUri)) {
        return relativeUri;
    }

    const auto schemeEnd = parseScheme(resolutionScope);
    std::string schemeAndAuthority;
    std::string basePath;
    if (schemeEnd.has_value()) {
        // Extract scheme+authority, e.g. http://userinfo@example.com:8080
        const auto authorityEnd = resolutionScope.find('/', *schemeEnd);
        schemeAndAuthority = resolutionScope.substr(0, authorityEnd);

        // Assume path starts immediately after scheme+authority
        const std::string::size_type pathStart = schemeAndAuthority.size();

        // Extract base path from resolution scope, or use '/' as default
        basePath = pathStart < resolutionScope.size()
                ? resolutionScope.substr(pathStart)
                : "/";
    } else {
        // A relative resolution scope still represents a URI path. Relative
        // references replace its final path segment rather than being appended
        // directly to the complete filename.
        basePath = resolutionScope;
    }

    // Strip everything after fragment marker if present
    const std::string::size_type baseFragmentPos = basePath.find('#');
    if (baseFragmentPos != std::string::npos) {
        basePath.erase(baseFragmentPos);
    }

    // Strip everything after query marker if present (we need both)
    const std::string::size_type baseQueryPos = basePath.find('?');
    if (baseQueryPos != std::string::npos) {
        basePath.erase(baseQueryPos);
    }

    // Next two cases...
    if (!relativeUri.empty()) {
        // Relative URI is just a fragment OR a query
        if ((relativeUri[0] == '#') || relativeUri[0] == '?') {
            // Append it to the scheme+authority and clean base path
            return schemeAndAuthority + basePath + relativeUri;
        }
    }

    // Split the relative URI into path and suffix components
    std::string relativePath = relativeUri;
    std::string suffix;
    const std::string::size_type relativeFragmentPos = relativePath.find('#');
    const std::string::size_type relativeQueryPos = relativePath.find('?');
    const std::string::size_type suffixPos =
            relativeFragmentPos == std::string::npos ? relativeQueryPos :
            relativeQueryPos == std::string::npos ? relativeFragmentPos :
            std::min(relativeFragmentPos, relativeQueryPos);
    if (suffixPos != std::string::npos) {
        suffix = relativePath.substr(suffixPos);
        relativePath.erase(suffixPos);
    }

    // Merged path can be constructed in two ways
    std::string mergedPath;
    if (!relativePath.empty() && relativePath[0] == '/') {
        // Case 1: Relative path starts with `/` so we can use it as-is
        mergedPath = relativePath;
    } else {
        // Case 2: We need to join the base path and relative path, which we do by
        // stripping the base path of any segment after its last slash
        const std::string::size_type lastSlashPos = basePath.find_last_of('/');
        mergedPath = lastSlashPos == std::string::npos
                ? relativePath
                : basePath.substr(0, lastSlashPos + 1) + relativePath;
    }

    // Resolve relative path segments
    const bool absolutePath = !mergedPath.empty() && mergedPath[0] == '/';
    std::vector<std::string> segments;
    std::string segment;
    for (const char c : mergedPath) {
        if (c == '/') {
            if (segment == "..") {
                if (!segments.empty() && segments.back() != "..") {
                    segments.pop_back();
                } else if (!absolutePath) {
                    segments.push_back(segment);
                }
            } else if (!segment.empty() && segment != ".") {
                segments.push_back(segment);
            }
            segment.clear();
        } else {
            segment += c;
        }
    }
    if (segment == "..") {
        if (!segments.empty() && segments.back() != "..") {
            segments.pop_back();
        } else if (!absolutePath) {
            segments.push_back(segment);
        }
    } else if (!segment.empty() && segment != ".") {
        segments.push_back(segment);
    }

    // Generate final normalised path
    std::string normalisedPath = absolutePath ? "/" : "";
    for (auto itr = segments.begin(); itr != segments.end(); ++itr) {
        if (itr != segments.begin()) {
            normalisedPath += "/";
        }
        normalisedPath += *itr;
    }
    if (!mergedPath.empty() && mergedPath[mergedPath.size() - 1] == '/' &&
            (normalisedPath.empty() || normalisedPath[normalisedPath.size() - 1] != '/')) {
        normalisedPath += "/";
    }

    // Construct final URI
    return schemeAndAuthority + normalisedPath + suffix;
}

namespace detail {

inline bool isAlpha(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

inline bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}

inline bool isHexDigit(char c)
{
    return isDigit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

/// unreserved = ALPHA / DIGIT / "-" / "." / "_" / "~"
inline bool isUnreserved(char c)
{
    return isAlpha(c) || isDigit(c) || c == '-' || c == '.' || c == '_' ||
           c == '~';
}

/// sub-delims = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
inline bool isSubDelim(char c)
{
    switch (c) {
    case '!':
    case '$':
    case '&':
    case '\'':
    case '(':
    case ')':
    case '*':
    case '+':
    case ',':
    case ';':
    case '=':
        return true;
    default:
        return false;
    }
}

/**
 * @brief  Check that the range [begin, end) consists only of unreserved
 *         characters, sub-delims, percent-encoded octets and the characters
 *         in \p extra
 */
inline bool isEncodedRange(const std::string &s, size_t begin, size_t end,
                           const char *extra)
{
    for (size_t i = begin; i < end; ++i) {
        const char c = s[i];
        if (c == '%') {
            if (end - i < 3 || !isHexDigit(s[i + 1]) || !isHexDigit(s[i + 2])) {
                return false;
            }
            i += 2;
        } else if (!isUnreserved(c) && !isSubDelim(c) &&
                   (c == '\0' || std::strchr(extra, c) == nullptr)) {
            return false;
        }
    }

    return true;
}

/// IPvFuture = "v" 1*HEXDIG "." 1*( unreserved / sub-delims / ":" )
inline bool isIpvFuture(const std::string &s)
{
    if (s.size() < 4 || (s[0] != 'v' && s[0] != 'V')) {
        return false;
    }

    size_t i = 1;
    while (i < s.size() && isHexDigit(s[i])) {
        ++i;
    }

    if (i == 1 || i >= s.size() - 1 || s[i] != '.') {
        return false;
    }

    for (++i; i < s.size(); ++i) {
        if (!isUnreserved(s[i]) && !isSubDelim(s[i]) && s[i] != ':') {
            return false;
        }
    }

    return true;
}

/// authority = [ userinfo "@" ] host [ ":" port ]
inline bool isAuthority(const std::string &s, size_t begin, size_t end)
{
    // userinfo = *( unreserved / pct-encoded / sub-delims / ":" )
    const size_t at = s.find('@', begin);
    if (at != std::string::npos && at < end) {
        if (!isEncodedRange(s, begin, at, ":")) {
            return false;
        }
        begin = at + 1;
    }

    size_t hostEnd = end;
    if (begin < end && s[begin] == '[') {
        // IP-literal = "[" ( IPv6address / IPvFuture ) "]"
        const size_t close = s.find(']', begin);
        if (close == std::string::npos || close >= end) {
            return false;
        }

        const std::string literal = s.substr(begin + 1, close - begin - 1);
        if (!isIpv6Address(literal) && !isIpvFuture(literal)) {
            return false;
        }

        hostEnd = close + 1;
        if (hostEnd < end && s[hostEnd] != ':') {
            return false;
        }
    } else {
        // reg-name = *( unreserved / pct-encoded / sub-delims ), which also
        // covers IPv4address
        const size_t colon = s.find(':', begin);
        if (colon != std::string::npos && colon < end) {
            hostEnd = colon;
        }

        if (!isEncodedRange(s, begin, hostEnd, "")) {
            return false;
        }
    }

    // port = *DIGIT
    return hostEnd == end ||
           std::all_of(s.begin() + static_cast<std::ptrdiff_t>(hostEnd + 1),
                       s.begin() + static_cast<std::ptrdiff_t>(end), isDigit);
}

/**
 * @brief  Check the part of a URI or relative reference that follows the
 *         scheme
 *
 * Covers hier-part or relative-part, followed by optional query and fragment
 * components. For relative references, the first path segment must not
 * contain a colon (path-noscheme).
 */
inline bool isHierPartQueryFragment(const std::string &s, size_t begin,
                                    bool relative)
{
    const size_t fragmentStart = s.find('#', begin);
    const size_t end =
        fragmentStart == std::string::npos ? s.size() : fragmentStart;

    const size_t queryStart = s.find('?', begin);
    const size_t pathEnd = (queryStart == std::string::npos || queryStart > end)
                               ? end
                               : queryStart;

    // query = *( pchar / "/" / "?" )
    if (pathEnd < end && !isEncodedRange(s, pathEnd + 1, end, ":@/?")) {
        return false;
    }

    // fragment = *( pchar / "/" / "?" )
    if (end < s.size() && !isEncodedRange(s, end + 1, s.size(), ":@/?")) {
        return false;
    }

    size_t pathBegin = begin;
    if (pathEnd - begin >= 2 && s[begin] == '/' && s[begin + 1] == '/') {
        const size_t slash = s.find('/', begin + 2);
        const size_t authorityEnd =
            (slash == std::string::npos || slash > pathEnd) ? pathEnd : slash;
        if (!isAuthority(s, begin + 2, authorityEnd)) {
            return false;
        }
        pathBegin = authorityEnd;
    } else if (relative && pathBegin < pathEnd && s[pathBegin] != '/') {
        // path-noscheme: the first segment must not contain a colon
        const size_t slash = s.find('/', pathBegin);
        const size_t firstSegmentEnd =
            (slash == std::string::npos || slash > pathEnd) ? pathEnd : slash;
        const size_t colon = s.find(':', pathBegin);
        if (colon != std::string::npos && colon < firstSegmentEnd) {
            return false;
        }
    }

    // path segments consist of pchar = unreserved / pct-encoded / sub-delims
    // / ":" / "@", separated by "/"
    return isEncodedRange(s, pathBegin, pathEnd, ":@/");
}

} // namespace detail

/**
 * @brief  Check whether a string is a URI, as defined in RFC 3986, section 3
 *
 * A URI must have a scheme. Characters outside of US-ASCII are not permitted
 * and must be percent-encoded.
 *
 * @param  s  string to check
 *
 * @return \c true if \p s is a URI
 */
inline bool isUri(const std::string &s)
{
    // scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
    if (s.empty() || !detail::isAlpha(s[0])) {
        return false;
    }

    size_t i = 1;
    while (i < s.size() && (detail::isAlpha(s[i]) || detail::isDigit(s[i]) ||
                            s[i] == '+' || s[i] == '-' || s[i] == '.')) {
        ++i;
    }

    if (i >= s.size() || s[i] != ':') {
        return false;
    }

    return detail::isHierPartQueryFragment(s, i + 1, false);
}

/**
 * @brief  Check whether a string is a URI reference, as defined in RFC 3986,
 *         section 4.1
 *
 * A URI reference is either a URI or a relative reference.
 *
 * @param  s  string to check
 *
 * @return \c true if \p s is a URI reference
 */
inline bool isUriReference(const std::string &s)
{
    return isUri(s) || detail::isHierPartQueryFragment(s, 0, true);
}

} // namespace uri
} // namespace internal
} // namespace valijson
