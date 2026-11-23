#pragma once

#include <cstddef>
#include <cstring>
#include <string>

namespace valijson {
namespace internal {

namespace detail {

/// VCHAR / WSP, as defined in RFC 5234, appendix B.1
inline bool isEmailVisibleOrWhitespace(const char c)
{
    return (c >= 33 && c <= 126) || c == ' ' || c == '\t';
}

/// atext, as defined in RFC 5322, section 3.2.3
inline bool isEmailAtext(const char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
           (c >= '0' && c <= '9') ||
           (c != '\0' && std::strchr("!#$%&'*+-/=?^_`{|}~", c) != nullptr);
}

/**
 * @brief  Check that the range [begin, end) is a dot-atom-text, as defined in
 *         RFC 5322, section 3.2.3
 *
 * A dot-atom-text consists of one or more atoms separated by single dots, and
 * does not start or end with a dot.
 */
inline bool isEmailDotAtom(const std::string &s, size_t begin, size_t end)
{
    if (begin >= end || s[begin] == '.' || s[end - 1] == '.') {
        return false;
    }

    for (size_t i = begin; i < end; ++i) {
        if (s[i] == '.') {
            if (s[i - 1] == '.') {
                return false;
            }
        } else if (!isEmailAtext(s[i])) {
            return false;
        }
    }

    return true;
}

/**
 * @brief  Check that the range [begin, end) is a quoted-string, as defined in
 *         RFC 5322, section 3.2.4
 *
 * The content of a quoted-string consists of printable US-ASCII characters
 * other than '"' and '\', whitespace, and quoted pairs.
 */
inline bool isEmailQuotedString(const std::string &s, const size_t begin,
                                const size_t end)
{
    if (end - begin < 2 || s[begin] != '"' || s[end - 1] != '"') {
        return false;
    }

    for (size_t i = begin + 1; i < end - 1; ++i) {
        const char c = s[i];
        if (c == '\\') {
            // quoted-pair = "\" ( VCHAR / WSP )
            if (++i >= end - 1) {
                return false;
            }
            if (!isEmailVisibleOrWhitespace(s[i])) {
                return false;
            }
        } else if (c == '"' || !isEmailVisibleOrWhitespace(c)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief  Check that the range [begin, end) is a domain-literal, as defined in
 *         RFC 5322, section 3.4.1
 *
 * A domain-literal is enclosed in square brackets and consists of printable
 * US-ASCII characters other than '[', ']' and '\'.
 */
inline bool isEmailDomainLiteral(const std::string &s, size_t begin, size_t end)
{
    if (end - begin < 2 || s[begin] != '[' || s[end - 1] != ']') {
        return false;
    }

    for (size_t i = begin + 1; i < end - 1; ++i) {
        const char c = s[i];
        if (c < 33 || c > 126 || c == '[' || c == ']' || c == '\\') {
            return false;
        }
    }

    return true;
}

} // namespace detail

/**
 * @brief  Check whether a string is an e-mail address, as defined by the
 *         addr-spec production in RFC 5322, section 3.4.1
 *
 * The local part is either a dot-atom or a quoted string, and the domain is
 * either a dot-atom or a domain literal. Comments and folding whitespace are
 * not permitted. Characters outside of US-ASCII are not permitted.
 *
 * @param  s  string to check, which may contain embedded null bytes
 *
 * @return \c true if \p s is an e-mail address
 */
inline bool isEmail(const std::string &s)
{
    // The domain cannot contain '@', so the last '@' separates the local part
    // from the domain
    const size_t at = s.rfind('@');
    if (at == std::string::npos || at == 0 || at + 1 >= s.size()) {
        return false;
    }

    const bool localPartValid = detail::isEmailDotAtom(s, 0, at) ||
                                detail::isEmailQuotedString(s, 0, at);
    const bool domainValid = detail::isEmailDotAtom(s, at + 1, s.size()) ||
                             detail::isEmailDomainLiteral(s, at + 1, s.size());

    return localPartValid && domainValid;
}

} // namespace internal
} // namespace valijson
