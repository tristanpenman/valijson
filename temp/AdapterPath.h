#pragma once
/* Copyright (c) 2016 Akamai Technologies. All rights reserved.

adapted from jsoncpp jsonpath code which appears to be in the public domain.

I have simplified it by taking out "%d" argument interpolation, which
    - Isn't need for our purposes,
    - is complicated
    - can be replaced by the caller using snprintf
    - and had an odd limit of 5 arguments

Used exceptions so an invalid path can't be constructed.
removed buggy resolve without default value.
*/


/**
 * a very minimal implementation of jsonpath.
 * Does not include:
 *    * wildcard
 *    () expressions
 *    ?() filter expressions
 *    .. recursive descent
 *    | union operator
 *    [::]  array slice operator
 *
 *
 * Syntax of a path:
 * - "." => root node
 * - ".[n]" => elements at index 'n' of root node (an array value)
 * - ".name" => member named 'name' of root node (an object value)
 * - ".name1.name2.name3"
 * - ".[0][1][2].name1[3]"
 */

#include <stdexcept>
#include <string>
#include <vector>

namespace valijson {

typedef unsigned int ArrayIndex;

/* contains a single element of the path */
class PathArgument {
  public:
    friend class Path;

    PathArgument() = delete;

    PathArgument(ArrayIndex index) : kind_(kindIndex), index_(index) {}

    PathArgument(const char* key) : kind_(kindKey), key_(key) {}

    PathArgument(const std::string& key) : kind_(kindKey), key_(key.c_str()) {}

    friend std::ostream& operator<<(std::ostream& str,
        const PathArgument& path);

    enum Kind { kindNone = 0, kindIndex, kindKey };
    Kind kind_;
    std::string key_;
    ArrayIndex index_;
};

class ResolveError : public std::runtime_error {
  public:
    ResolveError(const char* err) : runtime_error(err) {}
};

std::ostream& operator<<(std::ostream& str, const PathArgument& path);

class Path {
  public:
    Path(const std::string& path);

    std::vector<PathArgument>::const_iterator begin() const {
        return args_.begin();
    }

    std::vector<PathArgument>::const_iterator end() const {
        return args_.end();
    }

    std::string toString() const;

    template <typename AdapterType>
    const AdapterType resolve(const AdapterType& root,
            size_t offset = 0) const {

        if (offset == args_.size())
            return root;

        
        const PathArgument& arg = args_[offset];
        if (arg.kind_ == PathArgument::kindIndex) {

            if (!root.isArray() || !root.getArraySize() > arg.index_)
                throw ResolveError("json path looking for index, document does "
                    "not contain array");

            auto array = root.getArray();
            return resolve<AdapterType>(array[arg.index_], offset + 1);

        } else if (arg.kind_ == PathArgument::kindKey) {

            if (!root.isObject())
                throw ResolveError(
                    "path contains key, document does not contain object");

            auto object = root.getObject();
            auto found(object.find(arg.key_));
            if (found != object.end())
                return resolve<AdapterType>(found->second, offset + 1);
            throw ResolveError("json path not found in document");

        } else {
            throw std::runtime_error("bad PathArgument kind");
        }
    }

    template <typename AdapterType>
    const std::string resolveErr(const AdapterType& root) const {
        try {
            resolve<AdapterType>(root);
            return "";
        } catch (ResolveError rex) {
            std::string estring("Failed to find " + toString() + " in input: "
                + rex.what());
            return estring;
        }
    }

  private:
    std::vector<PathArgument> args_;
};
}
