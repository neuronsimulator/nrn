/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/after_cvode_to_cnexp_visitor.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"


using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;


//=============================================================================
// AfterCVodeToCnexp visitor tests
//=============================================================================

std::string run_after_cvode_to_cnexp_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    AfterCVodeToCnexpVisitor().visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return to_nmodl(ast);
}


SCENARIO("AfterCVodeToCnexpVisitor changes after_cvode solver method to cnexp") {
    GIVEN("Breakpoint block with after_cvode method") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD after_cvode
            }
        )";

        std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }
        )";

        THEN("AfterCVodeToCnexp visitor replaces after_cvode solver with cnexp") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_after_cvode_to_cnexp_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}
