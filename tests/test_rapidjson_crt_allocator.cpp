#define RAPIDJSON_DEFAULT_ALLOCATOR rapidjson::CrtAllocator

#include <type_traits>

#include <valijson/adapters/rapidjson_adapter.hpp>

int main()
{
    using Adapter = valijson::adapters::RapidJsonAdapter;
    using Document = valijson::adapters::AdapterTraits<Adapter>::DocumentType;

    static_assert(std::is_same_v<Document, rapidjson::Document>);

    Document document;
    Adapter adapter(document);
    return adapter.isNull() ? 0 : 1;
}
