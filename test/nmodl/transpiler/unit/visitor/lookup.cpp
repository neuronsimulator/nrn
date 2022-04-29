/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/visitor_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;
using symtab::syminfo::NmodlType;


//=============================================================================
// Ast lookup visitor tests
//=============================================================================

SCENARIO("Searching for ast nodes using AstLookupVisitor", "[visitor][lookup]") {
    auto to_ast = [](const std::string& text) {
        NmodlDriver driver;
        return driver.parse_string(text);
    };

    GIVEN("A mod file with nodes of type NEURON, RANGE, BinaryExpression") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, h
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
                h' = h + 2
            }

            : My comment here
        )";

        auto ast = to_ast(nmodl_text);

        WHEN("Looking for existing nodes") {
            THEN("Can find RANGE variables") {
                const auto& result = collect_nodes(*ast, {AstNodeType::RANGE_VAR});
                REQUIRE(result.size() == 2);
                REQUIRE(to_nmodl(result[0]) == "tau");
                REQUIRE(to_nmodl(result[1]) == "h");
            }

            THEN("Can find NEURON block") {
                const auto& nodes = collect_nodes(*ast, {AstNodeType::NEURON_BLOCK});
                REQUIRE(nodes.size() == 1);

                const std::string neuron_block = R"(
                    NEURON {
                        RANGE tau, h
                    })";
                const auto& result = reindent_text(to_nmodl(nodes[0]));
                const auto& expected = reindent_text(neuron_block);
                REQUIRE(result == expected);
            }

            THEN("Can find Binary Expressions and function call") {
                const auto& result =
                    collect_nodes(*ast,
                                  {AstNodeType::BINARY_EXPRESSION, AstNodeType::FUNCTION_CALL});
                REQUIRE(result.size() == 4);
            }
        }

        WHEN("Looking for missing nodes") {
            THEN("Can not find BREAKPOINT block") {
                const auto& result = collect_nodes(*ast, {AstNodeType::BREAKPOINT_BLOCK});
                REQUIRE(result.empty());
            }
        }
    }
}
