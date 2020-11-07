#pragma once

#include <iostream>
#include <sstream>

#include <boost/property_tree/ptree.hpp>

#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Wshadow"
# include <boost/property_tree/json_parser.hpp>
# pragma clang diagnostic pop
#else
# include <boost/property_tree/json_parser.hpp>
#endif

#include <boost/assert/source_location.hpp>
#include <boost/config/detail/suffix.hpp>
#include <valijson/utils/file_utils.hpp>
#include <valijson/exceptions.hpp>

#if !VALIJSON_USE_EXCEPTION

namespace boost {

// Boost requires used-defined exception throwers when exceptions are
// disabled.
BOOST_NORETURN void throw_exception(std::exception const & e ) {
 valijson::throwRuntimeError(e.what());
}

BOOST_NORETURN void throw_exception(std::exception const & e, boost::source_location const & loc ) {
  valijson::throwRuntimeError(e.what());
}

}  // namespace boost

#endif

namespace valijson {
namespace utils {

inline bool loadDocument(const std::string &path, boost::property_tree::ptree &document)
{
#if !defined(BOOST_NO_EXCEPTIONS)
    try {
#endif
        boost::property_tree::read_json(path, document);
#if !defined(BOOST_NO_EXCEPTIONS)
    } catch (std::exception &e) {
        std::cerr << "Boost Property Tree JSON parser failed to parse the document:" << std::endl;
        std::cerr << e.what() << std::endl;
        return false;
    }
#endif

    return true;
}

}  // namespace utils
}  // namespace valijson
