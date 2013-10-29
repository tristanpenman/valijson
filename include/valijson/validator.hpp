#ifndef __VALIJSON_VALIDATOR_HPP
#define __VALIJSON_VALIDATOR_HPP

#include <boost/bind.hpp>
#include <boost/scoped_ptr.hpp>

#include <valijson/schema.hpp>
#include <valijson/validation_visitor.hpp>

namespace valijson {

class Schema;
class ValidationResults;

/**
 * @brief  Class that wraps a schema and provides validation functionality.
 *
 * This class wraps a Schema object, and encapsulates the logic required to
 * validate rapidjson values aginst the schema.
 */
class Validator {

public:

    /**
     * @brief  Construct a validator using the specified schema.
     *
     * The schema that is provided will be copied.
     *
     * @param  schema       A schema to use for validation
     */
    Validator(const Schema &schema)
      : schema(new Schema(schema)),
        strictTypes(true) { }

    /**
     * @brief  Set flag to use strict comparison during validation.
     *
     * The default value is true, but this can be changed at any time. Future
     * invokations of validate() will make use of the new value.
     *
     * @param  strictTypes  whether or not to use strict comparison
     */
    void setStrict(bool strictTypes)
    {
        this->strictTypes = strictTypes;
    }

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
     * @param  target   A rapidjson::Value to be validated.
     *
     * @param  results  An optional pointer to a ValidationResults instance that 
     *                  will be used to report validation errors.
     *
     * @returns  true if validation succeeds, false otherwise.
     */
    template<typename AdapterType>
    bool validate(const AdapterType &target, ValidationResults *results)
    {
        // Construct a ValidationVisitor to perform validation at the root level
        ValidationVisitor<AdapterType> v(target, std::string(),
            strictTypes, results);
        
        return v.validateSchema(*schema);
    }

private:

    /// Pointer to an internal copy of a schema to use for validation
    boost::scoped_ptr<const Schema> schema;
    
    /// Flag indicating that strict type comparisons should be used
    bool strictTypes;

};

}  // namespace valijson

#endif
