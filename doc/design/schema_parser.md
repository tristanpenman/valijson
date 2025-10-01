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

The initial call to `populateSchema()` doesn't do much, as it is primarily an entry point for a recursive parsing process. The initial call sets up document and schema caches, which are used to minimise unnecessary work and to help resolve cycles. Then it calls `resolveThenPopulateSchema()`, which is where the real work begins.

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
    SchemaCache &schemaCache)
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
    SchemaCache &schemaCache)
```

This is a huge function that searches for all of the supported JSON Schema rules (referred to in Valijson as 'constraints'). When a supported rule is found, it is parsed and instantiated as subclass of the `Constraint` class.

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
    SchemaCache &schemaCache)
```

The return value is a `Subschema *`, which may be retrieved from the schema cache (described below).
