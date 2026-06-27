# Schema Parser

This document describes the inner workings of Valijson's `SchemaParser` class. This is the component that is responsible for converting a JSON Schema document into an internal representation that is parser-agnostic.

Once parsed, the internal representation of a schema is stored in an instance of the `Schema` class.

## Entry Point

The entry point for parsing schemas is called `populateSchema()`. This is declared as follows:

```c++
template<typename AdapterType>
void populateSchema(
    const AdapterType &node,
    Schema &schema,
    typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc = nullptr,
    typename FunctionPtrs<AdapterType>::FreeDoc freeDoc = nullptr);
```

This version of `populateSchema()` is typically invoked using default values (`nullptr`) for the third and fourth arguments. When these arguments are not set, Valijson will not attempt to fetch/resolve external document references. The usage of these arguments is discussed in the [Fetch / Free](#fetch--free) section below.

The initial call to `populateSchema()` doesn't do much, as it is primarily an entry point for a recursive parsing process. The initial call sets up a document cache and schema registry, which are used to minimise unnecessary work and to help resolve cycles. Then it calls `resolveThenPopulateSchema()`, which is where the real work begins.

## Dialects

`SchemaParser` is constructed with a `Version` value. The default is `kDraft7`, and the parser also supports `kDraft3`, `kDraft4`, and the experimental `kDraft202012` mode.

The dialect changes how some keywords are parsed:

* Draft 3 treats property-level `required` and `extends` as legacy syntax.
* Draft 7 and Draft 2020-12 accept boolean schemas and Draft 7-era keywords such as `contains`, `const`, `if`, `then`, and `else`.
* Draft 2020-12 reads `$id`, treats `$defs` as the replacement for `definitions`, and aliases legacy `#/definitions/...` references to `#/$defs/...` while this mode is active.
* Draft 2020-12 gives array applicators their newer meaning: `prefixItems` is used for tuple validation, `items` applies to elements after the tuple prefix, and `additionalItems` is ignored.

## Resolve Then Populate

This step is a little more complicated. This occurs in `resolveThenPopulateSchema()`, which is declared as:

```c++
template<typename AdapterType>
void resolveThenPopulateSchema(
    Schema &rootSchema,
    const AdapterType &rootNode,
    const AdapterType &node,
    const Subschema &subschema,
    const opt::optional<std::string> currentScope,
    const std::string &nodePath,
    const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
    const Subschema *parentSchema,
    const std::string *ownName,
    typename DocumentCache<AdapterType>::Type &docCache,
    SchemaRegistry &schemaRegistry)
```

The first step is to check for JSON References. These are objects of the form:

```json
{
  "$ref": "..."
}
```
where the `...` could be a URL, a JSON Pointer, or some other kind of special value. JSON References can point to external documents, or reference other parts of the current document. The simple case is when a JSON Reference is NOT found. We can simply proceed to populate the schema with a call to the recursive form of `populateSchema()`.

If a JSON Reference is found, we must first resolve it. This may involve using the `fetchDoc` callback to retrieve an external document, or using the current document if this is a relative reference. External documents will be stored in the document cache, which is described in the [Document Cache](#document-cache) section below.

Resolving a JSON Reference may require a recursive call to `resolveThenPopulateSchema()`.

## Document Cache

The document cache is a map from resolved document URI to the external document returned by the `fetchDoc` callback. It is only used when remote document fetching is enabled. Fetching must be enabled by providing both `fetchDoc` and `freeDoc`; providing only one of them is rejected before parsing begins.

When the parser resolves a `$ref` to a document URI outside the current scope, it first checks this cache. A cache miss calls `fetchDoc`, stores the returned document pointer, and continues parsing through that document's adapter type. After parsing succeeds or throws, the cache is released with `freeDoc`.

The document cache owns external documents for the lifetime of one top-level `populateSchema()` call. It does not cache local subschemas; local reuse is handled by the schema registry.

## Schema Registry

The schema registry maps canonical lookup keys to populated `Subschema` instances. Keys are built from the current resolution scope and JSON Pointer path, or from resolved `$ref` targets. The registry prevents duplicate parsing and breaks reference cycles by allowing later lookups to reuse an existing `Subschema`.

`makeOrReuseSchema()` records registry keys encountered while chasing `$ref` chains. Once it reaches a concrete schema node, all pending keys are registered against the concrete `Subschema`. If the concrete node was already registered, the existing `Subschema` is reused instead of creating and populating a duplicate.

The registry is intentionally stricter than a plain `std::map` lookup. `querySchemaRegistry()` never creates missing entries, and `updateSchemaRegistry()` throws if a key is registered twice. Duplicate registration indicates parser bookkeeping has gone wrong rather than malformed user input.

## Populate Schema

The next step in parsing a schema is a recursive call to `populateSchema()`. The recursive version of this function is declared as:

```c++
template<typename AdapterType>
void populateSchema(
    Schema &rootSchema,
    const AdapterType &rootNode,
    const AdapterType &node,
    const Subschema &subschema,
    const opt::optional<std::string>& currentScope,
    const std::string &nodePath,
    const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
    const Subschema *parentSubschema,
    const std::string *ownName,
    typename DocumentCache<AdapterType>::Type &docCache,
    SchemaRegistry &schemaRegistry)
```

This is a huge function that searches for all of the supported JSON Schema rules (referred to in Valijson as 'constraints'). When a supported rule is found, it is parsed and instantiated as subclass of the `Constraint` class.

In Draft 2020-12 mode, `populateSchema()` also switches the array-keyword path. Instead of treating an array-valued `items` as tuple validation, it builds a `LinearItemsConstraint` from `prefixItems`, uses `items` as the schema for remaining elements, and falls back to `unevaluatedItems` when no `items` schema is present. `additionalItems` is not consulted in this mode.

## Constraints

Constraints are Valijson's internal representation of JSON Schema validation keywords, which can later be applied when validating a document. An example would be the `required` keyword. From JSON Schema Draft 4 onwards the value associated with this keyword is an array of property names that must be present on an object being validated.

Constraints are typically implemented as simple data objects, which are designed to be _visited_ as per the [Visitor Pattern](https://en.wikipedia.org/wiki/Visitor_pattern). Validation is handled by the `ValidationVisitor` class. All of this is described in more detail in the [Validation](validation.md) design doc.

### Construction

Construction of a constraint is usually performed by a helper function, such as `makeRequiredConstraint` for the `required`. Almost all of these helper functions accept a reference to a JSON document node (wrapped by an Adapter), so that they can extract the relevant properties. `makeRequiredConstraint` is declared as:

```c++
template<typename AdapterType>
constraints::RequiredConstraint makeRequiredConstraint(
    const AdapterType &node)
```

### Subschemas

This all becomes more complicated when handling validation keywords that require a sub-schema, such as `properties`, `additionalProperties`, or `patternProperties`. This is where the recursive nature of schema parsing comes in.

The first step in parsing a sub-schema is a call to `makeOrReuseSchema()`, declared as:

```c++
template<typename AdapterType>
const Subschema * makeOrReuseSchema(
    Schema &rootSchema,
    const AdapterType &rootNode,
    const AdapterType &node,
    const opt::optional<std::string> currentScope,
    const std::string &nodePath,
    const typename FunctionPtrs<AdapterType>::FetchDoc fetchDoc,
    const Subschema *parentSubschema,
    const std::string *ownName,
    typename DocumentCache<AdapterType>::Type &docCache,
    SchemaRegistry &schemaRegistry)
```

The return value is a `Subschema *`, which may be retrieved from the schema registry.

For Draft 2020-12 array keywords, `makeDraft202012ItemsConstraint()` reuses `LinearItemsConstraint` with a mode flag that records whether the schema for remaining array elements came from `items` or `unevaluatedItems`. This keeps validation compatible with the existing linear tuple machinery while preserving enough information for error messages and basic evaluated-item tracking.

The current `unevaluatedItems` implementation is intentionally limited: it tracks the highest contiguous array index count evaluated by item applicators. That supports direct `prefixItems` cases and simple annotation flow such as `allOf`, but it does not model the full Draft 2020-12 annotation semantics for all combiners, `$ref`, `$dynamicRef`, or `contains` interactions.
