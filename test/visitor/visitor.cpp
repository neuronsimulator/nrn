#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/verbatim_visitor.hpp"
#include "visitors/json_visitor.hpp"

#include "json/json.hpp"

using json = nlohmann::json;

//=============================================================================
// Verbatim visitor tests
//=============================================================================

std::vector<std::string> run_verbatim_visitor(std::string text) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    VerbatimVisitor v;
    v.visitProgram(ast.get());
    return v.verbatim_blocks();
}

TEST_CASE("Verbatim Visitor") {
    SECTION("Single Block") {
        std::string text = "VERBATIM int a; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 1);
        REQUIRE(blocks.front().compare(" int a; ") == 0);
    }

    SECTION("Multiple Blocks") {
        std::string text = "VERBATIM int a; ENDVERBATIM VERBATIM float b; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 2);
        REQUIRE(blocks[0].compare(" int a; ") == 0);
        REQUIRE(blocks[1].compare(" float b; ") == 0);
    }
}

//=============================================================================
// JSON visitor tests
//=============================================================================

std::string run_json_visitor(std::string text, bool compact = false) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    std::stringstream ss;
    JSONVisitor v(ss);

    /// if compact is true then we get compact json output
    v.compact_json(compact);

    v.visitProgram(ast.get());
    return ss.str();
}

TEST_CASE("JSON Visitor") {
    SECTION("JSON object test") {
        std::string nmodl_text = "NEURON {}";
        json expected = R"(
            {
              "Program": [
                {
                  "NeuronBlock": [
                    {
                      "StatementBlock": []
                    }
                  ]
                }
              ]
            }
        )"_json;

        auto json_text = run_json_visitor(nmodl_text);
        json result = json::parse(json_text);

        REQUIRE(expected == result);
    }

    SECTION("JSON text test (compact format)") {
        std::string nmodl_text = "NEURON {}";
        std::string expected = "{\"Program\":[{\"NeuronBlock\":[{\"StatementBlock\":[]}]}]}";

        auto result = run_json_visitor(nmodl_text, true);
        REQUIRE(result == expected);
    }
}