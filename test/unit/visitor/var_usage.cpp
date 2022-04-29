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
#include "visitors/var_usage_visitor.hpp"

using namespace nmodl;
using namespace visitor;

//=============================================================================
// Variable usage visitor tests
//=============================================================================

static bool run_var_usage_visitor(const std::shared_ptr<ast::Node>& node,
                                  const std::string& variable) {
    return VarUsageVisitor().variable_used(*node, variable);
}

SCENARIO("Searching for variable name using VarUsageVisitor", "[visitor][var_usage]") {
    auto to_ast = [](const std::string& text) {
        parser::NmodlDriver driver;
        return driver.parse_string(text);
    };

    GIVEN("A simple NMODL block") {
        std::string nmodl_text = R"(
            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            {
                h' = h + 2 + n
            }
            }
        )";

        auto ast = to_ast(nmodl_text);
        auto node = ast->get_blocks().front();

        WHEN("Looking for existing variable") {
            THEN("Can find variables") {
                REQUIRE(run_var_usage_visitor(node, "tau"));
                REQUIRE(run_var_usage_visitor(node, "h"));
                REQUIRE(run_var_usage_visitor(node, "n"));
            }
        }

        WHEN("Looking for missing variable") {
            THEN("Can not find variable") {
                REQUIRE_FALSE(run_var_usage_visitor(node, "tauu"));
                REQUIRE_FALSE(run_var_usage_visitor(node, "my_var"));
            }
        }
    }

    GIVEN("A nested NMODL block") {
        std::string nmodl_text = R"(
            NET_RECEIVE (weight,weight_AMPA, weight_NMDA, R){
                LOCAL result
                weight_AMPA = weight
                weight_NMDA = weight * NMDA_ratio
                INITIAL {
                    R = 1
                    u = u0
                    {
                        tsyn = t
                    }
                }
            }
        )";

        auto ast = to_ast(nmodl_text);
        auto node = ast->get_blocks().front();

        WHEN("Looking for existing variable in outer block") {
            THEN("Can find variables") {
                REQUIRE(run_var_usage_visitor(node, "weight"));
                REQUIRE(run_var_usage_visitor(node, "NMDA_ratio"));
            }
        }

        WHEN("Looking for existing variable in inner block") {
            THEN("Can find variables") {
                REQUIRE(run_var_usage_visitor(node, "R"));
                REQUIRE(run_var_usage_visitor(node, "u0"));
                REQUIRE(run_var_usage_visitor(node, "tsyn"));
            }
        }
    }
}
