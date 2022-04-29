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
#include "visitors/loop_unroll_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;

//=============================================================================
// Loop unroll tests
//=============================================================================

std::string run_loop_unroll_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    ConstantFolderVisitor().visit_program(*ast);
    LoopUnrollVisitor().visit_program(*ast);
    ConstantFolderVisitor().visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return to_nmodl(ast, {AstNodeType::DEFINE});
}

SCENARIO("Perform loop unrolling of FROM construct", "[visitor][unroll]") {
    GIVEN("A loop with known iteration space") {
        std::string input_nmodl = R"(
            DEFINE N 2
            PROCEDURE rates() {
                LOCAL x[N]
                FROM i=0 TO N {
                    x[i] = x[i] + 11
                }
                FROM i=(0+(0+1)) TO (N+2-1) {
                    x[(i+0)] = x[i+1] + 11
                }
            }
            KINETIC state {
                FROM i=1 TO N+1 {
                    ~ ca[i] <-> ca[i+1] (DFree*frat[i+1]*1(um), DFree*frat[i+1]*1(um))
                }
            }
        )";
        std::string output_nmodl = R"(
            PROCEDURE rates() {
                LOCAL x[N]
                {
                    x[0] = x[0]+11
                    x[1] = x[1]+11
                    x[2] = x[2]+11
                }
                {
                    x[1] = x[2]+11
                    x[2] = x[3]+11
                    x[3] = x[4]+11
                }
            }

            KINETIC state {
                {
                    ~ ca[1] <-> ca[2] (DFree*frat[2]*1(um), DFree*frat[2]*1(um))
                    ~ ca[2] <-> ca[3] (DFree*frat[3]*1(um), DFree*frat[3]*1(um))
                    ~ ca[3] <-> ca[4] (DFree*frat[4]*1(um), DFree*frat[4]*1(um))
                }
            }
        )";
        THEN("Loop body gets correctly unrolled") {
            auto result = run_loop_unroll_visitor(input_nmodl);
            REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
        }
    }

    GIVEN("A nested loop") {
        std::string input_nmodl = R"(
            DEFINE N 1
            PROCEDURE rates() {
                LOCAL x[N]
                FROM i=0 TO N {
                    FROM j=1 TO N+1 {
                        x[i] = x[i+j] + 1
                    }
                }
            }
        )";
        std::string output_nmodl = R"(
            PROCEDURE rates() {
                LOCAL x[N]
                {
                    {
                        x[0] = x[1]+1
                        x[0] = x[2]+1
                    }
                    {
                        x[1] = x[2]+1
                        x[1] = x[3]+1
                    }
                }
            }
        )";
        THEN("Loop get unrolled recursively") {
            auto result = run_loop_unroll_visitor(input_nmodl);
            REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
        }
    }


    GIVEN("Loop with verbatim and unknown iteration space") {
        std::string input_nmodl = R"(
            DEFINE N 1
            PROCEDURE rates() {
                LOCAL x[N]
                FROM i=((0+0)) TO (((N+0))) {
                    FROM j=1 TO k {
                        x[i] = x[i+k] + 1
                    }
                }
                FROM i=0 TO N {
                    VERBATIM ENDVERBATIM
                }
            }
        )";
        std::string output_nmodl = R"(
            PROCEDURE rates() {
                LOCAL x[N]
                {
                    FROM j = 1 TO k {
                        x[0] = x[0+k]+1
                    }
                    FROM j = 1 TO k {
                        x[1] = x[1+k]+1
                    }
                }
                FROM i = 0 TO N {
                    VERBATIM ENDVERBATIM
                }
            }
        )";
        THEN("Only some loops get unrolled") {
            auto result = run_loop_unroll_visitor(input_nmodl);
            REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
        }
    }
}
