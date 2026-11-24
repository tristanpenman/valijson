#pragma once

#include <valijson/exceptions.hpp>
#include <valijson/internal/regex.hpp>
#include <valijson/schema.hpp>
#include <valijson/validation_visitor.hpp>

namespace valijson {

class Schema;
class ValidationResults;


/**
 * @brief   Class that provides validation functionality.
 *
 * @tparam  RegexEngine regular expression engine used for pattern constraint validation.

 */
template <typename RegexEngine>
class ValidatorT
{
public:
    enum TypeCheckingMode
    {
        kStrongTypes,
        kWeakTypes
    };

    enum DateTimeMode
    {
        kStrictDateTime,
        kPermissiveDateTime
    };

    /**
     * @brief  Construct a Validator that uses strong type checking by default
     */
    ValidatorT()
      : strictTypes(true)
      , strictDateTime(true)
    { }

    /**
     * @brief  Construct a Validator using a specific type checking mode
     *
     * @param  typeCheckingMode  choice of strong or weak type checking
     */
    ValidatorT(TypeCheckingMode typeCheckingMode, DateTimeMode dateTimeMode = kStrictDateTime)
      : strictTypes(typeCheckingMode == kStrongTypes)
      , strictDateTime(dateTimeMode == kStrictDateTime)
    { }

    /**
     * @brief  Validate a JSON document and optionally return the results.
     *
     * When a ValidationResults object is provided via the \c results parameter,
     * validation will be performed against each constraint defined by the
     * schema, even if validation fails for some or all constraints.
     *
     * If a pointer to a ValidationResults instance is not provided, validation
     * will only continue for as long as the constraints are validated
     * successfully.
     *
     * @param  schema   The schema to validate against
     * @param  target   A rapidjson::Value to be validated
     *
     * @param  results  An optional pointer to a ValidationResults instance that
     *                  will be used to report validation errors
     *
     * @returns  true if validation succeeds, false otherwise
     */
    template<typename AdapterType>
    bool validate(const Subschema &schema, const AdapterType &target,
            ValidationResults *results)
    {
        // Construct a ValidationVisitor to perform validation at the root level
        ValidationVisitor<AdapterType, RegexEngine> v(
                target,
                ValidationResults::Path(),
                strictTypes,
                strictDateTime,
                results,
                regexesCache);

        return v.validateSchema(schema);
    }

private:

    /// Flag indicating that strict type comparisons should be used
    bool strictTypes;

    /// Parse date/time values strictly, according to RFC-3999
    bool strictDateTime;

    /// Cached regex objects for pattern constraint. Key - pattern.
    std::unordered_map<std::string, RegexEngine> regexesCache;
};

/**
 * @brief   Struct that provides a default Regular Expression Engine using std::regex
 */
struct DefaultRegexEngine
{
    /**
     * @brief  Compile a regular expression
     *
     * @throws std::runtime_error if the regular expression is too complex to
     *         be compiled by std::regex without risking a stack overflow, as
     *         determined by internal::isRegexTooComplex(), or if it has a
     *         quantifier that follows another quantifier (see
     *         internal::hasRepeatedQuantifier()), for which matching with
     *         libstdc++ takes exponential time. These checks do not apply to
     *         Boost.Regex, which has limits of its own, and for which
     *         quantifiers such as "a++" are valid possessive quantifiers.
     */
    DefaultRegexEngine(const std::string& pattern)
      : regex(checkComplexity(pattern)) { }

    static bool search(const std::string& s, const DefaultRegexEngine& r)
    {
        return internal::regex_search(s, r.regex);
    }

private:
    static const std::string & checkComplexity(const std::string &pattern)
    {
#if !(defined(VALIJSON_USE_BOOST_REGEX) && VALIJSON_USE_BOOST_REGEX)
        if (internal::isRegexTooComplex(pattern)) {
            throwRuntimeError("Regular expression is too complex to be "
                    "compiled safely");
        }

        if (internal::hasRepeatedQuantifier(pattern)) {
            throwRuntimeError("Regular expression has a quantifier that "
                    "follows another quantifier");
        }
#endif

        return pattern;
    }

    internal::regex regex;
};

using Validator = ValidatorT<DefaultRegexEngine>;

}  // namespace valijson
