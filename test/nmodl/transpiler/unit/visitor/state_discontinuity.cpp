/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
#include "visitors/state_discontinuity_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/semantic_analysis_visitor.hpp"
#include "visitors/visitor_utils.hpp"


using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using nmodl::parser::NmodlDriver;

//=============================================================================
// State discontinuity tests
//=============================================================================

auto run_state_discontinuity_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    SymtabVisitor().visit_program(*ast);
    if (SemanticAnalysisVisitor{}.check(*ast)) {
        throw std::runtime_error("Semantic analysis failed");
    }
    StateDiscontinuityVisitor().visit_program(*ast);

    return to_nmodl(*ast);
}

SCENARIO("State discontinuity in a NET_RECEIVE block", "[visitor][state_discontinuity]") {
    GIVEN("A call to state_discontinuity") {
        std::string nmodl_text = R"(
            NET_RECEIVE (w) {
                state_discontinuity(A, B)
            }
        )";
        std::string expected = R"(
            NET_RECEIVE (w) {
                A = B
            }
        )";

        THEN("Convert it to A = B") {
            const auto& actual = run_state_discontinuity_visitor(nmodl_text);
            REQUIRE(reindent_text(expected) == reindent_text(actual));
        }
    }
}

SCENARIO("State discontinuity outside of a NET_RECEIVE block", "[visitor][state_discontinuity]") {
    GIVEN("A call to state_discontinuity") {
        const std::string nmodl_text = R"(
            PROCEDURE fn(w) {
                state_discontinuity(A, B)
            }
        )";
        THEN("Leave it as-is") {
            const auto& actual = run_state_discontinuity_visitor(nmodl_text);
            REQUIRE(reindent_text(nmodl_text) == reindent_text(actual));
        }
    }
}

SCENARIO("State discontinuity call contains invalid arguments", "[visitor][state_discontinuity]") {
    GIVEN("A call to state_discontinuity with no args") {
        const std::string nmodl_text = R"(
            NET_RECEIVE (w) {
                state_discontinuity()
            }
        )";
        THEN("Raise an error") {
            REQUIRE_THROWS(run_state_discontinuity_visitor(nmodl_text));
        }
    }
    GIVEN("A call to state_discontinuity with 1 arg") {
        const std::string nmodl_text = R"(
            NET_RECEIVE (w) {
                state_discontinuity(A)
            }
        )";
        THEN("Raise an error") {
            REQUIRE_THROWS(run_state_discontinuity_visitor(nmodl_text));
        }
    }
    GIVEN("A call to state_discontinuity with 3 args") {
        const std::string nmodl_text = R"(
            NET_RECEIVE (w) {
                state_discontinuity(A, B, C)
            }
        )";
        THEN("Raise an error") {
            REQUIRE_THROWS(run_state_discontinuity_visitor(nmodl_text));
        }
    }
    GIVEN("A call to state_discontinuity with first arg a compound expression") {
        const std::string nmodl_text = R"(
            NET_RECEIVE (w) {
                state_discontinuity(A + 1, B)
            }
        )";
        THEN("Raise an error") {
            REQUIRE_THROWS(run_state_discontinuity_visitor(nmodl_text));
        }
    }
}
