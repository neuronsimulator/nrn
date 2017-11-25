#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/verbatim_visitor.hpp"


std::vector<std::string> verbatim_visitor_process(std::string text) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    VerbatimVisitor v;
    v.visitProgram(ast.get());
    return v.verbatim_blocks();
}

/// test verbatim visitor
TEST_CASE("Verbatim Visitor") {
    SECTION("Single Block") {
        std::string text = "VERBATIM int a; ENDVERBATIM";
        auto blocks = verbatim_visitor_process(text);

        REQUIRE(blocks.size() == 1);
        REQUIRE(blocks.front().compare(" int a; ") == 0);
    }

    SECTION("Multiple Blocks") {
        std::string text = "VERBATIM int a; ENDVERBATIM VERBATIM float b; ENDVERBATIM";
        auto blocks = verbatim_visitor_process(text);

        REQUIRE(blocks.size() == 2);
        REQUIRE(blocks.front().compare(" int a; ") == 0);
        REQUIRE(blocks.back().compare(" float b; ") == 0);
    }
}