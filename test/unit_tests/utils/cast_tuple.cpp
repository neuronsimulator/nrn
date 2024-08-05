#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "utils.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("can_cast", "[Neuron][nrnpython]") {
    auto args = nanobind::make_tuple(42, "test", 3.14);
    auto [i, s, d] = nrn::cast_tuple<int, std::string, double>(args);
    REQUIRE(i == 42);
    REQUIRE(s == "test");
    REQUIRE(d == 3.14);
}

TEST_CASE("can_cast_extra_arguments", "[Neuron][nrnpython]") {
    auto args = nanobind::make_tuple("test", 3.14, 42);
    auto [s, d] = nrn::cast_tuple<std::string, double>(args);
    REQUIRE(s == "test");
    REQUIRE(d == 3.14);
}

TEST_CASE("not_enough_arguments", "[Neuron][nrnpython]") {
    auto args = nanobind::make_tuple(42, std::string("test"));
    REQUIRE_THROWS(nrn::cast_tuple<int, std::string, double>(args));

    auto empty_tuple = nanobind::tuple();
    REQUIRE_THROWS(nrn::cast_tuple<int>(empty_tuple));
}
