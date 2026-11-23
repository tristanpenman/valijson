#pragma once

#include <iostream>
#include <stdexcept>

#include <rapidjson/document.h>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

template<typename Encoding, typename Allocator>
inline bool loadDocument(const std::string &path, rapidjson::GenericDocument<Encoding, Allocator> &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'." << std::endl;
        return false;
    }

    // Parse schema
#if VALIJSON_USE_EXCEPTIONS
    try {
#endif
        document.template Parse<rapidjson::kParseIterativeFlag>(file.c_str());
        if (document.HasParseError()) {
            std::cerr << "RapidJson failed to parse the document:" << std::endl;
            std::cerr << "Parse error: " << document.GetParseError() << std::endl;
            // Show up to 20 characters either side of the error, taking care
            // not to underflow when the error is near the start of the file
            const size_t errorOffset = document.GetErrorOffset();
            const size_t contextStart = errorOffset > 20 ? errorOffset - 20 : 0;
            std::cerr << "Near: " << file.substr(contextStart, 40) << std::endl;
            return false;
        }
#if VALIJSON_USE_EXCEPTIONS
    } catch (const std::runtime_error &e) {
        std::cerr << "RapidJson failed to parse the document:" << std::endl;
        std::cerr << "Runtime error: " << e.what() << std::endl;
        return false;
    }
#endif

    return true;
}

}  // namespace utils
}  // namespace valijson
