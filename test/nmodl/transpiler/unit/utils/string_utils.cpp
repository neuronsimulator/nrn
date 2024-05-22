#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "utils/string_utils.hpp"

using namespace nmodl;

TEST_CASE("ends_with") {
    SECTION("empty substring") {
        REQUIRE(stringutils::ends_with("abcde", ""));
    }

    SECTION("empty str") {
        REQUIRE(!stringutils::ends_with("", "abc"));
    }

    SECTION("both empty") {
        REQUIRE(stringutils::ends_with("", ""));
    }
    SECTION("match") {
        REQUIRE(stringutils::ends_with("abcde", "de"));
    }

    SECTION("mismatch") {
        REQUIRE(!stringutils::ends_with("abcde", "d"));
    }

    SECTION("oversized") {
        REQUIRE(!stringutils::ends_with("abcde", "--abcde"));
    }
}
