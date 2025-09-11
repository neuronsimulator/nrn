/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/solve_without_method_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace nmodl;
using nmodl::test_utils::reindent_text;

//=============================================================================
// Solve without method visitor tests
//=============================================================================

std::string generate_mod_after_solve_without_method_visitor(std::string const& text) {
    parser::NmodlDriver driver{};
    auto const ast = driver.parse_string(text);
    visitor::SymtabVisitor{}.visit_program(*ast);
    visitor::SolveWithoutMethodVisitor{}.visit_program(*ast);
    return to_nmodl(*ast);
}

SCENARIO("Check insertion of explicit METHOD to SOLVE block", "[visitor][solve_without_method]") {
    GIVEN("A mod file that has a SOLVE block of a derivative without an explicit METHOD") {
        auto const nmodl_text = R"(
            NEURON {
                SUFFIX ImplicitMethodTest
            }

            BREAKPOINT {
                SOLVE states
            }

            STATE {
                n
            }

            DERIVATIVE states {
                n' = -n
            }
        )";
        auto const expected_text = R"(
            NEURON {
                SUFFIX ImplicitMethodTest
            }

            BREAKPOINT {
                SOLVE states METHOD derivimplicit
            }

            STATE {
                n
            }

            DERIVATIVE states {
                n' = -n
            }
        )";
        auto const actual_text = generate_mod_after_solve_without_method_visitor(nmodl_text);
        THEN("The two mod files should match") {
            REQUIRE(reindent_text(actual_text) == reindent_text(expected_text));
        }
    }
    GIVEN("A mod file that has a SOLVE block of a derivative with an explicit METHOD") {
        auto const nmodl_text = R"(
            NEURON {
                SUFFIX ImplicitMethodTest
            }

            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            STATE {
                n
            }

            DERIVATIVE states {
                n' = -n
            }
        )";
        auto const actual_text = generate_mod_after_solve_without_method_visitor(nmodl_text);
        THEN("The mod file should remain as-is") {
            REQUIRE(reindent_text(actual_text) == reindent_text(nmodl_text));
        }
    }
    GIVEN("A mod file that has a SOLVE block of a LINEAR block without an explicit METHOD") {
        auto const nmodl_text = R"(
            NEURON {
                SUFFIX ImplicitMethodTest
            }

            BREAKPOINT {
                SOLVE states
            }

            STATE {
                x
            }

            LINEAR lin {
                ~ 2*a*x = 1
            }
        )";
        auto const actual_text = generate_mod_after_solve_without_method_visitor(nmodl_text);
        THEN("The mod file should remain as-is") {
            REQUIRE(reindent_text(actual_text) == reindent_text(nmodl_text));
        }
    }
}
