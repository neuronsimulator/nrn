/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/semantic_analysis_visitor.hpp"


using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using nmodl::parser::NmodlDriver;

//=============================================================================
// Procedure/Function inlining tests
//=============================================================================

void run_semantic_analysis_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    SemanticAnalysisVisitor().visit_program(*ast);
}

SCENARIO("TABLE stmt", "[visitor][semantic_analysis]") {
    GIVEN("Procedure with more than one argument") {
        std::string nmodl_text = R"(
            PROCEDURE rates_1(a, b) {
                TABLE ainf FROM 0 TO 1 WITH 1
                ainf = 1
            }
        )";
        THEN("throw") {
            REQUIRE_THROWS(run_semantic_analysis_visitor(nmodl_text));
        }
    }
    GIVEN("Procedure with exactly one argument") {
        std::string nmodl_text = R"(
            PROCEDURE rates_1(a) {
                TABLE ainf FROM 0 TO 1 WITH 1
                ainf = 1
            }
        )";
        THEN("no throw") {
            REQUIRE_NOTHROW(run_semantic_analysis_visitor(nmodl_text));
        }
    }
    GIVEN("Procedure with less than one argument") {
        std::string nmodl_text = R"(
            PROCEDURE rates_1() {
                TABLE ainf FROM 0 TO 1 WITH 1
                ainf = 1
            }
        )";
        THEN("throw") {
            REQUIRE_THROWS(run_semantic_analysis_visitor(nmodl_text));
        }
    }
}
