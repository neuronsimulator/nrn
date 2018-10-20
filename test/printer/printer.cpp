#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "printer/json_printer.hpp"

TEST_CASE("JSON Printer Tests", "[JSONPrinter]") {
    SECTION("Stringstream test 1") {
        std::stringstream ss;
        JSONPrinter p(ss);
        p.compact_json(true);

        p.push_block("A");
        p.add_node("B");
        p.pop_block();
        p.flush();

        auto result = R"({"A":[{"value":"B"}]})";
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

        auto result = R"({"A":[{"value":"B"},{"value":"C"},{"D":[{"value":"E"}]}]})";
        REQUIRE(ss.str() == result);
    }
}
