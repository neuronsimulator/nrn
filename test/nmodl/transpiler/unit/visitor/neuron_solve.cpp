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
#include "visitors/symtab_visitor.hpp"


using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;


//=============================================================================
// CnexpSolve visitor tests
//=============================================================================

std::string run_cnexp_solve_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return stream.str();
}


SCENARIO("NeuronSolveVisitor visitor solves different ODE types") {
    GIVEN("Derivative block with cnexp method in breakpoint block") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m' = (mInf-m)/mTau[0]
                h' = (hInf-h)/hTau
                m = m + h
            }
        )";

        std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m = m+(1.0-exp(dt*((((-1.0)))/mTau[0])))*(-(((mInf))/mTau[0])/((((-1.0)))/mTau[0])-m)
                h = h+(1.0-exp(dt*((((-1.0)))/hTau)))*(-(((hInf))/hTau)/((((-1.0)))/hTau)-h)
                m = m+h
            }
        )";

        THEN("ODEs get replaced with solution") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("Derivative block without any solve method specification") {
        std::string nmodl_text = R"(
            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }
        )";

        std::string output_nmodl = R"(
            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }
        )";

        THEN("ODEs don't get solved") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("Derivative block with non-cnexp method in breakpoint block") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD derivimplicit
            }

            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }
        )";

        std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD derivimplicit
            }

            DERIVATIVE states {
                Dm = (mInf-m)/mTau
                Dh = (hInf-h)/hTau
            }
        )";

        THEN("ODEs don't get solved but state variables get replaced with Dstate ") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("Derivative block with ODEs that needs non-cnexp method to solve") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                A_AMPA' = tau_r_AMPA/A_AMPA
            }
        )";

        std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                A_AMPA' = tau_r_AMPA/A_AMPA
            }
        )";

        THEN("ODEs don't get replaced as cnexp is not possible") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}
