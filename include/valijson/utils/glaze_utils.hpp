#pragma once

#include <iostream>
#include <string>

#include <glaze/json/read.hpp>

// GlazeDocument resolves to glz::generic on Glaze >= 6.0.0, and to
// glz::json_t on older releases. That detection lives in the adapter, so it is
// included here rather than being duplicated.
#include <valijson/adapters/glaze_adapter.hpp>
#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, adapters::GlazeDocument &document)
{
    // Load JSON document from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'."
                  << std::endl;
        return false;
    }

    // Glaze reports parse failures via a return value rather than an
    // exception, so this works with or without VALIJSON_USE_EXCEPTIONS
    const auto error = glz::read_json(document, file);
    if (error) {
        std::cerr << "Glaze failed to parse the document\n"
                  << "Parse error: " << glz::format_error(error, file) << "\n";
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson
