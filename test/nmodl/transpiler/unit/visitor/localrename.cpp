/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"


using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using Catch::Matchers::Equals;
using nmodl::parser::NmodlDriver;

//=============================================================================
// Procedure/Function inlining tests
//=============================================================================

std::string run_inline_localvarrename_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    InlineVisitor().visit_program(*ast);
    SymtabVisitor().visit_program(*ast);
    LocalVarRenameVisitor().visit_program(*ast);
    SymtabVisitor().visit_program(*ast);
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);


    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return stream.str();
}

SCENARIO("LocalVarRenameVisitor works with InlineVisitor", "[visitor][localvarrename]") {
    GIVEN("A FUNCTION that gets inlined with a LOCAL variable") {
        std::string nmodl_text = R"(
            FUNCTION rates_1(Vm (mV)) {
                LOCAL v
                v = Vm + 5
                rates_1 = v
            }

            FUNCTION rates_2(Vm (mV)) {
                rates_2 = rates_1(Vm)
            }
        )";
        std::string output_nmodl = R"(
            FUNCTION rates_1(Vm(mV)) {
                LOCAL v_r_0
                v_r_0 = Vm+5
                rates_1 = v_r_0
            }

            FUNCTION rates_2(Vm(mV)) {
                LOCAL rates_1_in_0
                {
                    LOCAL v_r_1, Vm_in_0
                    Vm_in_0 = Vm
                    v_r_1 = Vm_in_0+5
                    rates_1_in_0 = v_r_1
                }
                rates_2 = rates_1_in_0
            }
        )";
        THEN("LOCAL variable in inlined function gets renamed") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_localvarrename_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}
