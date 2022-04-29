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
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;


//=============================================================================
// SolveBlock visitor tests
//=============================================================================

std::string run_solve_block_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    SymtabVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    SolveBlockVisitor().visit_program(*ast);
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return stream.str();
}

TEST_CASE("Solve ODEs using legacy NeuronSolveVisitor", "[visitor][solver]") {
    SECTION("SolveBlock add NrnState block") {
        GIVEN("Breakpoint block with single solve block in breakpoint") {
            std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m' = (mInf-m)/mTau
            }
        )";

            std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m = m+(1.0-exp(dt*((((-1.0)))/mTau)))*(-(((mInf))/mTau)/((((-1.0)))/mTau)-m)
            }

            NRN_STATE SOLVE states METHOD cnexp{
                m = m+(1.0-exp(dt*((((-1.0)))/mTau)))*(-(((mInf))/mTau)/((((-1.0)))/mTau)-m)
            }

        )";

            THEN("Single NrnState block gets added") {
                auto result = run_solve_block_visitor(nmodl_text);
                REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
            }
        }

        GIVEN("Breakpoint block with two solve block in breakpoint") {
            std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE state1 METHOD cnexp
                SOLVE state2 METHOD cnexp
            }

            DERIVATIVE state1 {
                m' = (mInf-m)/mTau
            }

            DERIVATIVE state2 {
                h' = (mInf-h)/mTau
            }
        )";

            std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE state1 METHOD cnexp
                SOLVE state2 METHOD cnexp
            }

            DERIVATIVE state1 {
                m = m+(1.0-exp(dt*((((-1.0)))/mTau)))*(-(((mInf))/mTau)/((((-1.0)))/mTau)-m)
            }

            DERIVATIVE state2 {
                h = h+(1.0-exp(dt*((((-1.0)))/mTau)))*(-(((mInf))/mTau)/((((-1.0)))/mTau)-h)
            }

            NRN_STATE SOLVE state1 METHOD cnexp{
                m = m+(1.0-exp(dt*((((-1.0)))/mTau)))*(-(((mInf))/mTau)/((((-1.0)))/mTau)-m)
            }
            SOLVE state2 METHOD cnexp{
                h = h+(1.0-exp(dt*((((-1.0)))/mTau)))*(-(((mInf))/mTau)/((((-1.0)))/mTau)-h)
            }

        )";

            THEN("NrnState blok combining multiple solve nodes added") {
                auto result = run_solve_block_visitor(nmodl_text);
                REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
            }
        }
    }
}
