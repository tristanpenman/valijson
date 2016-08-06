#!/bin/bash

set -e   # (errexit) Exit if any subcommand or pipeline returns a non-zero status
set -u   # (nounset) Exit on any attempt to use an uninitialised variable

: ${CXX:=}
: ${TRAVIS:=false}

echo -n "Checking that we're running on Travis CI..."
if [[ $TRAVIS == 'true' ]]; then
	echo " OK"
else
	echo " Nope."
	echo "This script is only intended for use on Travis CI."
	echo "Set the TRAVIS environment variable to 'true' to override."
	exit 1
fi

mkdir -p build
pushd build > /dev/null

# Boost does not correctly identify version of libstdc++ used by clang
CMAKE_FLAGS=
if [[ $CXX == 'clang++' ]]; then
	CMAKE_FLAGS=-DCMAKE_CXX_FLAGS=-DBOOST_NO_CXX11_ALLOCATOR
	echo "Additional flags to pass to cmake: $CMAKE_FLAGS"
fi

echo "Attempting to build and run test suite with C++11 support enabled..."
cmake $CMAKE_FLAGS -DVALIJSON_CXX11_ADAPTERS=enabled ..
VERBOSE=1 make
./test_suite
make clean

popd > /dev/null

