/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ast/breakpoint_block.hpp"
#include "ast/initial_block.hpp"
#include "ast/program.hpp"
#include "ast/statement_block.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
#include "visitors/merge_top_level_blocks_visitor.hpp"
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
// MergeToplevelBlocks visitor tests
//=============================================================================

template <typename block_type, ast::AstNodeType node_type>
auto generate_mod_after_merge_top_level_blocks_visitor(std::string const& text) {
    parser::NmodlDriver driver{};
    auto ast = driver.parse_string(text);
    visitor::MergeTopLevelBlocksVisitor<block_type, node_type>{}.visit_program(*ast);
    return to_nmodl(*ast);
}

SCENARIO("Check multiple INITIAL blocks are handled properly",
         "[visitor][merge_top_level_blocks]") {
    GIVEN("A mod file with multiple empty top-level INITIAL blocks") {
        const auto nmodl_text_before = R"(
            NEURON {
                SUFFIX InitialBlockTest
                RANGE foo, bar
            }
            INITIAL {
            }
            INITIAL {
            }
            INITIAL {
            }
        )";
        const auto nmodl_text_after = R"(
            NEURON {
              SUFFIX InitialBlockTest
              RANGE foo, bar
            }
            INITIAL {
            }
        )";
        parser::NmodlDriver driver{};
        auto ast_expected = driver.parse_string(nmodl_text_after);
        const auto program_expected = to_nmodl(ast_expected);
        const auto program_actual =
            generate_mod_after_merge_top_level_blocks_visitor<ast::InitialBlock,
                                                              ast::AstNodeType::INITIAL_BLOCK>(
                nmodl_text_before);
        THEN("expected and actual should be identical at the level of the AST") {
            REQUIRE(reindent_text(program_actual) == reindent_text(program_expected));
        }
    }
    GIVEN("A mod file with an empty and a non-empty top-level INITIAL block") {
        const auto nmodl_text_before = R"(
            NEURON {
                SUFFIX InitialBlockTest
                RANGE foo, bar
            }
            INITIAL {
            }
            INITIAL {
                foo = 1
            }
        )";
        const auto nmodl_text_after = R"(
            NEURON {
              SUFFIX InitialBlockTest
              RANGE foo, bar
            }
            INITIAL {
                {
                    foo = 1
                }
            }
        )";
        parser::NmodlDriver driver{};
        auto ast_expected = driver.parse_string(nmodl_text_after);
        const auto program_expected = to_nmodl(ast_expected);
        const auto program_actual =
            generate_mod_after_merge_top_level_blocks_visitor<ast::InitialBlock,
                                                              ast::AstNodeType::INITIAL_BLOCK>(
                nmodl_text_before);
        THEN("expected and actual should be identical at the level of the AST") {
            REQUIRE(reindent_text(program_actual) == reindent_text(program_expected));
        }
    }
    GIVEN("A mod file with multiple non-empty INITIAL blocks") {
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
                {
                    foo = 1
                }
                {
                    bar = 2
                }
            }
        )";
        parser::NmodlDriver driver{};
        auto ast_expected = driver.parse_string(nmodl_text_after);
        const auto program_expected = to_nmodl(ast_expected);
        const auto program_actual =
            generate_mod_after_merge_top_level_blocks_visitor<ast::InitialBlock,
                                                              ast::AstNodeType::INITIAL_BLOCK>(
                nmodl_text_before);
        THEN("expected and actual should be identical at the level of the AST") {
            REQUIRE(reindent_text(program_actual) == reindent_text(program_expected));
        }
    }
    GIVEN("A mod file with an INITIAL block only inside of a NET_RECEIVE block") {
        const auto nmodl_text_before = R"(
            NEURON {
                SUFFIX test
                RANGE foo, bar
            }

            NET_RECEIVE (w) {
                INITIAL {
                    foo = 1
                }
            }
        )";
        const auto program_actual =
            generate_mod_after_merge_top_level_blocks_visitor<ast::InitialBlock,
                                                              ast::AstNodeType::INITIAL_BLOCK>(
                nmodl_text_before);
        THEN("leave the mod file as-is") {
            REQUIRE(reindent_text(program_actual) == reindent_text(nmodl_text_before));
        }
    }
    GIVEN("A mod file with an INITIAL block, and one inside of a NET_RECEIVE block") {
        // Note that the visitor actually modifies the AST (since there is > 1 INITIAL block in the
        // entire file: one top-level, and one in NET_RECEIVE). If we place the top-level INITIAL
        // block before NET_RECEIVE in the below, the top-level INITIAL block will be deleted and
        // appended. However, since the position of the INITIAL block in the mod file has no impact
        // on the semantics, the visitor works as expected
        const auto nmodl_text_before = R"(
            NEURON {
                SUFFIX test
                RANGE foo, bar
            }

            NET_RECEIVE (w) {
                INITIAL {
                    foo = 1
                }
            }

            INITIAL {
                bar = 2
            }
        )";
        const auto nmodl_text_after = R"(
            NEURON {
                SUFFIX test
                RANGE foo, bar
            }

            NET_RECEIVE (w) {
                INITIAL {
                    foo = 1
                }
            }

            INITIAL {
                {
                    bar = 2
                }
            }
        )";
        parser::NmodlDriver driver{};
        auto ast_expected = driver.parse_string(nmodl_text_after);
        const auto program_expected = to_nmodl(ast_expected);
        const auto program_actual =
            generate_mod_after_merge_top_level_blocks_visitor<ast::InitialBlock,
                                                              ast::AstNodeType::INITIAL_BLOCK>(
                nmodl_text_before);
        THEN("leave the mod file as-is") {
            REQUIRE(reindent_text(program_actual) == reindent_text(program_expected));
        }
    }
}

SCENARIO("Check multiple BREAKPOINT blocks are handled properly",
         "[visitor][merge_top_level_blocks]") {
    GIVEN("A mod file with multiple BREAKPOINT blocks") {
        const auto nmodl_text_before = R"(
            NEURON {
                SUFFIX BreakpointBlockTest
                RANGE foo, bar
            }
            BREAKPOINT {
                foo = 1
            }
            BREAKPOINT {
                bar = 2
            }
        )";
        const auto nmodl_text_after = R"(
            NEURON {
              SUFFIX BreakpointBlockTest
              RANGE foo, bar
            }
            BREAKPOINT {
                {
                    foo = 1
                }
                {
                    bar = 2
                }
            }
        )";
        parser::NmodlDriver driver{};
        auto ast_expected = driver.parse_string(nmodl_text_after);
        const auto program_expected = to_nmodl(ast_expected);
        const auto program_actual =
            generate_mod_after_merge_top_level_blocks_visitor<ast::BreakpointBlock,
                                                              ast::AstNodeType::BREAKPOINT_BLOCK>(
                nmodl_text_before);
        THEN("expected and actual should be identical at the level of the AST") {
            REQUIRE(reindent_text(program_actual) == reindent_text(program_expected));
        }
    }
}
