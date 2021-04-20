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
#include "visitors/checkparent_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/symtab_visitor.hpp"


using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;

//=============================================================================
// Procedure/Function inlining tests
//=============================================================================

std::string run_inline_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    InlineVisitor().visit_program(*ast);
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);


    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return stream.str();
}

SCENARIO("Inlining of external procedure calls", "[visitor][inline]") {
    GIVEN("Procedures with external procedure call") {
        std::string nmodl_text = R"(
            PROCEDURE rates_1() {
                hello()
            }

            PROCEDURE rates_2() {
                bye()
            }
        )";

        THEN("nothing gets inlinine") {
            std::string input = reindent_text(nmodl_text);
            auto result = run_inline_visitor(input);
            REQUIRE(result == input);
        }
    }
}

SCENARIO("Inlining of function call as argument in external function", "[visitor][inline]") {
    GIVEN("An external function calling a function") {
        std::string input_nmodl = R"(
            FUNCTION rates_1() {
                rates_1 = 1
            }

            FUNCTION rates_2() {
                net_send(rates_1(), 0)
            }
        )";

        std::string output_nmodl = R"(
            FUNCTION rates_1() {
                rates_1 = 1
            }

            FUNCTION rates_2() {
                LOCAL rates_1_in_0
                {
                    rates_1_in_0 = 1
                }
                net_send(rates_1_in_0, 0)
            }
        )";
        THEN("External function doesn't get inlined") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inlining of simple, one level procedure call", "[visitor][inline]") {
    GIVEN("A procedure calling another procedure") {
        std::string input_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x
                rates_2(23.1)
            }

            PROCEDURE rates_2(y) {
                LOCAL x
                x = 21.1*v+y
            }
        )";

        std::string output_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x
                {
                    LOCAL x, y_in_0
                    y_in_0 = 23.1
                    x = 21.1*v+y_in_0
                }
            }

            PROCEDURE rates_2(y) {
                LOCAL x
                x = 21.1*v+y
            }
        )";
        THEN("Procedure body gets inlined") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inlining of nested procedure call", "[visitor][inline]") {
    GIVEN("A procedure with nested call chain and arguments") {
        std::string input_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x, y
                rates_2()
                rates_3(x, y)
            }

            PROCEDURE rates_2() {
                LOCAL x
                x = 21.1*v + rates_3(x, x+1.1)
            }

            PROCEDURE rates_3(a, b) {
                LOCAL c
                c = 21.1*v+a*b
            }
        )";

        std::string output_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x, y
                {
                    LOCAL x, rates_3_in_0
                    {
                        LOCAL c, a_in_0, b_in_0
                        a_in_0 = x
                        b_in_0 = x+1.1
                        c = 21.1*v+a_in_0*b_in_0
                        rates_3_in_0 = 0
                    }
                    x = 21.1*v+rates_3_in_0
                }
                {
                    LOCAL c, a_in_1, b_in_1
                    a_in_1 = x
                    b_in_1 = y
                    c = 21.1*v+a_in_1*b_in_1
                }
            }

            PROCEDURE rates_2() {
                LOCAL x, rates_3_in_0
                {
                    LOCAL c, a_in_0, b_in_0
                    a_in_0 = x
                    b_in_0 = x+1.1
                    c = 21.1*v+a_in_0*b_in_0
                    rates_3_in_0 = 0
                }
                x = 21.1*v+rates_3_in_0
            }

            PROCEDURE rates_3(a, b) {
                LOCAL c
                c = 21.1*v+a*b
            }
        )";
        THEN("Nested procedure gets inlined with variables renaming") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inline function call in procedure", "[visitor][inline]") {
    GIVEN("A procedure with function call") {
        std::string input_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x
                x = 12.1+rates_2()
            }

            FUNCTION rates_2() {
                LOCAL x
                x = 21.1*12.1+11
                rates_2 = x
            }
        )";

        std::string output_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x, rates_2_in_0
                {
                    LOCAL x
                    x = 21.1*12.1+11
                    rates_2_in_0 = x
                }
                x = 12.1+rates_2_in_0
            }

            FUNCTION rates_2() {
                LOCAL x
                x = 21.1*12.1+11
                rates_2 = x
            }
        )";
        THEN("Procedure body gets inlined") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inling function call within conditional statement", "[visitor][inline]") {
    GIVEN("A procedure with function call in if statement") {
        std::string input_nmodl = R"(
            FUNCTION rates_1() {
                IF (rates_2()) {
                    rates_1 = 10
                } ELSE {
                    rates_1 = 20
                }
            }

            FUNCTION rates_2() {
                rates_2 = 10
            }
        )";

        std::string output_nmodl = R"(
            FUNCTION rates_1() {
                LOCAL rates_2_in_0
                {
                    rates_2_in_0 = 10
                }
                IF (rates_2_in_0) {
                    rates_1 = 10
                } ELSE {
                    rates_1 = 20
                }
            }

            FUNCTION rates_2() {
                rates_2 = 10
            }
        )";

        THEN("Procedure body gets inlined and return value is used in if condition") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inline multiple function calls in same statement", "[visitor][inline]") {
    GIVEN("A procedure with two function calls in binary expression") {
        std::string input_nmodl = R"(
            FUNCTION rates_1() {
                IF (rates_2()-rates_2()) {
                    rates_1 = 20
                }
            }

            FUNCTION rates_2() {
                rates_2 = 10
            }
        )";

        std::string output_nmodl = R"(
            FUNCTION rates_1() {
                LOCAL rates_2_in_0, rates_2_in_1
                {
                    rates_2_in_0 = 10
                }
                {
                    rates_2_in_1 = 10
                }
                IF (rates_2_in_0-rates_2_in_1) {
                    rates_1 = 20
                }
            }

            FUNCTION rates_2() {
                rates_2 = 10
            }
        )";

        THEN("Procedure body gets inlined") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("A procedure with multiple function calls in an expression") {
        std::string input_nmodl = R"(
            FUNCTION rates_1() {
                LOCAL x
                x = (rates_2()+(rates_2()/rates_2()))
            }

            FUNCTION rates_2() {
                rates_2 = 10
            }
        )";

        std::string output_nmodl = R"(
            FUNCTION rates_1() {
                LOCAL x, rates_2_in_0, rates_2_in_1, rates_2_in_2
                {
                    rates_2_in_0 = 10
                }
                {
                    rates_2_in_1 = 10
                }
                {
                    rates_2_in_2 = 10
                }
                x = (rates_2_in_0+(rates_2_in_1/rates_2_in_2))
            }

            FUNCTION rates_2() {
                rates_2 = 10
            }
        )";

        THEN("Procedure body gets inlined and return values are used in an expression") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inline nested function calls withing arguments", "[visitor][inline]") {
    GIVEN("A procedure with function call") {
        std::string input_nmodl = R"(
            FUNCTION rates_2() {
                IF (rates_3(11,21)) {
                   rates_2 = 10.1
                }
                rates_2 = rates_3(12,22)
            }

            FUNCTION rates_1() {
                rates_1 = 12.1+rates_2()+exp(12.1)
            }

            FUNCTION rates_3(x, y) {
                rates_3 = x+y
            }
        )";

        std::string output_nmodl = R"(
            FUNCTION rates_2() {
                LOCAL rates_3_in_0, rates_3_in_1
                {
                    LOCAL x_in_0, y_in_0
                    x_in_0 = 11
                    y_in_0 = 21
                    rates_3_in_0 = x_in_0+y_in_0
                }
                IF (rates_3_in_0) {
                    rates_2 = 10.1
                }
                {
                    LOCAL x_in_1, y_in_1
                    x_in_1 = 12
                    y_in_1 = 22
                    rates_3_in_1 = x_in_1+y_in_1
                }
                rates_2 = rates_3_in_1
            }

            FUNCTION rates_1() {
                LOCAL rates_2_in_0
                {
                    LOCAL rates_3_in_0, rates_3_in_1
                    {
                        LOCAL x_in_0, y_in_0
                        x_in_0 = 11
                        y_in_0 = 21
                        rates_3_in_0 = x_in_0+y_in_0
                    }
                    IF (rates_3_in_0) {
                        rates_2_in_0 = 10.1
                    }
                    {
                        LOCAL x_in_1, y_in_1
                        x_in_1 = 12
                        y_in_1 = 22
                        rates_3_in_1 = x_in_1+y_in_1
                    }
                    rates_2_in_0 = rates_3_in_1
                }
                rates_1 = 12.1+rates_2_in_0+exp(12.1)
            }

            FUNCTION rates_3(x, y) {
                rates_3 = x+y
            }
        )";

        THEN("Procedure body gets inlined") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inline function call in non-binary expression", "[visitor][inline]") {
    GIVEN("A function call in unary expression") {
        std::string input_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x
                x = (-rates_2(23.1))
            }

            FUNCTION rates_2(y) {
                rates_2 = 21.1*v+y
            }
        )";

        std::string output_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x, rates_2_in_0
                {
                    LOCAL y_in_0
                    y_in_0 = 23.1
                    rates_2_in_0 = 21.1*v+y_in_0
                }
                x = (-rates_2_in_0)
            }

            FUNCTION rates_2(y) {
                rates_2 = 21.1*v+y
            }
        )";
        THEN("Function gets inlined in the unary expression") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("A function call as part of function argument itself") {
        std::string input_nmodl = R"(
            FUNCTION rates_1() {
                rates_1 = 10 + rates_2( rates_2(11) )
            }

            FUNCTION rates_2(x) {
                rates_2 = 10+x
            }
        )";

        std::string output_nmodl = R"(
            FUNCTION rates_1() {
                LOCAL rates_2_in_0, rates_2_in_1
                {
                    LOCAL x_in_0
                    x_in_0 = 11
                    rates_2_in_0 = 10+x_in_0
                }
                {
                    LOCAL x_in_1
                    x_in_1 = rates_2_in_0
                    rates_2_in_1 = 10+x_in_1
                }
                rates_1 = 10+rates_2_in_1
            }

            FUNCTION rates_2(x) {
                rates_2 = 10+x
            }
        )";
        THEN("Function and it's arguments gets inlined recursively") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}


