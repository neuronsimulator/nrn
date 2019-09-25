/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "catch/catch.hpp"

#include "parser/nmodl_driver.hpp"
#include "test/utils/test_utils.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
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

std::vector<std::shared_ptr<ast::Ast>> run_lookup_visitor(ast::Program* node,
                                                          std::vector<AstNodeType>& types) {
    return AstLookupVisitor().lookup(node, types);
}

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
                std::vector<AstNodeType> types{AstNodeType::RANGE_VAR};
                auto result = run_lookup_visitor(ast.get(), types);
                REQUIRE(result.size() == 2);
                REQUIRE(to_nmodl(result[0].get()) == "tau");
                REQUIRE(to_nmodl(result[1].get()) == "h");
            }

            THEN("Can find NEURON block") {
                AstLookupVisitor v(AstNodeType::NEURON_BLOCK);
                ast->accept(v);
                auto nodes = v.get_nodes();
                REQUIRE(nodes.size() == 1);

                std::string neuron_block = R"(
                    NEURON {
                        RANGE tau, h
                    })";
                auto result = reindent_text(to_nmodl(nodes[0].get()));
                auto expected = reindent_text(neuron_block);
                REQUIRE(result == expected);
            }

            THEN("Can find Binary Expressions and function call") {
                std::vector<AstNodeType> types{AstNodeType::BINARY_EXPRESSION,
                                               AstNodeType::FUNCTION_CALL};
                auto result = run_lookup_visitor(ast.get(), types);
                REQUIRE(result.size() == 4);
            }
        }

        WHEN("Looking for missing nodes") {
            THEN("Can not find BREAKPOINT block") {
                std::vector<AstNodeType> types{AstNodeType::BREAKPOINT_BLOCK};
                auto result = run_lookup_visitor(ast.get(), types);
                REQUIRE(result.size() == 0);
            }
        }
    }
}
