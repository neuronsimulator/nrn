#include <catch2/catch_test_macros.hpp>

#define UNIT_TESTING
#include "oc/wrap_sprintf.h"

using neuron::SprintfAsrt;

TEST_CASE("buf fits", "[NEURON]") {
    char buf[20];
    SprintfAsrt(buf, "%s", "hello");
    REQUIRE(strcmp(buf, "hello") == 0);
}

TEST_CASE("buf too small", "[NEURON]") {
    char buf[5];
    char* s = strdup("hello");
    REQUIRE_THROWS(SprintfAsrt(buf, "%s", s));
    free(s);
}