SCENARIO("Inline function call as standalone expression", "[visitor][inline]") {
    GIVEN("Function call as a statement") {
        std::string input_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x
                rates_2(23.1)
            }

            FUNCTION rates_2(y) {
                rates_2 = 21.1*v+y
            }
        )";

        std::string output_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x, rates_2_in_0
                {
                    LOCAL y_in_0
                    y_in_0 = 23.1
                    rates_2_in_0 = 21.1*v+y_in_0
                }
            }

            FUNCTION rates_2(y) {
                rates_2 = 21.1*v+y
            }
        )";
        THEN("Function gets inlined but it's value is not used") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inline procedure call as standalone statement as well as part of expression",
         "[visitor][inline]") {
    GIVEN("A procedure call in expression and statement") {
        std::string input_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x
                x = 10 + rates_2()
                rates_2()
            }

            PROCEDURE rates_2() {
            }
        )";

        std::string output_nmodl = R"(
            PROCEDURE rates_1() {
                LOCAL x, rates_2_in_0
                {
                    rates_2_in_0 = 0
                }
                x = 10+rates_2_in_0
                {
                }
            }

            PROCEDURE rates_2() {
            }
        )";
        THEN("Return statement from procedure (with zero value) is used") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Inlining pass handles local-global name conflict", "[visitor][inline]") {
    GIVEN("A procedure with local variable that exist in global scope") {
        /// note that x in rates_2 should still update global x after inlining
        std::string input_nmodl = R"(
            NEURON {
                RANGE x
            }

            PROCEDURE rates_1() {
                LOCAL x
                x = 12
                rates_2(x)
                x = 11
            }

            PROCEDURE rates_2(y) {
                x = 10+y
            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE x
            }

            PROCEDURE rates_1() {
                LOCAL x_r_0
                x_r_0 = 12
                {
                    LOCAL y_in_0
                    y_in_0 = x_r_0
                    x = 10+y_in_0
                }
                x_r_0 = 11
            }

            PROCEDURE rates_2(y) {
                x = 10+y
            }
        )";

        THEN("Caller variables get renamed first and then inlining is done") {
            std::string input = reindent_text(input_nmodl);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_inline_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}
