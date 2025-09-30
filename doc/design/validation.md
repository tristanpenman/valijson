# Validation

## Visitor Pattern

The Visitor Pattern was chosen to ensure that validation could be implemented independent of any specific parser.

This pattern fits naturally with the hierarchical and recursive nature of JSON documents (and schemas).

If you are unfamiliar with the Visitor Pattern, a prototypical example has been included in [visitor.cpp](visitor.cpp).

## Schema Validation

You have a stable class hierarchy and need to perform many unrelated operations.

You want to keep operations decoupled from the objects they operate on.

## Validation Errors

Validation is designed to fail-fast by default. If `Validator::validate` is invoked without a `ValidationResults` instance in which to record validation errors, traversal will stop as soon as a constraint cannot be satisfied and the method will return `false`. If you want to accumulate all of the failures that were encountered, pass a
`ValidationResults` instance to `validate`.

`ValidationResults` maintains a FIFO queue of `Error` objects. Each reporting error contains a free-form description of the problem as a `context` vector describing where the failure occurred in the input document. The context always begins with `"<root>"`; array indices and object properties are appended using bracket notation (for example, `"[0]"` for the first array element or `"[title]"` for a property). The caller can
reconstruct a human readable path to the failing value.

After validation, errors can be consumed in the order that they were reported by either iterating over the container or repeatedly calling `popError` until it returns `false`.

The snippet below mirrors the approach taken in the example programs bundled with Valijson:

```c++
valijson::Validator validator;
valijson::ValidationResults results;
if (!validator.validate(schema, adapter, &results)) {
    valijson::ValidationResults::Error error;
    while (results.popError(error)) {
        std::cout << "At ";
        for (const auto &segment : error.context) {
            std::cout << segment << ' ';
        }
        std::cout << "- " << error.description << std::endl;
    }
}
```

This design keeps validation logic decoupled from presentation requirements. Users of the library are free to render error paths and descriptions in the format that best suits their needs.
