#include <nanobind/nanobind.h>
#include <nanobind/stl/string.h>

#include "cast_tuple.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("can_cast_nanobind_tuple", "[Neuron][nrnpython]") {
    auto tup = nanobind::make_tuple(42, "test", 3.14);
    auto [i, s, d] = nrn::cast_tuple<int, std::string, double>(tup);
    REQUIRE(i == 42);
    REQUIRE(s == "test");
    REQUIRE(d == 3.14);
}

TEST_CASE("can_cast_nanobind_args", "[Neuron][nrnpython]") {
    auto tup = nanobind::make_tuple(42, "test", 3.14);
    auto args = nanobind::cast<nanobind::args>(tup);
    auto [i, s, d] = nrn::cast_tuple<int, std::string, double>(args);
    REQUIRE(i == 42);
    REQUIRE(s == "test");
    REQUIRE(d == 3.14);
}

TEST_CASE("can_cast_extra_arguments", "[Neuron][nrnpython]") {
    auto tup = nanobind::make_tuple("test", 3.14, 42);
    auto [s, d] = nrn::cast_tuple<std::string, double>(tup);
    REQUIRE(s == "test");
    REQUIRE(d == 3.14);
}

TEST_CASE("not_enough_arguments", "[Neuron][nrnpython]") {
    auto tup = nanobind::make_tuple(42, std::string("test"));
    REQUIRE_THROWS(nrn::cast_tuple<int, std::string, double>(tup));

    auto empty_tuple = nanobind::tuple();
    REQUIRE_THROWS(nrn::cast_tuple<int>(empty_tuple));
}

TEST_CASE("can_cast_objects", "[Neuron][nrnpython]") {
    nanobind::object ob = nanobind::none();
    nanobind::list li{};
    li.append(1);
    auto tup = nanobind::make_tuple("test", ob, li, 42);
    auto [s, o, l, i] = nrn::cast_tuple<std::string, nanobind::handle, nanobind::list, int>(tup);
    REQUIRE(s == "test");
    REQUIRE(o.is_none());
    REQUIRE(l.is_valid());
    REQUIRE(nanobind::list::check_(l));
    REQUIRE(static_cast<int>(static_cast<nanobind::int_>(l[0])) == 1);
    REQUIRE(i == 42);
}
