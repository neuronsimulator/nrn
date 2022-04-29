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
#include "visitors/checkparent_visitor.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/loop_unroll_visitor.hpp"
#include "visitors/steadystate_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;


//=============================================================================
// STEADYSTATE visitor tests
//=============================================================================

std::vector<std::string> run_steadystate_visitor(
    const std::string& text,
    const std::vector<AstNodeType>& ret_nodetypes = {AstNodeType::SOLVE_BLOCK,
                                                     AstNodeType::DERIVATIVE_BLOCK}) {
    std::vector<std::string> results;
    // construct AST from text
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor().visit_program(*ast);

    // unroll loops and fold constants
    ConstantFolderVisitor().visit_program(*ast);
    LoopUnrollVisitor().visit_program(*ast);
    ConstantFolderVisitor().visit_program(*ast);
    SymtabVisitor().visit_program(*ast);

    // Run kinetic block visitor first, so any kinetic blocks
    // are converted into derivative blocks
    KineticBlockVisitor().visit_program(*ast);
    SymtabVisitor().visit_program(*ast);

    // run SteadystateVisitor on AST
    SteadystateVisitor().visit_program(*ast);

    // run lookup visitor to extract results from AST
    const auto& res = collect_nodes(*ast, ret_nodetypes);
    results.reserve(res.size());
    for (const auto& r: res) {
        results.push_back(to_nmodl(r));
    }

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return results;
}

SCENARIO("Solving ODEs with STEADYSTATE solve method", "[visitor][steadystate]") {
    GIVEN("STEADYSTATE sparse solve") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states STEADYSTATE sparse
            }
            DERIVATIVE states {
                m' = m + h
            }
        )";
        std::string expected_text1 = R"(
            DERIVATIVE states {
                m' = m+h
            })";
        std::string expected_text2 = R"(
            DERIVATIVE states_steadystate {
                dt = 1000000000
                m' = m+h
            })";
        THEN("Construct DERIVATIVE block with steadystate solution & update SOLVE statement") {
            auto result = run_steadystate_visitor(nmodl_text);
            REQUIRE(result.size() == 3);
            REQUIRE(result[0] == "SOLVE states_steadystate METHOD sparse");
            REQUIRE(reindent_text(result[1]) == reindent_text(expected_text1));
            REQUIRE(reindent_text(result[2]) == reindent_text(expected_text2));
        }
    }
    GIVEN("STEADYSTATE derivimplicit solve") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states STEADYSTATE derivimplicit
            }
            DERIVATIVE states {
                m' = m + h
            }
        )";
        std::string expected_text1 = R"(
            DERIVATIVE states {
                m' = m+h
            })";
        std::string expected_text2 = R"(
            DERIVATIVE states_steadystate {
                dt = 1e-09
                m' = m+h
            })";
        THEN("Construct DERIVATIVE block with steadystate solution & update SOLVE statement") {
            auto result = run_steadystate_visitor(nmodl_text);
            REQUIRE(result.size() == 3);
            REQUIRE(result[0] == "SOLVE states_steadystate METHOD derivimplicit");
            REQUIRE(reindent_text(result[1]) == reindent_text(expected_text1));
            REQUIRE(reindent_text(result[2]) == reindent_text(expected_text2));
        }
    }
    GIVEN("two STEADYSTATE solves") {
        std::string nmodl_text = R"(
            STATE {
                Z[3]
                x
            }
            BREAKPOINT  {
                SOLVE states0 STEADYSTATE derivimplicit
                SOLVE states1 STEADYSTATE sparse
            }
            DERIVATIVE states0 {
                Z'[0] = Z[1] - Z[2]
                Z'[1] = Z[0] + 2*Z[2]
                Z'[2] = Z[0]*Z[0] - 3.10
            }
            DERIVATIVE states1 {
                x' = x + c
            }
        )";
        std::string expected_text1 = R"(
            DERIVATIVE states0 {
                Z'[0] = Z[1]-Z[2]
                Z'[1] = Z[0]+2*Z[2]
                Z'[2] = Z[0]*Z[0]-3.10
            })";
        std::string expected_text2 = R"(
            DERIVATIVE states1 {
                x' = x+c
            })";
        std::string expected_text3 = R"(
            DERIVATIVE states0_steadystate {
                dt = 1e-09
                Z'[0] = Z[1]-Z[2]
                Z'[1] = Z[0]+2*Z[2]
                Z'[2] = Z[0]*Z[0]-3.10
            })";
        std::string expected_text4 = R"(
            DERIVATIVE states1_steadystate {
                dt = 1000000000
                x' = x+c
            })";
        THEN("Construct DERIVATIVE blocks with steadystate solution & update SOLVE statements") {
            auto result = run_steadystate_visitor(nmodl_text);
            REQUIRE(result.size() == 6);
            REQUIRE(result[0] == "SOLVE states0_steadystate METHOD derivimplicit");
            REQUIRE(result[1] == "SOLVE states1_steadystate METHOD sparse");
            REQUIRE(reindent_text(result[2]) == reindent_text(expected_text1));
            REQUIRE(reindent_text(result[3]) == reindent_text(expected_text2));
            REQUIRE(reindent_text(result[4]) == reindent_text(expected_text3));
            REQUIRE(reindent_text(result[5]) == reindent_text(expected_text4));
        }
    }
}
