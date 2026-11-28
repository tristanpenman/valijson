#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace valijson {
namespace internal {
namespace uri_template {

/**
 * @brief  Determine whether a character is a hexadecimal digit
 */
inline bool isHexDigit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') ||
           (c >= 'a' && c <= 'f');
}

/**
 * @brief  Determine whether a pct-encoded triplet starts at a position
 */
inline bool isPctEncoded(const std::string &s, std::size_t i)
{
    return i + 2 < s.size() && s[i] == '%' && isHexDigit(s[i + 1]) &&
           isHexDigit(s[i + 2]);
}

/**
 * @brief  Decode a UTF-8 encoded code point
 *
 * @param  s          string to decode from
 * @param  i          index of the first byte of the code point, which is
 *                    advanced past its last byte if decoding succeeds
 * @param  codePoint  set to the decoded code point
 *
 * @return \c true if a code point was decoded; \c false if the bytes are not
 *         valid UTF-8, including overlong encodings and surrogates
 */
inline bool decodeUtf8(const std::string &s, std::size_t &i,
                       std::uint32_t &codePoint)
{
    const auto byte = [&s](std::size_t index) {
        return static_cast<unsigned char>(s[index]);
    };

    const unsigned char first = byte(i);
    std::size_t length = 0;
    std::uint32_t minimum = 0;
    if (first < 0x80) {
        codePoint = first;
        i++;
        return true;
    } else if ((first & 0xE0) == 0xC0) {
        length = 2;
        minimum = 0x80;
        codePoint = first & 0x1F;
    } else if ((first & 0xF0) == 0xE0) {
        length = 3;
        minimum = 0x800;
        codePoint = first & 0x0F;
    } else if ((first & 0xF8) == 0xF0) {
        length = 4;
        minimum = 0x10000;
        codePoint = first & 0x07;
    } else {
        return false;
    }

    if (i + length > s.size()) {
        return false;
    }

    for (std::size_t j = 1; j < length; j++) {
        if ((byte(i + j) & 0xC0) != 0x80) {
            return false;
        }
        codePoint = (codePoint << 6) | (byte(i + j) & 0x3F);
    }

    if (codePoint < minimum || codePoint > 0x10FFFF ||
        (codePoint >= 0xD800 && codePoint <= 0xDFFF)) {
        return false;
    }

    i += length;
    return true;
}

/**
 * @brief  Determine whether a code point is a ucschar or an iprivate, as
 *         defined in RFC 3987, which RFC 6570 allows in literals
 */
inline bool isUcsCharOrIPrivate(std::uint32_t c)
{
    // iprivate
    if ((c >= 0xE000 && c <= 0xF8FF) || (c >= 0xF0000 && c <= 0xFFFFD) ||
        (c >= 0x100000 && c <= 0x10FFFD)) {
        return true;
    }

    // ucschar
    if ((c >= 0xA0 && c <= 0xD7FF) || (c >= 0xF900 && c <= 0xFDCF) ||
        (c >= 0xFDF0 && c <= 0xFFEF)) {
        return true;
    }

    // ucschar in the supplementary planes, excluding the last two code
    // points of each plane, from %x10000-1FFFD to %xE1000-EFFFD
    if (c >= 0x10000 && c <= 0xEFFFD) {
        const std::uint32_t plane = c >> 16;
        const std::uint32_t offset = c & 0xFFFF;
        return offset <= 0xFFFD && (plane != 0xE || offset >= 0x1000);
    }

    return false;
}

/**
 * @brief  Determine whether an ASCII character may appear in a literal
 *
 * Includes the apostrophe, as corrected by RFC 6570 errata 6937.
 */
inline bool isLiteralAscii(char c)
{
    return c == 0x21 || c == 0x23 || c == 0x24 || c == 0x26 || c == 0x27 ||
           (c >= 0x28 && c <= 0x3B) || c == 0x3D || (c >= 0x3F && c <= 0x5B) ||
           c == 0x5D || c == 0x5F || (c >= 0x61 && c <= 0x7A) || c == 0x7E;
}

/**
 * @brief  Parse a varchar: ALPHA / DIGIT / "_" / pct-encoded
 *
 * @return \c true if a varchar was parsed, advancing the index past it
 */
inline bool parseVarchar(const std::string &s, std::size_t &i)
{
    if (i >= s.size()) {
        return false;
    }

    const char c = s[i];
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '_') {
        i++;
        return true;
    }

    if (isPctEncoded(s, i)) {
        i += 3;
        return true;
    }

    return false;
}

/**
 * @brief  Parse a varspec: varname [ ":" max-length / "*" ]
 *
 * @return \c true if a varspec was parsed, advancing the index past it
 */
inline bool parseVarspec(const std::string &s, std::size_t &i)
{
    // varname = varchar *( ["."] varchar )
    if (!parseVarchar(s, i)) {
        return false;
    }
    for (;;) {
        std::size_t next = i;
        if (next < s.size() && s[next] == '.') {
            next++;
        }
        if (!parseVarchar(s, next)) {
            break;
        }
        i = next;
    }

    if (i < s.size() && s[i] == ':') {
        // max-length = %x31-39 0*3DIGIT
        i++;
        if (i >= s.size() || s[i] < '1' || s[i] > '9') {
            return false;
        }
        i++;
        for (int digits = 0;
             digits < 3 && i < s.size() && s[i] >= '0' && s[i] <= '9';
             digits++) {
            i++;
        }
    } else if (i < s.size() && s[i] == '*') {
        i++;
    }

    return true;
}

/**
 * @brief  Parse an expression: "{" [ operator ] variable-list "}"
 *
 * @param  i  index just past the opening brace, which is advanced past the
 *            closing brace if an expression was parsed
 */
inline bool parseExpression(const std::string &s, std::size_t &i)
{
    static const std::string operators = "+#./;?&=,!@|";
    if (i < s.size() && operators.find(s[i]) != std::string::npos) {
        i++;
    }

    for (;;) {
        if (!parseVarspec(s, i) || i >= s.size()) {
            return false;
        }
        if (s[i] == '}') {
            i++;
            return true;
        }
        if (s[i] != ',') {
            return false;
        }
        i++;
    }
}

/**
 * @brief  Determine whether a string is a URI Template, as defined in
 *         RFC 6570
 *
 * @param  s  string to check, encoded as UTF-8
 *
 * @return \c true if the string is a URI Template
 */
inline bool isUriTemplate(const std::string &s)
{
    std::size_t i = 0;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '{') {
            i++;
            if (!parseExpression(s, i)) {
                return false;
            }
        } else if (c == '%') {
            if (!isPctEncoded(s, i)) {
                return false;
            }
            i += 3;
        } else if (static_cast<unsigned char>(c) >= 0x80) {
            std::uint32_t codePoint = 0;
            if (!decodeUtf8(s, i, codePoint) ||
                !isUcsCharOrIPrivate(codePoint)) {
                return false;
            }
        } else if (isLiteralAscii(c)) {
            i++;
        } else {
            return false;
        }
    }

    return true;
}

} // namespace uri_template
} // namespace internal
} // namespace valijson
