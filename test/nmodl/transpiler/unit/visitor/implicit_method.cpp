/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
#include "visitors/implicit_method_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

using namespace nmodl;
using nmodl::test_utils::reindent_text;

using Catch::Matchers::ContainsSubstring;  // ContainsSubstring in newer Catch2

//=============================================================================
// Implicit visitor tests
//=============================================================================

std::string generate_mod_after_implicit_method_visitor(std::string const& text) {
    parser::NmodlDriver driver{};
    auto const ast = driver.parse_string(text);
    visitor::SymtabVisitor{}.visit_program(*ast);
    visitor::ImplicitMethodVisitor{}.visit_program(*ast);
    return to_nmodl(*ast);
}

SCENARIO("Check insertion of explicit arguments to SOLVE block", "[codegen][implicit_methods]") {
    GIVEN("A mod file that has a SOLVE block of a derivative without an explicit METHOD") {
        auto const nmodl_text = R"(
            NEURON {
                SUFFIX ImplicitMethodTest
            }

            BREAKPOINT {
                SOLVE states
            }

            ASSIGNED {
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

            ASSIGNED {
                n
            }

            DERIVATIVE states {
                n' = -n
            }
        )";
        auto const actual_text = generate_mod_after_implicit_method_visitor(nmodl_text);
        THEN("at_time should have nt as its first argument") {
            REQUIRE(reindent_text(actual_text) == reindent_text(expected_text));
        }
    }
}
