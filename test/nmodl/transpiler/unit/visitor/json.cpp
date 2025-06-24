/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/visitor_utils.hpp"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

using namespace nmodl;
using namespace visitor;

using nmodl::parser::NmodlDriver;

//=============================================================================
// JSON visitor tests
//=============================================================================

std::string run_json_visitor(const std::string& text, bool compact = false) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    return to_json(*ast, compact);
}

TEST_CASE("Convert NMODL to AST to JSON form using JSONVisitor", "[visitor][json]") {
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
        std::string expected = R"({"Program":[{"NeuronBlock":[{"StatementBlock":[]}]}]})";

        auto result = run_json_visitor(nmodl_text, true);
        REQUIRE(result == expected);
    }
}
