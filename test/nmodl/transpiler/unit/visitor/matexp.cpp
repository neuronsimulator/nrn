/*
 * Copyright 2025 David McDougall
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <catch2/catch_test_macros.hpp>

#include "ast/all.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/matexp_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"
#include "utils/test_utils.hpp"

using nmodl::test_utils::reindent_text;

//=============================================================================
// Tests for solver method "matexp"
//=============================================================================

std::string run_matexp_visitor(const std::string& text, bool verbatim) {
    using namespace nmodl;
    using namespace visitor;
    using nmodl::ast::AstNodeType;
    using nmodl::parser::NmodlDriver;

    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);

    MatexpVisitor().visit_program(*ast);

    std::string nmodl;
    if (verbatim) {
        nmodl = to_nmodl(ast);
    } else {
        nmodl = to_nmodl(ast, {AstNodeType::VERBATIM});
    }

    return nmodl;
}

SCENARIO("Solve a KINETIC block using the matexp method", "[visitor][matexp]") {
    GIVEN("KINETIC block, to be solved in initial and breakpoint blocks") {
        std::string input_nmodl = R"(
        INITIAL {
            SOLVE test_kin STEADYSTATE matexp
        }

        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }

        PARAMETER {
            a = 0.654
            b = 0.129
        }

        STATE {
            x
            y
        }

        KINETIC test_kin {
            ~ x <-> y (a, b)
            CONSERVE x + y = 3.14159
        })";
        std::string expect_output = R"(
        INITIAL {
            MATEXP_SOLVE (1) {
                nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
                nmodl_eigen_j[1] = nmodl_eigen_j[1]+(a)*dt
                nmodl_eigen_j[3] = nmodl_eigen_j[3]-(b)*dt
                nmodl_eigen_j[2] = nmodl_eigen_j[2]+(b)*dt
            } CONSERVE = 3.14159
        }

        BREAKPOINT {
        }

        PARAMETER {
            a = 0.654
            b = 0.129
        }

        STATE {
            x
            y
        }

        MATEXP_SOLVE (0) {
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
            nmodl_eigen_j[1] = nmodl_eigen_j[1]+(a)*dt
            nmodl_eigen_j[3] = nmodl_eigen_j[3]-(b)*dt
            nmodl_eigen_j[2] = nmodl_eigen_j[2]+(b)*dt
        } CONSERVE = 3.14159
        )";
        THEN("Replace the kinetic block with its solution in a procedure") {
            auto result = run_matexp_visitor(input_nmodl, true);
            REQUIRE(reindent_text(expect_output) == reindent_text(result));
        }
    }
}

SCENARIO("Test exponential decay using the matexp method", "[visitor][matexp]") {
    GIVEN("KINETIC block, to be solved in initial and breakpoint blocks") {
        std::string input_nmodl = R"(
        INITIAL {
            SOLVE test_kin STEADYSTATE matexp
        }

        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }

        PARAMETER {
            a = 0.129
        }

        STATE {
            x
        }

        KINETIC test_kin {
            ~ x -> (a)
        })";
        std::string expect_output = R"(
        INITIAL {
            MATEXP_SOLVE (1) {
                nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
            }
        }

        BREAKPOINT {
        }

        PARAMETER {
            a = 0.129
        }

        STATE {
            x
        }

        MATEXP_SOLVE (0) {
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
        }
        )";
        THEN("Replace the kinetic block with its solution in a procedure") {
            auto result = run_matexp_visitor(input_nmodl, true);
            REQUIRE(reindent_text(expect_output) == reindent_text(result));
        }
    }
}

SCENARIO("Mix matexp and sparse solver methods", "[visitor][matexp]") {
    GIVEN("KINETIC block, to be solved by multiple methods") {
        std::string input_nmodl = R"(
        INITIAL {
            SOLVE test_kin STEADYSTATE sparse
        }

        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }

        STATE {
            x
            y
        }

        KINETIC test_kin {
            ~ x <-> y (a, b)
        })";
        std::string expect_output = R"(
        INITIAL {
            SOLVE test_kin STEADYSTATE sparse
        }

        BREAKPOINT {
        }

        STATE {
            x
            y
        }

        KINETIC test_kin {
            ~ x <-> y (a, b)
        }

        MATEXP_SOLVE (0) {
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
            nmodl_eigen_j[1] = nmodl_eigen_j[1]+(a)*dt
            nmodl_eigen_j[3] = nmodl_eigen_j[3]-(b)*dt
            nmodl_eigen_j[2] = nmodl_eigen_j[2]+(b)*dt
        }
        )";
        THEN(
            "The original KINETIC block is kept unchanged,"
            "its solution is inserted into a new PROCEDURE block") {
            auto result = run_matexp_visitor(input_nmodl, true);
            REQUIRE(reindent_text(expect_output) == reindent_text(result));
        }
    }
}

SCENARIO("Solve multiple blocks with matexp", "[visitor][matexp]") {
    GIVEN("Mutliple KINETIC block to be solved") {
        std::string input_nmodl = R"(
        INITIAL {
            SOLVE test_kin_1 STEADYSTATE matexp
            SOLVE test_kin_2 STEADYSTATE matexp
        }

        BREAKPOINT {
            SOLVE test_kin_1 METHOD matexp
            SOLVE test_kin_2 METHOD matexp
        }

        STATE {
            a
            b
            x
            y
        }

        KINETIC test_kin_1 {
            ~ a <-> b (1.1, 2.2)
        }

        KINETIC test_kin_2 {
            ~ x <-> y (3.3, 4.4)
        })";
        std::string expect_output = R"(
        INITIAL {
            MATEXP_SOLVE (1) {
                nmodl_eigen_j[0] = nmodl_eigen_j[0]-(1.1)*dt
                nmodl_eigen_j[1] = nmodl_eigen_j[1]+(1.1)*dt
                nmodl_eigen_j[5] = nmodl_eigen_j[5]-(2.2)*dt
                nmodl_eigen_j[4] = nmodl_eigen_j[4]+(2.2)*dt
            }
            MATEXP_SOLVE (1) {
                nmodl_eigen_j[10] = nmodl_eigen_j[10]-(3.3)*dt
                nmodl_eigen_j[11] = nmodl_eigen_j[11]+(3.3)*dt
                nmodl_eigen_j[15] = nmodl_eigen_j[15]-(4.4)*dt
                nmodl_eigen_j[14] = nmodl_eigen_j[14]+(4.4)*dt
            }
        }

        BREAKPOINT {
        }

        STATE {
            a
            b
            x
            y
        }

        MATEXP_SOLVE (0) {
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(1.1)*dt
            nmodl_eigen_j[1] = nmodl_eigen_j[1]+(1.1)*dt
            nmodl_eigen_j[5] = nmodl_eigen_j[5]-(2.2)*dt
            nmodl_eigen_j[4] = nmodl_eigen_j[4]+(2.2)*dt
        }

        MATEXP_SOLVE (0) {
            nmodl_eigen_j[10] = nmodl_eigen_j[10]-(3.3)*dt
            nmodl_eigen_j[11] = nmodl_eigen_j[11]+(3.3)*dt
            nmodl_eigen_j[15] = nmodl_eigen_j[15]-(4.4)*dt
            nmodl_eigen_j[14] = nmodl_eigen_j[14]+(4.4)*dt
        }
        )";
        THEN("All KINETIC block are solved") {
            auto result = run_matexp_visitor(input_nmodl, true);
            REQUIRE(reindent_text(expect_output) == reindent_text(result));
        }
    }
}

SCENARIO("Give non-linear equations to the matexp solver", "[visitor][matexp]") {
    GIVEN("KINETIC block with x + y <-> z reaction statement") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }
        STATE {
            x
            y
            z
        }
        KINETIC test_kin {
            ~ x + y <-> z (123, 456)
        })";
        THEN("Raise an exception") {
            REQUIRE_THROWS_AS(run_matexp_visitor(input_nmodl, true), std::invalid_argument);
        }
    }
    GIVEN("KINETIC block with 2 x <-> y reaction statement") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }
        STATE {
            x
            y
        }
        KINETIC test_kin {
            ~ 2 x <-> y (123, 456)
        })";
        THEN("Raise an exception") {
            REQUIRE_THROWS_AS(run_matexp_visitor(input_nmodl, true), std::invalid_argument);
        }
    }
    GIVEN("KINETIC block with 1 x <-> y reaction statement") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }
        STATE {
            x
            y
        }
        KINETIC test_kin {
            ~ 1 x <-> y (123, 456)
        })";
        THEN("Equation is valid, do not raise exception") {
            REQUIRE_NOTHROW(run_matexp_visitor(input_nmodl, true));
        }
    }
    GIVEN("KINETIC block with << reaction statement") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }
        STATE {
            x
        }
        KINETIC test_kin {
            ~ x << (123)
        })";
        THEN("Raise an exception") {
            REQUIRE_THROWS_AS(run_matexp_visitor(input_nmodl, true), std::invalid_argument);
        }
    }
    GIVEN("KINETIC block with -> reaction statement") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }
        STATE {
            x
        }
        KINETIC test_kin {
            ~ x -> (123)
        })";
        THEN("Equation is valid, do not raise exception") {
            REQUIRE_NOTHROW(run_matexp_visitor(input_nmodl, true));
        }
    }
    GIVEN("KINETIC block with invalid state variable") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            SOLVE test_kin METHOD matexp
        }
        STATE {
            x
        }
        KINETIC test_kin {
            ~ y -> (123)
        })";
        THEN("Raise an exception") {
            REQUIRE_THROWS_AS(run_matexp_visitor(input_nmodl, true), std::invalid_argument);
        }
    }
}
