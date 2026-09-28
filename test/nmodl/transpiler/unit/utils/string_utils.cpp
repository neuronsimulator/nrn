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

TEST_CASE("starts_with") {
    SECTION("empty substring") {
        REQUIRE(stringutils::starts_with("abcde", ""));
    }

    SECTION("empty str") {
        REQUIRE(!stringutils::starts_with("", "abc"));
    }

    SECTION("both empty") {
        REQUIRE(stringutils::starts_with("", ""));
    }
    SECTION("match") {
        REQUIRE(stringutils::starts_with("abcde", "ab"));
    }

    SECTION("mismatch") {
        REQUIRE(!stringutils::starts_with("abcde", "b"));
    }

    SECTION("oversized") {
        REQUIRE(!stringutils::starts_with("abcde", "abcde++"));
    }
}

TEST_CASE("join_arguments") {
    SECTION("both empty") {
        REQUIRE(stringutils::join_arguments("", "") == "");
    }

    SECTION("lhs emtpy") {
        REQUIRE(stringutils::join_arguments("", "foo, bar") == "foo, bar");
    }

    SECTION("rhs empty") {
        REQUIRE(stringutils::join_arguments("foo", "") == "foo");
    }

    SECTION("neither empty") {
        REQUIRE(stringutils::join_arguments("foo", "bar") == "foo, bar");
    }
}
