/*
 * Copyright 2025 David McDougall.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <regex>

#include "ast/all.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/matexp_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

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

std::string trim_text(std::string text) {
    // Remote leading whitespace
    text = std::regex_replace(text, std::regex("^[ \t\n\r\f\v]+"), "");

    // Remote trailing whitespace
    text = std::regex_replace(text, std::regex("[ \t\n\r\f\v]+$"), "");

    // Remote whitespace around newlines
    text = std::regex_replace(text, std::regex("[ \t\r\f\v]*\n[ \t\r\f\v]*"), "\n");

    // Remove duplicate newlines
    text = std::regex_replace(text, std::regex("\n+"), "\n");

    return text;
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
        VERBATIM
            #undef g
            #undef F
            #undef t
            #include <unsupported/Eigen/MatrixFunctions>
        ENDVERBATIM
        INITIAL {
            SOLVE test_kin_steadystate
        }
        BREAKPOINT {
            SOLVE test_kin_matexp
        }
        PARAMETER {
            a = 0.654
            b = 0.129
        }
        STATE {
            x
            y
        }
        PROCEDURE test_kin_steadystate() (){
            VERBATIM
                Eigen::Matrix<double, 2, 2> nmodl_eigen_jm = Eigen::Matrix<double, 2, 2>::Zero();
                double* nmodl_eigen_j = nmodl_eigen_jm.data();
            ENDVERBATIM
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*1000000000
            nmodl_eigen_j[1] = nmodl_eigen_j[1]+(a)*1000000000
            nmodl_eigen_j[3] = nmodl_eigen_j[3]-(b)*1000000000
            nmodl_eigen_j[2] = nmodl_eigen_j[2]+(b)*1000000000
            VERBATIM
                Eigen::Matrix<double, 2, 1> nmodl_eigen_xm;
                double* nmodl_eigen_x = nmodl_eigen_xm.data();
            ENDVERBATIM
            nmodl_eigen_x[0] = (3.14159)/2
            nmodl_eigen_x[1] = (3.14159)/2
            VERBATIM
                Eigen::Matrix<double, 2, 1> nmodl_eigen_ym = nmodl_eigen_jm.exp() * nmodl_eigen_xm;
                nmodl_eigen_ym *= (3.14159) / nmodl_eigen_ym.sum();
                double* nmodl_eigen_y = nmodl_eigen_ym.data();
            ENDVERBATIM
            x = nmodl_eigen_y[0]
            y = nmodl_eigen_y[1]
        }
        PROCEDURE test_kin_matexp() (){
            VERBATIM
                Eigen::Matrix<double, 2, 2> nmodl_eigen_jm = Eigen::Matrix<double, 2, 2>::Zero();
                double* nmodl_eigen_j = nmodl_eigen_jm.data();
            ENDVERBATIM
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
            nmodl_eigen_j[1] = nmodl_eigen_j[1]+(a)*dt
            nmodl_eigen_j[3] = nmodl_eigen_j[3]-(b)*dt
            nmodl_eigen_j[2] = nmodl_eigen_j[2]+(b)*dt
            VERBATIM
                Eigen::Matrix<double, 2, 1> nmodl_eigen_xm;
                double* nmodl_eigen_x = nmodl_eigen_xm.data();
            ENDVERBATIM
            nmodl_eigen_x[0] = x
            nmodl_eigen_x[1] = y
            VERBATIM
                Eigen::Matrix<double, 2, 1> nmodl_eigen_ym = nmodl_eigen_jm.exp() * nmodl_eigen_xm;
                nmodl_eigen_ym *= (3.14159) / nmodl_eigen_ym.sum();
                double* nmodl_eigen_y = nmodl_eigen_ym.data();
            ENDVERBATIM
            x = nmodl_eigen_y[0]
            y = nmodl_eigen_y[1]
        })";
        THEN("Replace the kinetic block with its solution in a procedure") {
            auto result = run_matexp_visitor(input_nmodl, true);
            REQUIRE(trim_text(expect_output) == trim_text(result));
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
        VERBATIM
            #undef g
            #undef F
            #undef t
            #include <unsupported/Eigen/MatrixFunctions>
        ENDVERBATIM
        INITIAL {
            SOLVE test_kin_steadystate
        }
        BREAKPOINT {
            SOLVE test_kin_matexp
        }
        PARAMETER {
            a = 0.129
        }
        STATE {
            x
        }
        PROCEDURE test_kin_steadystate() (){
            VERBATIM
                Eigen::Matrix<double, 1, 1> nmodl_eigen_jm = Eigen::Matrix<double, 1, 1>::Zero();
                double* nmodl_eigen_j = nmodl_eigen_jm.data();
            ENDVERBATIM
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*1000000000
            VERBATIM
                Eigen::Matrix<double, 1, 1> nmodl_eigen_xm;
                double* nmodl_eigen_x = nmodl_eigen_xm.data();
            ENDVERBATIM
            nmodl_eigen_x[0] = x
            VERBATIM
                Eigen::Matrix<double, 1, 1> nmodl_eigen_ym = nmodl_eigen_jm.exp() * nmodl_eigen_xm;
                double* nmodl_eigen_y = nmodl_eigen_ym.data();
            ENDVERBATIM
            x = nmodl_eigen_y[0]
        }
        PROCEDURE test_kin_matexp() (){
            VERBATIM
                Eigen::Matrix<double, 1, 1> nmodl_eigen_jm = Eigen::Matrix<double, 1, 1>::Zero();
                double* nmodl_eigen_j = nmodl_eigen_jm.data();
            ENDVERBATIM
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
            VERBATIM
                Eigen::Matrix<double, 1, 1> nmodl_eigen_xm;
                double* nmodl_eigen_x = nmodl_eigen_xm.data();
            ENDVERBATIM
            nmodl_eigen_x[0] = x
            VERBATIM
                Eigen::Matrix<double, 1, 1> nmodl_eigen_ym = nmodl_eigen_jm.exp() * nmodl_eigen_xm;
                double* nmodl_eigen_y = nmodl_eigen_ym.data();
            ENDVERBATIM
            x = nmodl_eigen_y[0]
        })";
        THEN("Replace the kinetic block with its solution in a procedure") {
            auto result = run_matexp_visitor(input_nmodl, true);
            REQUIRE(trim_text(expect_output) == trim_text(result));
        }
    }
}

SCENARIO("Mix matexp and sparse solver methods", "[visitor][matexp]") {
    GIVEN("KINETIC block, to be solved by multiple methods") {
        std::string input_nmodl = R"(
        BREAKPOINT {
            IF (t<1000) {
                SOLVE test_kin METHOD sparse
            } ELSE {
                SOLVE test_kin METHOD matexp
            }
        }
        STATE {
            x
            y
        }
        KINETIC test_kin {
            ~ x <-> y (a, b)
        })";
        std::string expect_output = R"(
        VERBATIM
            #undef g
            #undef F
            #undef t
            #include <unsupported/Eigen/MatrixFunctions>
        ENDVERBATIM
        BREAKPOINT {
            IF (t<1000) {
                SOLVE test_kin METHOD sparse
            } ELSE {
                SOLVE test_kin_matexp
            }
        }
        STATE {
            x
            y
        }
        KINETIC test_kin {
            ~ x <-> y (a, b)
        }
        PROCEDURE test_kin_matexp() (){
            VERBATIM
                Eigen::Matrix<double, 2, 2> nmodl_eigen_jm = Eigen::Matrix<double, 2, 2>::Zero();
                double* nmodl_eigen_j = nmodl_eigen_jm.data();
            ENDVERBATIM
            nmodl_eigen_j[0] = nmodl_eigen_j[0]-(a)*dt
            nmodl_eigen_j[1] = nmodl_eigen_j[1]+(a)*dt
            nmodl_eigen_j[3] = nmodl_eigen_j[3]-(b)*dt
            nmodl_eigen_j[2] = nmodl_eigen_j[2]+(b)*dt
            VERBATIM
                Eigen::Matrix<double, 2, 1> nmodl_eigen_xm;
                double* nmodl_eigen_x = nmodl_eigen_xm.data();
            ENDVERBATIM
            nmodl_eigen_x[0] = x
            nmodl_eigen_x[1] = y
            VERBATIM
                Eigen::Matrix<double, 2, 1> nmodl_eigen_ym = nmodl_eigen_jm.exp() * nmodl_eigen_xm;
                double* nmodl_eigen_y = nmodl_eigen_ym.data();
            ENDVERBATIM
            x = nmodl_eigen_y[0]
            y = nmodl_eigen_y[1]
        })";
        THEN(
            "The original KINETIC block is kept unchanged,"
            "its solution is inserted into a new PROCEDURE block") {
            auto result = run_matexp_visitor(input_nmodl, true);
            REQUIRE(trim_text(expect_output) == trim_text(result));
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
