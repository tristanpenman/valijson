#pragma once
#ifndef __VALIJSON_SCHEMA_HPP
#define __VALIJSON_SCHEMA_HPP

#include <valijson/subschema.hpp>

namespace valijson {

/**
 * Represents the root of a JSON Schema
 *
 * The root is distinct from other sub-schemas because it is the canonical
 * starting point for validation of a document against a given a JSON Schema.
 */
class Schema: public Subschema
{
public:
    /**
     * @brief  Construct a new Schema instance with no constraints
     */
    Schema() { }

    /**
     * @brief  Construct a new Schema based an existing Schema instance
     *
     * The constructed Schema object will contain a copy of each constraint
     * defined in the referenced Schema.
     *
     * @param  schema  schema to copy constraints from
     */
    Schema(const Schema &schema)
      : Subschema(schema) { }

};

} // namespace valijson

#endif
