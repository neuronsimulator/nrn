/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <sstream>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "printer/json_printer.hpp"

using nmodl::printer::JSONPrinter;

TEST_CASE("JSON printer converting object to string form", "[printer][json]") {
    SECTION("Stringstream test 1") {
        std::stringstream ss;
        JSONPrinter p(ss);
        p.compact_json(true);

        p.push_block("A");
        p.add_node("B");
        p.pop_block();
        p.flush();

        auto result = R"({"A":[{"name":"B"}]})";
        REQUIRE(ss.str() == result);
    }

    SECTION("Stringstream test 2") {
        std::stringstream ss;
        JSONPrinter p(ss);
        p.compact_json(true);

        p.push_block("A");
        p.add_node("B");
        p.add_node("C");
        p.push_block("D");
        p.add_node("E");
        p.pop_block();
        p.pop_block();
        p.flush();

        auto result = R"({"A":[{"name":"B"},{"name":"C"},{"D":[{"name":"E"}]}]})";
        REQUIRE(ss.str() == result);
    }

    SECTION("Test with nodes as separate tags") {
        std::stringstream ss;
        JSONPrinter p(ss);
        p.compact_json(true);
        p.expand_keys(true);

        p.push_block("A");
        p.add_node("B");
        p.push_block("D");
        p.add_node("E");
        p.pop_block();
        p.flush();

        auto result =
            R"({"children":[{"name":"B"},{"children":[{"name":"E"}],"name":"D"}],"name":"A"})";
        REQUIRE(ss.str() == result);
    }
}
