# Adapters

This document explains the design decisions that motivated Valijson's adapter architecture. Adapters allow Valijson to work with a multitude of JSON parsers. At time of writing, the list of supported parsers includes:

* Boost JSON
* Boost Property Trees
* JSON11
* JsonCpp
* NlohmannJSON
* PicoJSON
* PocoJSON
* QtJSON
* RapidJSON
* YamlCpp

This architecture has proven to be versatile and easy for new contributors to work with, with many of these implementations being submitted by first-time contributors.

## Goals

The initial design goals for Valijson were:

* **Performance** - To have performance that is competitive with hand-written validators. At the time (~2012), there weren't many options for validating JSON Schema in C++. My team was writing validation code by hand, using otherwise complete JSON Schemas as a reference.

* **Support multiple parsers** - Valijson was originally designed to work with three parsers: JsonCpp, RapidJSON and Boost Property Trees. These were the parsers used by my employer at the time, across various projects. I wanted to use the same validator regardless of which JSON parser a particular project used.

* **Header only** - Header only libraries have the advantage of being easy to integrate into existing code-bases. I wanted to reduce the friction for adopting Valijson.

## Architecture

Achieving high performance while supporting multiple parsers meant that static dispatch would be preferrable (as opposed to dynamic dispatch). Thus, Valijson makes heavy use of C++ Class Templates to allow composability.

### Class Templates

Class Templates have the advantage of being optimisation friendly, as the compiler can "see through" potentially costly code paths.

Central to Valijson's architecture is the _Parser Adapter_. A Parser Adapter (or just Adapter for short) provides a facade for a specific JSON parser library, ensuring that it conforms to an interface expected by Valijson's JSON Schema validator. This could be achieved using polymorphism, but I wanted to avoid dynamic dispatch.

Class Templates were a natural fit for this architecture, allowing Adapters to conform to the interface, without the overhead of dynamic dispatch.

### Alternative: Code Generation

There was one other approach that I considered, which would largely satisfy the same design goals. This approach was Code Generation, whereby Valijson would provide a Validator Generator script.

This was ultimately ruled out due to implementation complexity, and the difficulty of introducing Adapters for new JSON Parser implementations.

It also had the disadvantage of requiring a seperate build step to generate code for validators, and therefore no option for dynamic schemas.

## Implementation

_What is the process for implementing a new parser adapter?_

The implementation for each parser is fully contained in a single header file, named accordingly. For example:

* `picojson_adapter.hpp` implements an adapter for PicoJSON
* `rapidjson_adapter.hpp` implements an adapter for RapidJSON
* and so on...

For general use, each parser adapter provides a corresponding `utils` file. e.g. `rapidjson_utils.hpp`. This file contains a specialisation of the `loadDocument` template function, catered towards loading documents using that specific parser.

The purpose of this is to make it easier to read JSON documents from disk:

```cpp
    rapidjson::Document document;
    if (!valijson::utils::loadDocument(filename, document)) {
        std::cerr << "Failed to load document " << filename << std::endl;
        return;
    }
```

### Steps

Here's the general process you would follow:

1. **Learn.** The first thing to do when implementing an adapter is to become familiar with the API for the target parser. The key areas to focus on are object and array iteration, how different value types are identified, and any memory management constraints / requirements.

2. **Copy an existing adapter.** Most adapter implementations begin by using an existing adapter as a template. Start by picking a similar parser (according to the knowledge gained in step 1).

3. **Choose a name for the parser.** To make the new adapter compile, you can perform a find+replace to change the name of all the supporting classes. e.g. if you copied `rapidjson_adapter.hpp` to start implementing a [Parson JSON](https://github.com/kgabis/parson) adapter, you would replace all class/struct names that include `RapidJson` with `ParsonJson`.

4. **Copy an existing utils file.** All you need to do is implement `loadDocument` for your parser. The purpose of this is to make it easier to read JSON documents from disk.

4. **Setup a test harness.** The best place to start is the `examples` directory. The `check_schema.cpp` example app is a great template. You can make a copy of this that works with your new adapter. You'll need to add this to `CMakeLists.txt` to ensure that it compiles

5. **Start filling in the adapter...** otherwise known as "draw the rest of the **** owl". Working from top-to-bottom in your new `x_adapter.hpp` file, make the modifications necessary to compile with your target parser library.

### Nice-to-haves

If you are submitting a new adapter for inclusion in Valijson, tests are a must. You'll want to use one of the existing `test_x_adapter.hpp` files as a reference, updating it to work with your new adapter.

## Flaws

Once I had chosen to use C++ Class Templates to implement an Adapter for each target JSON Parser, the rest of the design evolved organically.

### Dynamic Dispatch

TODO: Document cases where

### Frozen Values

TODO: Document how frozen values are somewhat awkward.
