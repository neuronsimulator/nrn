#define CATCH_CONFIG_MAIN

#include <string>

#include "catch.hpp"
#include "printer/json_printer.hpp"

TEST_CASE("JSON Printer Tests", "[JSONPrinter]") {
    SECTION("Stringstream test 1") {
        std::stringstream ss;
        JSONPrinter p(ss);
        p.compact_json(true);

        p.pushBlock("A");
        p.addNode("B");
        p.popBlock();
        p.flush();

        auto result = R"({"A":[{"value":"B"}]})";
        REQUIRE(ss.str() == result);
    }

    SECTION("Stringstream test 2") {
        std::stringstream ss;
        JSONPrinter p(ss);
        p.compact_json(true);

        p.pushBlock("A");
        p.addNode("B");
        p.addNode("C");
        p.pushBlock("D");
        p.addNode("E");
        p.popBlock();
        p.popBlock();
        p.flush();

        auto result = R"({"A":[{"value":"B"},{"value":"C"},{"D":[{"value":"E"}]}]})";
        REQUIRE(ss.str() == result);
    }
}
