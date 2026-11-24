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

} // namespace internal
} // namespace valijson
