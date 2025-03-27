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

* **Performance** - To have performance that is competitive with hand-written validators. At the time (~2012), there weren't many options for validating JSON Schema in C++. My team was writing validation code by hand, using JSON Schema documents as a reference.

* **Support multiple parsers** - Valijson was originally designed to work with three parsers: JsonCpp, RapidJSON and Boost Property Trees. These were the parsers used by my employer at the time. I wanted to use the same validator regardless of which JSON parser a particular project used.

* **Header only** - Header only libraries have the advantage of being easy to integrate into existing code-bases. I wanted to reduce the friction for adopting Valijson.

## Architecture

Achieving high performance while supporting multiple parsers meant that static dispatch would be preferrable (as opposed to dynamic dispatch). Thus, Valijson makes heavy use of C++ Class Templates to allow composability. This could be seen as a variation of [Policy-based Design](https://en.wikipedia.org/wiki/Modern_C%2B%2B_Design#Policy-based_design), as popularised by Andrei Alexandrescu.

### Class Templates

Class Templates have the advantage of being optimisation friendly, as the compiler can "see through" potentially costly code paths.

Central to Valijson's architecture is the _Parser Adapter_. A Parser Adapter (or just Adapter for short) provides a facade for a specific JSON parser library, ensuring that it conforms to an interface used by Valijson's JSON Schema validator. While this could be achieved using polymorphism (or [dynamic dispatch](https://en.wikipedia.org/wiki/Dynamic_dispatch)), I wanted to avoid the overhead associated dynamic dispatch.

C++ Class Templates were therefore a natural fit for this architecture, allowing Adapters to conform to a common interface. The validator could then be implemented in terms of this interface.

Note: This makes it sound like the Adapter interface was designed up-front. In reality, the Adapter interface evolved alongside the validator implementation.

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

For convenience, each parser adapter provides a corresponding `utils` file. e.g. `rapidjson_utils.hpp`. This file contains a specialisation of the `loadDocument` template function, making it easier to load documents using that specific parser.

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

1. **Learn the API.** The first thing to do when implementing an adapter is to become familiar with the API for the target parser. The key areas to focus on are object and array iteration, how different value types are identified, and any memory management constraints / requirements.

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

The heavy use of templates in Valijson delivers good performance, but there are corners of the design where pure static dispatch simply cannot express what the library needs to do. A prime example arises when schema keywords such as `enum` or `const` capture JSON values during schema compilation and later compare them against documents that may be backed by a different parser. The validator cannot know at compile time which adapter implementation will be providing those runtime values, so the strictly templated approach breaks down as soon as a constraint needs to hold onto opaque data and interact with it through a uniform interface.

To bridge that gap the library leans on a thin layer of dynamic dispatch. Each adapter is still responsible for the fast, statically-dispatched accessors used during validation, but it also exposes a virtual `freeze` operation that performs type erasure for the captured data. The resulting objects can then be stored and revisited without any knowledge of the originating adapter type, letting the validator compare frozen schema values against live document values while keeping the performance-critical paths template-based.

### Frozen Values

Frozen values are type-erased snapshots of JSON data that constraints capture from a schema. When the schema parser sees an `enum`, `const`, or similar keyword, it asks the adapter to `freeze` the underlying value and receives a heap-allocated object derived from `FrozenValue`. That object owns the data from that onward. This is decoupled from the parser-specific DOM and exposes a minimal virtual interface so that later comparisons can query the stored JSON without caring about which parser originally produced it.

Frozen values addressed the limitations of static dispatch for cross-adapter comparisons, but it also required the introduction of dynamic dispatch infrastructure within Valijson. Once validators started storing `FrozenValue` instances, they needed a polymorphic base class that could be cloned, compared, and destroyed via virtual functions. Every parser adapter must provide concrete `FrozenValue` implementation alongside its templated interface, and the validator code embraces those dynamic hooks wherever data must survive independently of any specific adapter.
