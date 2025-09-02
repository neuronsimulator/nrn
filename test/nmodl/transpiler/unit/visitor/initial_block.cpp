/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
#include "visitors/initial_block_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"
#include "visitors/json_visitor.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace nmodl;
using nmodl::test_utils::reindent_text;

using Catch::Matchers::ContainsSubstring;  // ContainsSubstring in newer Catch2

//=============================================================================
// MergeInitialBlocks visitor tests
//=============================================================================

auto generate_mod_after_merge_initial_blocks_visitor(std::string const& text) {
    parser::NmodlDriver driver{};
    auto ast = driver.parse_string(text);
    visitor::MergeInitialBlocksVisitor{}.visit_program(*ast);
    return to_nmodl(*ast);
}

SCENARIO("Check multiple INITIAL blocks are merged properly", "[visitor][merge_initial_blocks]") {
    GIVEN("A mod file with multiple INITIAL blocks") {
        const auto nmodl_text_before = R"(
            NEURON {
                SUFFIX InitialBlockTest
                RANGE foo, bar
            }
            INITIAL {
                foo = 1
            }
            INITIAL {
                bar = 2
            }
        )";
        const auto nmodl_text_after = R"(
            NEURON {
              SUFFIX InitialBlockTest
              RANGE foo, bar
            }
            INITIAL {
              foo = 1
              bar = 2
            }
        )";
        parser::NmodlDriver driver{};
        auto ast_expected = driver.parse_string(nmodl_text_after);
        const auto program_expected = to_nmodl(ast_expected);
        const auto program_actual = generate_mod_after_merge_initial_blocks_visitor(
            nmodl_text_before);
        THEN("expected and actual should be identical at the level of the AST") {
            // TODO the AST class lacks an overload for `operator==` so here we compare it at the
            // string level
            REQUIRE(reindent_text(program_actual) == reindent_text(program_expected));
        }
    }
}
