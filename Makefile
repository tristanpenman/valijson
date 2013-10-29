
CPPFLAGS = \
	-ggdb \
	-O0 \
	-Iinclude \
	-Ithirdparty/gtest-1.6.0 \
	-Ithirdparty/gtest-1.6.0/src \
	-Ithirdparty/gtest-1.6.0/include \
	-Ithirdparty/jsoncpp-0.5.0/include \
	-Ithirdparty/rapidjson-0.1/include \
	-I/usr/local/include \
	-DBOOST_NO_CXX11_SMART_PTR \
	-DGTEST_USE_OWN_TR1_TUPLE \
	-L/usr/local/lib -lboost_regex-mt

TEST_SRCS = tests/test_adapter_comparison.cpp \
	tests/test_dereference_callback.cpp \
	tests/test_jsoncpp_adapter.cpp \
	tests/test_property_tree_adapter.cpp \
	tests/test_rapidjson_adapter.cpp \
	tests/test_uri_resolution.cpp \
	tests/test_validation_errors.cpp \
	tests/test_validator.cpp

JSONCPP_SRCS = \
	thirdparty/jsoncpp-0.5.0/src/lib_json/json_reader.cpp \
	thirdparty/jsoncpp-0.5.0/src/lib_json/json_value.cpp \
	thirdparty/jsoncpp-0.5.0/src/lib_json/json_writer.cpp

GTEST_SRCS = \
	thirdparty/gtest-1.6.0/src/gtest-death-test.cc \
	thirdparty/gtest-1.6.0/src/gtest-filepath.cc \
	thirdparty/gtest-1.6.0/src/gtest-port.cc \
	thirdparty/gtest-1.6.0/src/gtest-printers.cc \
	thirdparty/gtest-1.6.0/src/gtest-test-part.cc \
	thirdparty/gtest-1.6.0/src/gtest-typed-test.cc \
	thirdparty/gtest-1.6.0/src/gtest.cc \
	thirdparty/gtest-1.6.0/src/gtest_main.cc

.PHONY: check

check: bin bin/test_suite
	cd bin && ./test_suite

examples: bin bin/custom_schema bin/external_schema

bin:
	mkdir -p bin

bin/custom_schema:
	g++ $(CPPFLAGS) examples/custom_schema.cpp -o bin/custom_schema

bin/external_schema:
	g++ $(CPPFLAGS) examples/external_schema.cpp -o bin/external_schema

bin/test_suite:
	g++ $(CPPFLAGS) $(TEST_SRCS) $(JSONCPP_SRCS) $(GTEST_SRCS) -o bin/test_suite