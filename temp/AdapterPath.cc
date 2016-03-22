/* Copyright (c) 2016 Akamai Technologies. All rights reserved. 

adapted from jsoncpp jsonpath code which appears to be in the public domain.
*/
#include "AdapterPath.h"

#include <sstream>
#include <stdexcept>
#include <string.h>

namespace valijson {
Path::Path(const std::string& path) {
    const char* current = path.c_str();
    const char* end = current + path.length();
    while (current != end) {
        if (*current == '[') {
            ++current;
            ArrayIndex index = 0;
            for (; current != end && *current >= '0' && *current <= '9';
                 ++current)
                index = index * 10 + ArrayIndex(*current - '0');
            args_.push_back(index);
            if (current == end || *current++ != ']')
                throw std::runtime_error("path missing ']'");
        } else if (*current == '.') {
            ++current;
        } else {
            const char* beginName = current;
            while (current != end && !strchr("[.", *current))
                ++current;
            args_.push_back(std::string(beginName, current));
        }
    }
}

std::ostream& operator<<(std::ostream& str, const PathArgument& path) {
    if (path.kind_ == path.kindIndex)
        str << '[' << path.index_ << ']';
    else
        str << path.key_ << '.';
    return str;
}

std::string Path::toString() const {
    std::stringstream str;
    for (auto arg : args_)
        str << arg;
    return str.str();
}

#if 0 // I haven't needed this yet, but it might well be useful in the future.
Value& Path::make(Adapter& root) const {
  Adapter* node = &root;
  for (Args::const_iterator it = args_.begin(); it != args_.end(); ++it) {
    const PathArgument& arg = *it;
    if (arg.kind_ == PathArgument::kindIndex) {
      if (!node->isArray()) {
        // Error: node is not an array at position ...
      }
      node = &((*node)[arg.index_]);
    } else if (arg.kind_ == PathArgument::kindKey) {
      if (!node->isObject()) {
        // Error: node is not an object at position...
      }
      node = &((*node)[arg.key_]);
    }
  }
  return *node;
}
#endif
}
