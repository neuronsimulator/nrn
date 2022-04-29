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
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;

//=============================================================================
// Constant folding tests
//=============================================================================

std::string run_constant_folding_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    ConstantFolderVisitor().visit_program(*ast);

    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return stream.str();
}

SCENARIO("Perform constant folder on NMODL constructs") {
    GIVEN("Simple integer expression") {
        std::string nmodl_text = R"(
                PROCEDURE dummy() {
                    a = 1 + 2
                }
            )";
        std::string expected_text = R"(
                PROCEDURE dummy() {
                    a = 3
                }
            )";
        THEN("successfully folds") {
            auto result = run_constant_folding_visitor(nmodl_text);
            REQUIRE(reindent_text(result) == reindent_text(expected_text));
        }
    }

    GIVEN("Simple double expression") {
        std::string nmodl_text = R"(
                PROCEDURE dummy() {
                    a = 1.1 + 2e-10
                }
            )";
        std::string expected_text = R"(
                PROCEDURE dummy() {
                    a = 1.1000000002
                }
            )";
        THEN("successfully folds") {
            auto result = run_constant_folding_visitor(nmodl_text);
            REQUIRE(reindent_text(result) == reindent_text(expected_text));
        }
    }

    GIVEN("Complex expression") {
        std::string nmodl_text = R"(
                PROCEDURE dummy() {
                    a = 1 + (2) + (2 / 2) + (((1+((2)))))
                }
            )";
        std::string expected_text = R"(
                PROCEDURE dummy() {
                    a = 7
                }
            )";
        THEN("successfully folds") {
            auto result = run_constant_folding_visitor(nmodl_text);
            REQUIRE(reindent_text(result) == reindent_text(expected_text));
        }
    }

    GIVEN("Integer expression with define statement") {
        std::string nmodl_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = N + (2*N) + (N / 2) + (((1+((N)))))
                    FROM i = 0 TO N-2 {
                    }
                }
            )";
        std::string expected_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = 46
                    FROM i = 0 TO 8 {
                    }
                }
            )";
        THEN("successfully folds") {
            auto result = run_constant_folding_visitor(nmodl_text);
            REQUIRE(reindent_text(result) == reindent_text(expected_text));
        }
    }

    GIVEN("Only fold part of the statement") {
        std::string nmodl_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = N + 2.0 + b
                    c = a + d
                    d = 2^3
                    e = 2 || 3
                }
            )";
        std::string expected_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = 12+b
                    c = a+d
                    d = 2^3
                    e = 2 || 3
                }
            )";
        THEN("successfully folds and keep other statements untouched") {
            auto result = run_constant_folding_visitor(nmodl_text);
            REQUIRE(reindent_text(result) == reindent_text(expected_text));
        }
    }

    GIVEN("Don't remove parentheses if not simplifying") {
        std::string nmodl_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = ((N+1)+5)*(c+1+N)/(b - 2)
                }
            )";
        std::string expected_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = 16*(c+1+10)/(b-2)
                }
            )";
        THEN("successfully folds and keep other statements untouched") {
            auto result = run_constant_folding_visitor(nmodl_text);
            REQUIRE(reindent_text(result) == reindent_text(expected_text));
        }
    }
}
