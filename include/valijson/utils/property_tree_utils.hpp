#ifndef __VALIJSON_UTILS_PROPERTY_TREE_UTILS_HPP
#define __VALIJSON_UTILS_PROPERTY_TREE_UTILS_HPP

#include <sstream>

#include <json/json.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/detail/json_parser_error.hpp>

#include <valijson/utils/file_utils.hpp>

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, boost::property_tree::ptree &document)
{
    // Load schema JSON from file
    std::string file;
    if (!loadFile(path, file)) {
        std::cerr << "Failed to load json from file '" << path << "'." << std::endl;
        return false;
    }

    std::istringstream is(file);
    try {
        boost::property_tree::read_json(is, document);
    } catch (boost::property_tree::json_parser::json_parser_error &e) {
        std::cerr << "Boost Property Tree JSON parser failed to parse the document:" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }

    return true;
}

}  // namespace utils
}  // namespace valijson

#endif
