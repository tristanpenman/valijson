#pragma once

#if defined(VALIJSON_USE_BOOST_REGEX) && VALIJSON_USE_BOOST_REGEX

#include <boost/regex.hpp>

namespace valijson {
namespace internal {
using boost::regex;
using boost::regex_match;
using boost::regex_search;
using boost::smatch;
} // namespace internal
} // namespace valijson

#else

#include <regex>

namespace valijson {
namespace internal {
using std::regex;
using std::regex_match;
using std::regex_search;
using std::smatch;
} // namespace internal
} // namespace valijson

#endif

#include <cstddef>
#include <string>
#include <vector>

namespace valijson {
namespace internal {

/// Maximum length of a regular expression compiled by DefaultRegexEngine
constexpr std::size_t kMaxRegexLength = 8192;

/// Maximum nesting depth of groups in a regular expression
constexpr std::size_t kMaxRegexGroupDepth = 128;

/// Maximum nesting cost of a regular expression, see isRegexTooComplex()
constexpr std::size_t kMaxRegexNestingCost = 2048;

/**
 * @brief  Determine whether a regular expression is too complex to be compiled
 *         safely by std::regex
 *
 * The parsers of std::regex in libstdc++ and libc++ are recursive descent
 * parsers, so compiling a long or deeply nested regular expression can
 * overflow the stack. Depending on the implementation, the depth of recursion
 * grows with the number of terms in a sequence (libstdc++), the number of
 * alternatives (libc++), and the nesting of groups (both).
 *
 * The nesting cost of a regular expression is the largest number, at any point
 * in the expression, of terms in the sequences that are being parsed at each
 * level of nesting, plus four for each level of nesting. Terms of a sequence
 * that is followed by '|' no longer count once the next alternative starts.
 * The limits are well above what schemas use in practice, but low enough to
 * avoid stack overflows with 512 KiB of stack.
 *
 * @param  pattern  regular expression to check
 *
 * @return \c true if the regular expression exceeds one of the limits
 */
inline bool isRegexTooComplex(const std::string &pattern)
{
    if (pattern.size() > kMaxRegexLength) {
        return true;
    }

    // Number of terms in the current sequence at each level of nesting
    std::vector<std::size_t> terms{0};
    std::size_t totalTerms = 0;

    const std::size_t length = pattern.size();
    for (std::size_t i = 0; i < length; i++) {
        const char c = pattern[i];
        if (c == '(') {
            terms.back()++;
            totalTerms++;
            if (terms.size() > kMaxRegexGroupDepth) {
                return true;
            }
            terms.push_back(0);
        } else if (c == ')') {
            // An unbalanced parenthesis is reported by the regex engine
            if (terms.size() > 1) {
                totalTerms -= terms.back();
                terms.pop_back();
            }
        } else if (c == '|') {
            totalTerms -= terms.back();
            terms.back() = 0;
        } else {
            if (c == '\\') {
                // Skip the escaped character
                i++;
            } else if (c == '[') {
                // Skip to the end of the character class, which may start
                // with '^' and a literal ']'
                std::size_t j = i + 1;
                if (j < length && pattern[j] == '^') {
                    j++;
                }
                if (j < length && pattern[j] == ']') {
                    j++;
                }
                while (j < length && pattern[j] != ']') {
                    if (pattern[j] == '\\') {
                        j++;
                    }
                    j++;
                }
                i = j;
            }

            terms.back()++;
            totalTerms++;
        }

        const std::size_t depth = terms.size() - 1;
        if (totalTerms + 4 * depth > kMaxRegexNestingCost) {
            return true;
        }
    }

    return false;
}

/**
 * @brief  Find the end of a quantifier in braces, such as {2}, {2,} or {2,4}
 *
 * @return index just past the closing brace, or 0 if the brace at the given
 *         index does not start a quantifier, in which case it is a literal
 *         character
 */
inline std::size_t findBraceQuantifierEnd(const std::string &pattern,
                                          std::size_t brace)
{
    std::size_t i = brace + 1;
    const std::size_t digitsStart = i;
    while (i < pattern.size() && pattern[i] >= '0' && pattern[i] <= '9') {
        i++;
    }
    if (i == digitsStart) {
        return 0;
    }
    if (i < pattern.size() && pattern[i] == ',') {
        i++;
        while (i < pattern.size() && pattern[i] >= '0' && pattern[i] <= '9') {
            i++;
        }
    }

    return i < pattern.size() && pattern[i] == '}' ? i + 1 : 0;
}

/**
 * @brief  Determine whether a regular expression has a quantifier that
 *         follows another quantifier, such as "a++" or "a{2}*"
 *
 * In ECMA 262, a quantifier may only be followed by a single '?', which makes
 * it non-greedy. libstdc++ nevertheless accepts repeated quantifiers, and
 * treats them as nested quantifiers, for which matching takes time that is
 * exponential in the length of the string.
 *
 * @param  pattern  regular expression to check
 *
 * @return \c true if a quantifier follows another quantifier
 */
inline bool hasRepeatedQuantifier(const std::string &pattern)
{
    enum class Previous { kOther, kQuantifier, kNonGreedyQuantifier };
    Previous previous = Previous::kOther;

    const std::size_t length = pattern.size();
    for (std::size_t i = 0; i < length; i++) {
        const char c = pattern[i];

        std::size_t quantifierEnd = 0;
        if (c == '*' || c == '+') {
            quantifierEnd = i + 1;
        } else if (c == '?') {
            if (previous == Previous::kQuantifier) {
                previous = Previous::kNonGreedyQuantifier;
                continue;
            }
            quantifierEnd = i + 1;
        } else if (c == '{') {
            quantifierEnd = findBraceQuantifierEnd(pattern, i);
        }

        if (quantifierEnd != 0) {
            // A quantifier at the start of the expression, or of a group or
            // an alternative, is left for the regex engine to reject
            if (previous != Previous::kOther) {
                return true;
            }
            previous = Previous::kQuantifier;
            i = quantifierEnd - 1;
            continue;
        }

        if (c == '\\') {
            // Skip the escaped character
            i++;
        } else if (c == '[') {
            // Skip to the end of the character class, which may start with
            // '^' and a literal ']'
            std::size_t j = i + 1;
            if (j < length && pattern[j] == '^') {
                j++;
            }
            if (j < length && pattern[j] == ']') {
                j++;
            }
            while (j < length && pattern[j] != ']') {
                if (pattern[j] == '\\') {
                    j++;
                }
                j++;
            }
            i = j;
        }

        previous = Previous::kOther;
    }

    return false;
}

} // namespace internal
} // namespace valijson
