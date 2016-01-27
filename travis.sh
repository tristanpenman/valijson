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
pushd build

echo "Attempting to build and run test suite with C++11 support disabled..."
cmake -DVALIJSON_CXX11_ADAPTERS=disabled ..
make
./test_suite

echo "Checking if current compiler is GCC..."
if [[ $CXX == 'g++' ]]; then
	echo "Not building test suite with C++11 support due to ancient version of GCC on Travis CI"
else
	echo "Attempting to build and run test suite with C++11 support enabled..."
	make clean
	cmake -DVALIJSON_CXX11_ADAPTERS=enabled ..
	make
	./test_suite
fi

make clean

popd
