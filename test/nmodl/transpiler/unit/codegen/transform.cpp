/*************************************************************************
 * Copyright (C) 2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch2/catch.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_transform_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/nmodl_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using nmodl::parser::NmodlDriver;


//=============================================================================
// Variable rename tests
//=============================================================================

static std::string run_transform_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    CodegenTransformVisitor{}.visit_program(*ast);
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);

    return stream.str();
}

SCENARIO("Adding a variable for a table inside a function", "[visitor][transform]") {
    GIVEN("A mod file with a table inside a function") {
        // sample nmodl text
        std::string input_nmodl_text = R"(
            FUNCTION mAlpha(v) {
                TABLE FROM 0 TO 150 WITH 1
                mAlpha = 1
            }
        )";

        /// expected result after adding the variable
        std::string output_nmodl_text = R"(
            FUNCTION mAlpha(v) {
                TABLE mAlpha FROM 0 TO 150 WITH 1
                mAlpha = 1
            }
        )";

        std::string input = reindent_text(input_nmodl_text);
        std::string expected_output = reindent_text(output_nmodl_text);

        THEN("variable has been added") {
            auto result = run_transform_visitor(input);
            REQUIRE(result == expected_output);
        }
    }
}
