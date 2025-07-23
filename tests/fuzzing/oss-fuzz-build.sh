#!/bin/bash -eu

git submodule update --init --depth 1 thirdparty

mkdir build
cd build
cmake \
  -Dvalijson_BUILD_TESTS=TRUE \
  -Dvalijson_BUILD_EXAMPLES=FALSE \
  -Dvalijson_EXCLUDE_BOOST=TRUE \
  ..

make -j"$(nproc)"

cd ../tests/fuzzing

# CXXFLAGS may contain spaces
# shellcheck disable=SC2086
"$CXX" $CXXFLAGS "$LIB_FUZZING_ENGINE" \
    -DVALIJSON_USE_EXCEPTIONS=1 \
	-I/src/valijson/thirdparty/rapidjson/include \
	-I/src/valijson/include \
	fuzzer.cpp -o "${OUT}/fuzzer"

mkdir seed_corpus

find "${SRC}/valijson/thirdparty/JSON-Schema-Test-Suite/tests" -name "*.json" | while read file; do
    sha1=$(sha1sum "$file" | awk '{print $1}')
    cp "$file" seed_corpus/"${sha1}"
done

zip -j -r "${OUT}/fuzzer_seed_corpus.zip" seed_corpus
