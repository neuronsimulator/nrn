/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <pybind11/embed.h>
#include <pybind11/stl.h>

#include "ast/program.hpp"
#include "codegen/codegen_coreneuron_cpp_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/loop_unroll_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"


using namespace nmodl;
using namespace codegen;
using namespace visitor;
using namespace test;
using namespace test_utils;

using Catch::Matchers::ContainsSubstring;  // ContainsSubstring in newer Catch2

using nmodl::test_utils::reindent_text;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;


//=============================================================================
// SympySolver visitor tests
//=============================================================================

std::vector<std::string> run_sympy_solver_visitor(
    const std::string& text,
    bool pade = false,
    bool cse = false,
    AstNodeType ret_nodetype = AstNodeType::DIFF_EQ_EXPRESSION,
    bool kinetic = false) {
    std::vector<std::string> results;

    // construct AST from text
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor().visit_program(*ast);

    // unroll loops and fold constants
    ConstantFolderVisitor().visit_program(*ast);
    LoopUnrollVisitor().visit_program(*ast);
    ConstantFolderVisitor().visit_program(*ast);
    SymtabVisitor().visit_program(*ast);

    if (kinetic) {
        KineticBlockVisitor().visit_program(*ast);
    }

    // run SympySolver on AST
    SympySolverVisitor(pade, cse).visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    // run lookup visitor to extract results from AST
    for (const auto& eq: collect_nodes(*ast, {ret_nodetype})) {
        results.push_back(to_nmodl(eq));
    }

    return results;
}

// check if in a list of vars (like LOCAL) there are duplicates
bool is_unique_vars(std::string result) {
    result.erase(std::remove(result.begin(), result.end(), ','), result.end());
    std::stringstream ss(result);
    std::string token;

    std::unordered_set<std::string> old_vars;

    while (getline(ss, token, ' ')) {
        if (!old_vars.insert(token).second) {
            return false;
        }
    }
    return true;
}


/**
 * \brief Compare nmodl blocks that contain systems of equations (i.e. derivative, linear, etc.)
 *
 * This is basically and advanced string == string comparison where we detect the (various) systems
 * of equations and check if they are equivalent. Implemented mostly in python since we need a call
 * to sympy to simplify the equations.
 *
 * - compare_systems_of_eq The core of the code. \p result_dict and \p expected_dict are
 * dictionaries that represent the systems of equations in this way:
 *
 *   a = b*x + c -> result_dict['a'] = 'b*x + c'
 *
 *   where the variable \p a become a key \p k of the dictionary.
 *
 *   In there we go over all the equations in \p result_dict and \p expected_dict and check that
 * result_dict[k] - expected_dict[k] simplifies to 0.
 *
 * - sanitize is to transform the equations in something treatable by sympy (i.e. pow(dt, 3) ->
 * dt**3
 * - reduce back-substitution of the temporary variables
 *
 * \p require_fail requires that the equations are different. Used only for unit-test this function
 *
 * \warning do not use this method when there are tmp variables not in the form: tmp_<number>
 */
void compare_blocks(const std::string& result,
                    const std::string& expected,
                    const bool require_fail = false) {
    using namespace pybind11::literals;

    auto locals =
        pybind11::dict("result"_a = result, "expected"_a = expected, "is_equal"_a = false);
    pybind11::exec(R"(
                    # Comments are in the doxygen for better highlighting
                    def compare_blocks(result, expected):
    
                        def sanitize(s):
                            import re
                            d = {'\[(\d+)\]':'_\\1',  'pow\((\w+), ?(\d+)\)':'\\1**\\2', 'beta': 'beta_var', 'gamma': 'gamma_var'}
                            out = s
                            for key, val in d.items():
                                out = re.sub(key, val, out)
                            return out
    
                        def compare_systems_of_eq(result_dict, expected_dict):
                            from sympy.parsing.sympy_parser import parse_expr
                            try:
                                for k, v in result_dict.items():
                                    if parse_expr(f'simplify(({v})-({expected_dict[k]}))'):
                                        return False
                            except KeyError:
                                return False

                            result_dict.clear()
                            expected_dict.clear()
                            return True

                        def reduce(s):
                            max_tmp = -1
                            d = {}

                            sout = ""
                            # split of sout and a dict with the tmp variables
                            for line in s.split('\n'):
                                line_split = line.lstrip().split('=')

                                if len(line_split) == 2 and line_split[0].startswith('tmp_'):
                                    # back-substitution of tmp variables in tmp variables
                                    tmp_var = line_split[0].strip()
                                    if tmp_var in d:
                                        continue

                                    max_tmp = max(max_tmp, int(tmp_var[4:]))
                                    for k, v in d.items():
                                        line_split[1] = line_split[1].replace(k, f'({v})')
                                    d[tmp_var] = line_split[1]
                                elif 'LOCAL' in line:
                                    sout += line.split('tmp_0')[0] + '\n'
                                else:
                                    sout += line + '\n'

                            # Back-substitution of the tmps
                            # so that we do not replace tmp_11 with (tmp_1)1
                            for j in range(max_tmp, -1, -1):
                                k = f'tmp_{j}'
                                sout = sout.replace(k, f'({d[k]})')

                            return sout

                        result = reduce(sanitize(result)).split('\n')
                        expected = reduce(sanitize(expected)).split('\n')

                        if len(result) != len(expected):
                            return False

                        result_dict = {}
                        expected_dict = {}
                        for token1, token2 in zip(result, expected):
                            if token1 == token2:
                                if not compare_systems_of_eq(result_dict, expected_dict):
                                    return False
                                continue

                            eq1 = token1.split('=')
                            eq2 = token2.split('=')
                            if len(eq1) == 2 and len(eq2) == 2:
                                result_dict[eq1[0]] = eq1[1]
                                expected_dict[eq2[0]] = eq2[1]
                                continue

                            return False
                        return compare_systems_of_eq(result_dict, expected_dict)

                    is_equal = compare_blocks(result, expected))",
                   pybind11::globals(),
                   locals);

    // Error log
    if (require_fail == locals["is_equal"].cast<bool>()) {
        if (require_fail) {
            REQUIRE(result != expected);
        } else {
            REQUIRE(result == expected);
        }
    } else {  // so that we signal to ctest that an assert was performed
        REQUIRE(true);
    }
}


void run_sympy_visitor_passes(ast::Program& node) {
    // construct symbol table from AST
    SymtabVisitor v_symtab;
    v_symtab.visit_program(node);

    // run SympySolver on AST several times
    SympySolverVisitor v_sympy1;
    v_sympy1.visit_program(node);
    v_sympy1.visit_program(node);

    // also use a second instance of SympySolver
    SympySolverVisitor v_sympy2;
    v_sympy2.visit_program(node);
    v_sympy1.visit_program(node);
    v_sympy2.visit_program(node);
}


std::string ast_to_string(ast::Program& node) {
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(node);
    return stream.str();
}

SCENARIO("Check compare_blocks in sympy unit tests", "[visitor][sympy]") {
    GIVEN("Empty strings") {
        THEN("Strings are equal") {
            compare_blocks("", "");
        }
    }
    GIVEN("Equivalent equation") {
        THEN("Strings are equal") {
            compare_blocks("a = 3*b + c", "a = 2*b + b + c");
        }
    }
    GIVEN("Equivalent systems of equations") {
        std::string result = R"(
        x = 3*b + c
        y = 2*a + b)";
        std::string expected = R"(
        x = b+2*b + c
        y = 2*a + 2*b-b)";
        THEN("Systems of equations are equal") {
            compare_blocks(result, expected);
        }
    }
    GIVEN("Equivalent systems of equations with brackets") {
        std::string result = R"(
        DERIVATIVE {
        A[0] = 3*b + c
        y = pow(a, 3) + b
        })";
        std::string expected = R"(
        DERIVATIVE {
        tmp_0 = a + c
        tmp_1 = tmp_0 - a
        A[0] = b+2*b + tmp_1
        y = pow(a, 2)*a + 2*b-b
        })";
        THEN("Blocks are equal") {
            compare_blocks(result, expected);
        }
    }
    GIVEN("Different systems of equations (additional space)") {
        std::string result = R"(
        DERIVATIVE {
        x = 3*b + c
        y = 2*a + b
        })";
        std::string expected = R"(
        DERIVATIVE  {
        x = b+2*b + c
        y = 2*a + 2*b-b
        })";
        THEN("Blocks are different") {
            compare_blocks(result, expected, true);
        }
    }
    GIVEN("Different systems of equations") {
        std::string result = R"(
        DERIVATIVE {
        tmp_0 = a - c
        tmp_1 = tmp_0 - a
        x = 3*b + tmp_1
        y = 2*a + b
        })";
        std::string expected = R"(
        DERIVATIVE {
        x = b+2*b + c
        y = 2*a + 2*b-b
        })";
        THEN("Blocks are different") {
            compare_blocks(result, expected, true);
        }
    }
}

SCENARIO("Check local vars name-clash prevention", "[visitor][sympy]") {
    GIVEN("LOCAL tmp") {
        std::string nmodl_text = R"(
        STATE {
            x y
        }
        BREAKPOINT  {
            SOLVE states METHOD sparse
        }
        DERIVATIVE states {
            LOCAL tmp, b
            x' = tmp + b
            y' = tmp + b
        })";
        THEN("There are no duplicate vars in LOCAL") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, true, true, AstNodeType::LOCAL_LIST_STATEMENT);
            REQUIRE(!result.empty());
            REQUIRE(is_unique_vars(result[0]));
        }
    }
    GIVEN("LOCAL tmp_0") {
        std::string nmodl_text = R"(
        STATE {
            x y
        }
        BREAKPOINT  {
            SOLVE states METHOD sparse
        }
        DERIVATIVE states {
            LOCAL tmp_0, b
            x' = tmp_0 + b
            y' = tmp_0 + b
        })";
        THEN("There are no duplicate vars in LOCAL") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, true, true, AstNodeType::LOCAL_LIST_STATEMENT);
            REQUIRE(!result.empty());
            REQUIRE(is_unique_vars(result[0]));
        }
    }
}

SCENARIO("Solve ODEs with cnexp or euler method using SympySolverVisitor",
         "[visitor][sympy][cnexp][euler]") {
    GIVEN("Derivative block without ODE, solver method cnexp") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states METHOD cnexp
            }
            DERIVATIVE states {
                m = m + h
            }
        )";
        THEN("No ODEs found - do nothing") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.empty());
        }
    }
    GIVEN("Derivative block with ODES, solver method is euler") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states METHOD euler
            }
            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
                z = a*b + c
            }
        )";
        THEN("Construct forwards Euler solutions") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 2);
            REQUIRE(result[0] == "m = (-dt*(m-mInf)+m*mTau)/mTau");
            REQUIRE(result[1] == "h = (-dt*(h-hInf)+h*hTau)/hTau");
        }
    }
    GIVEN("Derivative block with calling external functions passes sympy") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states METHOD euler
            }
            DERIVATIVE states {
                m' = sawtooth(m)
                n' = sin(n)
                p' = my_user_func(p)
            }
        )";
        THEN("Construct forward Euler interpreting external functions as symbols") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 3);
            REQUIRE(result[0] == "m = dt*sawtooth(m)+m");
            REQUIRE(result[1] == "n = dt*sin(n)+n");
            REQUIRE(result[2] == "p = dt*my_user_func(p)+p");
        }
    }
    GIVEN("Derivative block with ODE, 1 state var in array, solver method euler") {
        std::string nmodl_text = R"(
            STATE {
                m[1]
            }
            BREAKPOINT  {
                SOLVE states METHOD euler
            }
            DERIVATIVE states {
                m'[0] = (mInf-m[0])/mTau
            }
        )";
        THEN("Construct forwards Euler solutions") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 1);
            REQUIRE(result[0] == "m[0] = (dt*(mInf-m[0])+mTau*m[0])/mTau");
        }
    }
    GIVEN("Derivative block with ODE, 1 state var in array, solver method cnexp") {
        std::string nmodl_text = R"(
            STATE {
                m[1]
            }
            BREAKPOINT  {
                SOLVE states METHOD cnexp
            }
            DERIVATIVE states {
                m'[0] = (mInf-m[0])/mTau
            }
        )";
        THEN("Construct forwards Euler solutions") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 1);
            REQUIRE(result[0] == "m[0] = mInf-(mInf-m[0])*exp(-dt/mTau)");
        }
    }
    GIVEN("Derivative block with linear ODES, solver method cnexp") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states METHOD cnexp
            }
            DERIVATIVE states {
                m' = (mInf-m)/mTau
                z = a*b + c
                h' = hInf/hTau - h/hTau
            }
        )";
        THEN("Integrate equations analytically") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 2);
            REQUIRE(result[0] == "m = mInf-(-m+mInf)*exp(-dt/mTau)");
            REQUIRE(result[1] == "h = hInf-(-h+hInf)*exp(-dt/hTau)");
        }
    }
    GIVEN("Derivative block including non-linear but solvable ODES, solver method cnexp") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states METHOD cnexp
            }
            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = c2 * h*h
            }
        )";
        THEN("Integrate equations analytically") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 2);
            REQUIRE(result[0] == "m = mInf-(-m+mInf)*exp(-dt/mTau)");
            REQUIRE(result[1] == "h = -h/(c2*dt*h-1.0)");
        }
    }
    GIVEN("Derivative block including array of 2 state vars, solver method cnexp") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }
            STATE {
                X[2]
            }
            DERIVATIVE states {
                X'[0] = (mInf-X[0])/mTau
                X'[1] = c2 * X[1]*X[1]
            }
        )";
        THEN("Integrate equations analytically") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 2);
            REQUIRE(result[0] == "X[0] = mInf-(mInf-X[0])*exp(-dt/mTau)");
            REQUIRE(result[1] == "X[1] = -X[1]/(c2*dt*X[1]-1.0)");
        }
    }
    GIVEN("Derivative block including loop over array vars, solver method cnexp") {
        std::string nmodl_text = R"(
            DEFINE N 3
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }
            ASSIGNED {
                mTau[N]
            }
            STATE {
                X[N]
            }
            DERIVATIVE states {
                FROM i=0 TO N-1 {
                    X'[i] = (mInf-X[i])/mTau[i]
                }
            }
        )";
        THEN("Integrate equations analytically") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 3);
            REQUIRE(result[0] == "X[0] = mInf-(mInf-X[0])*exp(-dt/mTau[0])");
            REQUIRE(result[1] == "X[1] = mInf-(mInf-X[1])*exp(-dt/mTau[1])");
            REQUIRE(result[2] == "X[2] = mInf-(mInf-X[2])*exp(-dt/mTau[2])");
        }
    }
    GIVEN("Derivative block including loop over array vars, solver method euler") {
        std::string nmodl_text = R"(
            DEFINE N 3
            BREAKPOINT {
                SOLVE states METHOD euler
            }
            ASSIGNED {
                mTau[N]
            }
            STATE {
                X[N]
            }
            DERIVATIVE states {
                FROM i=0 TO N-1 {
                    X'[i] = (mInf-X[i])/mTau[i]
                }
            }
        )";
        THEN("Integrate equations analytically") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 3);
            REQUIRE(result[0] == "X[0] = (dt*(mInf-X[0])+X[0]*mTau[0])/mTau[0]");
            REQUIRE(result[1] == "X[1] = (dt*(mInf-X[1])+X[1]*mTau[1])/mTau[1]");
            REQUIRE(result[2] == "X[2] = (dt*(mInf-X[2])+X[2]*mTau[2])/mTau[2]");
        }
    }
    GIVEN("Derivative block including ODES that can't currently be solved, solver method cnexp") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states METHOD cnexp
            }
            DERIVATIVE states {
                z' = a/z +  b/z/z
                h' = c2 * h*h
                x' = a
                y' = c3 * y*y*y
            }
        )";
        THEN("Integrate equations analytically where possible, otherwise leave untouched") {
            auto result = run_sympy_solver_visitor(nmodl_text);
            REQUIRE(result.size() == 4);
            /// sympy 1.9 able to solve ode but not older versions
            REQUIRE((result[0] == "z' = a/z+b/z/z" ||
                     result[0] ==
                         "z = (0.5*pow(a, 2)*pow(z, 2)-a*b*z+pow(b, 2)*log(a*z+b))/pow(a, 3)"));
            REQUIRE(result[1] == "h = -h/(c2*dt*h-1.0)");
            REQUIRE(result[2] == "x = a*dt+x");
            /// sympy 1.4 able to solve ode but not older versions
            REQUIRE((result[3] == "y' = c3*y*y*y" ||
                     result[3] == "y = sqrt(-pow(y, 2)/(2.0*c3*dt*pow(y, 2)-1.0))"));
        }
    }
    GIVEN("Derivative block with cnexp solver method, AST after SympySolver pass") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states METHOD cnexp
            }
            DERIVATIVE states {
                m' = (mInf-m)/mTau
            }
        )";
        // construct AST from text
        NmodlDriver driver;
        auto ast = driver.parse_string(nmodl_text);

        // construct symbol table from AST
        SymtabVisitor().visit_program(*ast);

        // run SympySolver on AST
        SympySolverVisitor().visit_program(*ast);

        std::string AST_string = ast_to_string(*ast);

        THEN("More SympySolver passes do nothing to the AST and don't throw") {
            REQUIRE_NOTHROW(run_sympy_visitor_passes(*ast));
            REQUIRE(AST_string == ast_to_string(*ast));
        }
    }
}

SCENARIO("Solve ODEs with derivimplicit method using SympySolverVisitor",
         "[visitor][sympy][derivimplicit]") {
    GIVEN("Derivative block with derivimplicit solver method and conditional block") {
        std::string nmodl_text = R"(
            STATE {
                m
            }
            BREAKPOINT  {
                SOLVE states METHOD derivimplicit
            }
            DERIVATIVE states {
                IF (mInf == 1) {
                    mInf = mInf+1
                }
                m' = (mInf-m)/mTau
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[1]{
                    LOCAL old_m
                }{
                    IF (mInf == 1) {
                        mInf = mInf+1
                    }
                    old_m = m
                }{
                    nmodl_eigen_x[0] = m
                }{
                    nmodl_eigen_f[0] = (dt*(-nmodl_eigen_x[0]+mInf)+mTau*(-nmodl_eigen_x[0]+old_m))/(dt*mTau)
                    nmodl_eigen_j[0] = (-dt-mTau)/(dt*mTau)
                }{
                    m = nmodl_eigen_x[0]
                }{
                }
            })";
        THEN("SympySolver correctly inserts ode to block") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }

    GIVEN("Derivative block, sparse, print in order") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            BREAKPOINT  {
                SOLVE states METHOD sparse
            }
            DERIVATIVE states {
                LOCAL a, b
                y' = a
                x' = b
            })";
        std::string expected_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL a, b, old_y, old_x
                }{
                    old_y = y
                    old_x = x
                }{
                    nmodl_eigen_x[0] = x
                    nmodl_eigen_x[1] = y
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[1]+a*dt+old_y)/dt
                    nmodl_eigen_j[0] = 0
                    nmodl_eigen_j[2] = -1/dt
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[0]+b*dt+old_x)/dt
                    nmodl_eigen_j[1] = -1/dt
                    nmodl_eigen_j[3] = 0
                }{
                    x = nmodl_eigen_x[0]
                    y = nmodl_eigen_x[1]
                }{
                }
            })";

        THEN("Construct & solve linear system for backwards Euler") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block, sparse, print in order, vectors") {
        std::string nmodl_text = R"(
            STATE {
                M[2]
            }
            BREAKPOINT  {
                SOLVE states METHOD sparse
            }
            DERIVATIVE states {
                LOCAL a, b
                M'[1] = a
                M'[0] = b
            })";
        std::string expected_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL a, b, old_M_1, old_M_0
                }{
                    old_M_1 = M[1]
                    old_M_0 = M[0]
                }{
                    nmodl_eigen_x[0] = M[0]
                    nmodl_eigen_x[1] = M[1]
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[1]+a*dt+old_M_1)/dt
                    nmodl_eigen_j[0] = 0
                    nmodl_eigen_j[2] = -1/dt
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[0]+b*dt+old_M_0)/dt
                    nmodl_eigen_j[1] = -1/dt
                    nmodl_eigen_j[3] = 0
                }{
                    M[0] = nmodl_eigen_x[0]
                    M[1] = nmodl_eigen_x[1]
                }{
                }
            })";

        THEN("Construct & solve linear system for backwards Euler") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block, sparse, derivatives mixed with local variable reassignment") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            BREAKPOINT  {
                SOLVE states METHOD sparse
            }
            DERIVATIVE states {
                LOCAL a, b
                    x' = a
                    b = b + 1
                    y' = b
            })";
        std::string expected_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL a, b, old_x, old_y
                }{
                    old_x = x
                    old_y = y
                }{
                    nmodl_eigen_x[0] = x
                    nmodl_eigen_x[1] = y
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+a*dt+old_x)/dt
                    nmodl_eigen_j[0] = -1/dt
                    nmodl_eigen_j[2] = 0
                    b = b+1
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+b*dt+old_y)/dt
                    nmodl_eigen_j[1] = 0
                    nmodl_eigen_j[3] = -1/dt
                }{
                    x = nmodl_eigen_x[0]
                    y = nmodl_eigen_x[1]
                }{
                }
            })";

        THEN("Construct & solve linear system for backwards Euler") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN(
        "Throw exception during derivative variable reassignment interleaved in the differential "
        "equation set") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            BREAKPOINT  {
                SOLVE states METHOD sparse
            }
            DERIVATIVE states {
                LOCAL a, b
                    x' = a
                    x = x + 1
                    y' = b + x
            })";

        THEN(
            "Throw an error because state variable assignments are not allowed inside the system "
            "of differential "
            "equations") {
            REQUIRE_THROWS_WITH(
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK),
                Catch::Matchers::ContainsSubstring(
                    "State variable assignment(s) interleaved in system of "
                    "equations/differential equations") &&
                    Catch::Matchers::StartsWith("SympyReplaceSolutionsVisitor"));
        }
    }
    GIVEN("Derivative block in control flow block") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            BREAKPOINT  {
                SOLVE states METHOD sparse
            }
            DERIVATIVE states {
                LOCAL a, b
                if (a == 1) {
                    x' = a
                    y' = b
                }
            })";
        std::string expected_result = R"(
            DERIVATIVE states {
                LOCAL a, b
                IF (a == 1) {
                    EIGEN_NEWTON_SOLVE[2]{
                        LOCAL old_x, old_y
                    }{
                        old_x = x
                        old_y = y
                    }{
                        nmodl_eigen_x[0] = x
                        nmodl_eigen_x[1] = y
                    }{
                        nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+a*dt+old_x)/dt
                        nmodl_eigen_j[0] = -1/dt
                        nmodl_eigen_j[2] = 0
                        nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+b*dt+old_y)/dt
                        nmodl_eigen_j[1] = 0
                        nmodl_eigen_j[3] = -1/dt
                    }{
                        x = nmodl_eigen_x[0]
                        y = nmodl_eigen_x[1]
                    }{
                    }
                }
            })";

        THEN("Construct & solve linear system for backwards Euler") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN(
        "Derivative block, sparse, coupled derivatives mixed with reassignment and control flow "
        "block") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            BREAKPOINT  {
                SOLVE states METHOD sparse
            }
            DERIVATIVE states {
                LOCAL a, b
                    x' = a * y+b
                    if (b == 1) {
                        a = a + 1
                    }
                    y' = x + a*y
            })";
        std::string expected_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL a, b, old_x, old_y
                }{
                    old_x = x
                    old_y = y
                }{
                    nmodl_eigen_x[0] = x
                    nmodl_eigen_x[1] = y
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(nmodl_eigen_x[1]*a+b)+old_x)/dt
                    nmodl_eigen_j[0] = -1/dt
                    nmodl_eigen_j[2] = a
                    IF (b == 1) {
                        a = a+1
                    }
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]+nmodl_eigen_x[1]*a)+old_y)/dt
                    nmodl_eigen_j[1] = 1.0
                    nmodl_eigen_j[3] = a-1/dt
                }{
                    x = nmodl_eigen_x[0]
                    y = nmodl_eigen_x[1]
                }{
                }
            })";
        std::string expected_result_cse = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL a, b, old_x, old_y
                }{
                    old_x = x
                    old_y = y
                }{
                    nmodl_eigen_x[0] = x
                    nmodl_eigen_x[1] = y
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(nmodl_eigen_x[1]*a+b)+old_x)/dt
                    nmodl_eigen_j[0] = -1/dt
                    nmodl_eigen_j[2] = a
                    IF (b == 1) {
                        a = a+1
                    }
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]+nmodl_eigen_x[1]*a)+old_y)/dt
                    nmodl_eigen_j[1] = 1.0
                    nmodl_eigen_j[3] = a-1/dt
                }{
                    x = nmodl_eigen_x[0]
                    y = nmodl_eigen_x[1]
                }{
                }
            })";

        THEN("Construct & solve linear system for backwards Euler") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            auto result_cse =
                run_sympy_solver_visitor(nmodl_text, true, true, AstNodeType::DERIVATIVE_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
            compare_blocks(reindent_text(result_cse[0]), reindent_text(expected_result_cse));
        }
    }

    GIVEN("Derivative block of coupled & linear ODES, solver method sparse") {
        std::string nmodl_text = R"(
            STATE {
                x y z
            }
            BREAKPOINT  {
                SOLVE states METHOD sparse
            }
            DERIVATIVE states {
                LOCAL a, b, c, d, h
                x' = a*z + b*h
                y' = c + 2*x
                z' = d*z - y
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[3]{
                    LOCAL a, b, c, d, h, old_x, old_y, old_z
                }{
                    old_x = x
                    old_y = y
                    old_z = z
                }{
                    nmodl_eigen_x[0] = x
                    nmodl_eigen_x[1] = y
                    nmodl_eigen_x[2] = z
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(nmodl_eigen_x[2]*a+b*h)+old_x)/dt
                    nmodl_eigen_j[0] = -1/dt
                    nmodl_eigen_j[3] = 0
                    nmodl_eigen_j[6] = a
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(2.0*nmodl_eigen_x[0]+c)+old_y)/dt
                    nmodl_eigen_j[1] = 2.0
                    nmodl_eigen_j[4] = -1/dt
                    nmodl_eigen_j[7] = 0
                    nmodl_eigen_f[2] = (-nmodl_eigen_x[2]+dt*(-nmodl_eigen_x[1]+nmodl_eigen_x[2]*d)+old_z)/dt
                    nmodl_eigen_j[2] = 0
                    nmodl_eigen_j[5] = -1.0
                    nmodl_eigen_j[8] = d-1/dt
                }{
                    x = nmodl_eigen_x[0]
                    y = nmodl_eigen_x[1]
                    z = nmodl_eigen_x[2]
                }{
                }
            })";
        std::string expected_cse_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[3]{
                    LOCAL a, b, c, d, h, old_x, old_y, old_z
                }{
                    old_x = x
                    old_y = y
                    old_z = z
                }{
                    nmodl_eigen_x[0] = x
                    nmodl_eigen_x[1] = y
                    nmodl_eigen_x[2] = z
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(nmodl_eigen_x[2]*a+b*h)+old_x)/dt
                    nmodl_eigen_j[0] = -1/dt
                    nmodl_eigen_j[3] = 0
                    nmodl_eigen_j[6] = a
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(2.0*nmodl_eigen_x[0]+c)+old_y)/dt
                    nmodl_eigen_j[1] = 2.0
                    nmodl_eigen_j[4] = -1/dt
                    nmodl_eigen_j[7] = 0
                    nmodl_eigen_f[2] = (-nmodl_eigen_x[2]+dt*(-nmodl_eigen_x[1]+nmodl_eigen_x[2]*d)+old_z)/dt
                    nmodl_eigen_j[2] = 0
                    nmodl_eigen_j[5] = -1.0
                    nmodl_eigen_j[8] = d-1/dt
                }{
                    x = nmodl_eigen_x[0]
                    y = nmodl_eigen_x[1]
                    z = nmodl_eigen_x[2]
                }{
                }
            })";

        THEN("Construct & solve linear system for backwards Euler") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            auto result_cse =
                run_sympy_solver_visitor(nmodl_text, true, true, AstNodeType::DERIVATIVE_BLOCK);

            compare_blocks(result[0], reindent_text(expected_result));
            compare_blocks(result_cse[0], reindent_text(expected_cse_result));
        }
    }
    GIVEN("Derivative block including ODES with sparse method (from nmodl paper)") {
        std::string nmodl_text = R"(
            STATE {
                mc m
            }
            BREAKPOINT  {
                SOLVE scheme1 METHOD sparse
            }
            DERIVATIVE scheme1 {
                mc' = -a*mc + b*m
                m' = a*mc - b*m
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE scheme1 {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL old_mc, old_m
                }{
                    old_mc = mc
                    old_m = m
                }{
                    nmodl_eigen_x[0] = mc
                    nmodl_eigen_x[1] = m
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*a+nmodl_eigen_x[1]*b)+old_mc)/dt
                    nmodl_eigen_j[0] = -a-1/dt
                    nmodl_eigen_j[2] = b
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]*a-nmodl_eigen_x[1]*b)+old_m)/dt
                    nmodl_eigen_j[1] = a
                    nmodl_eigen_j[3] = -b-1/dt
                }{
                    mc = nmodl_eigen_x[0]
                    m = nmodl_eigen_x[1]
                }{
                }
            })";
        THEN("Construct & solve linear system") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block with ODES with sparse method, CONSERVE statement of form m = ...") {
        std::string nmodl_text = R"(
            STATE {
                mc m
            }
            BREAKPOINT  {
                SOLVE scheme1 METHOD sparse
            }
            DERIVATIVE scheme1 {
                mc' = -a*mc + b*m
                m' = a*mc - b*m
                CONSERVE m = 1 - mc
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE scheme1 {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL old_mc
                }{
                    old_mc = mc
                }{
                    nmodl_eigen_x[0] = mc
                    nmodl_eigen_x[1] = m
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*a+nmodl_eigen_x[1]*b)+old_mc)/dt
                    nmodl_eigen_j[0] = -a-1/dt
                    nmodl_eigen_j[2] = b
                    nmodl_eigen_f[1] = -nmodl_eigen_x[0]-nmodl_eigen_x[1]+1.0
                    nmodl_eigen_j[1] = -1.0
                    nmodl_eigen_j[3] = -1.0
                }{
                    mc = nmodl_eigen_x[0]
                    m = nmodl_eigen_x[1]
                }{
                }
            })";
        THEN("Construct & solve linear system, replace ODE for m with rhs of CONSERVE statement") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN(
        "Derivative block with ODES with sparse method, invalid CONSERVE statement of form m + mc "
        "= ...") {
        std::string nmodl_text = R"(
            STATE {
                mc m
            }
            BREAKPOINT  {
                SOLVE scheme1 METHOD sparse
            }
            DERIVATIVE scheme1 {
                mc' = -a*mc + b*m
                m' = a*mc - b*m
                CONSERVE m + mc = 1
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE scheme1 {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL old_mc, old_m
                }{
                    old_mc = mc
                    old_m = m
                }{
                    nmodl_eigen_x[0] = mc
                    nmodl_eigen_x[1] = m
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*a+nmodl_eigen_x[1]*b)+old_mc)/dt
                    nmodl_eigen_j[0] = -a-1/dt
                    nmodl_eigen_j[2] = b
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]*a-nmodl_eigen_x[1]*b)+old_m)/dt
                    nmodl_eigen_j[1] = a
                    nmodl_eigen_j[3] = -b-1/dt
                }{
                    mc = nmodl_eigen_x[0]
                    m = nmodl_eigen_x[1]
                }{
                }
            })";
        THEN("Construct & solve linear system, ignore invalid CONSERVE statement") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block with ODES with sparse method, two CONSERVE statements") {
        std::string nmodl_text = R"(
            STATE {
                c1 o1 o2 p0 p1
            }
            BREAKPOINT  {
                SOLVE ihkin METHOD sparse
            }
            DERIVATIVE ihkin {
                LOCAL alpha, beta, k3p, k4, k1ca, k2
                evaluate_fct(v, cai)
                CONSERVE p1 = 1-p0
                CONSERVE o2 = 1-c1-o1
                c1' = (-1*(alpha*c1-beta*o1))
                o1' = (1*(alpha*c1-beta*o1))+(-1*(k3p*o1-k4*o2))
                o2' = (1*(k3p*o1-k4*o2))
                p0' = (-1*(k1ca*p0-k2*p1))
                p1' = (1*(k1ca*p0-k2*p1))
            })";
        std::string expected_result = R"(
            DERIVATIVE ihkin {
                EIGEN_NEWTON_SOLVE[5]{
                    LOCAL alpha, beta, k3p, k4, k1ca, k2, old_c1, old_o1, old_p0
                }{
                    evaluate_fct(v, cai)
                    old_c1 = c1
                    old_o1 = o1
                    old_p0 = p0
                }{
                    nmodl_eigen_x[0] = c1
                    nmodl_eigen_x[1] = o1
                    nmodl_eigen_x[2] = o2
                    nmodl_eigen_x[3] = p0
                    nmodl_eigen_x[4] = p1
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*alpha+nmodl_eigen_x[1]*beta)+old_c1)/dt
                    nmodl_eigen_j[0] = -alpha-1/dt
                    nmodl_eigen_j[5] = beta
                    nmodl_eigen_j[10] = 0
                    nmodl_eigen_j[15] = 0
                    nmodl_eigen_j[20] = 0
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]*alpha-nmodl_eigen_x[1]*beta-nmodl_eigen_x[1]*k3p+nmodl_eigen_x[2]*k4)+old_o1)/dt
                    nmodl_eigen_j[1] = alpha
                    nmodl_eigen_j[6] = -beta-k3p-1/dt
                    nmodl_eigen_j[11] = k4
                    nmodl_eigen_j[16] = 0
                    nmodl_eigen_j[21] = 0
                    nmodl_eigen_f[2] = -nmodl_eigen_x[0]-nmodl_eigen_x[1]-nmodl_eigen_x[2]+1.0
                    nmodl_eigen_j[2] = -1.0
                    nmodl_eigen_j[7] = -1.0
                    nmodl_eigen_j[12] = -1.0
                    nmodl_eigen_j[17] = 0
                    nmodl_eigen_j[22] = 0
                    nmodl_eigen_f[3] = (-nmodl_eigen_x[3]+dt*(-nmodl_eigen_x[3]*k1ca+nmodl_eigen_x[4]*k2)+old_p0)/dt
                    nmodl_eigen_j[3] = 0
                    nmodl_eigen_j[8] = 0
                    nmodl_eigen_j[13] = 0
                    nmodl_eigen_j[18] = -k1ca-1/dt
                    nmodl_eigen_j[23] = k2
                    nmodl_eigen_f[4] = -nmodl_eigen_x[3]-nmodl_eigen_x[4]+1.0
                    nmodl_eigen_j[4] = 0
                    nmodl_eigen_j[9] = 0
                    nmodl_eigen_j[14] = 0
                    nmodl_eigen_j[19] = -1.0
                    nmodl_eigen_j[24] = -1.0
                }{
                    c1 = nmodl_eigen_x[0]
                    o1 = nmodl_eigen_x[1]
                    o2 = nmodl_eigen_x[2]
                    p0 = nmodl_eigen_x[3]
                    p1 = nmodl_eigen_x[4]
                }{
                }
            })";
        THEN(
            "Construct & solve linear system, replacing ODEs for p1 and o2 with CONSERVE statement "
            "algebraic relations") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block including ODES with sparse method - single var in array") {
        std::string nmodl_text = R"(
            STATE {
                W[1]
            }
            ASSIGNED {
                A[2]
                B[1]
            }
            BREAKPOINT  {
                SOLVE scheme1 METHOD sparse
            }
            DERIVATIVE scheme1 {
                W'[0] = -A[0]*W[0] + B[0]*W[0] + 3*A[1]
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE scheme1 {
                EIGEN_NEWTON_SOLVE[1]{
                    LOCAL old_W_0
                }{
                    old_W_0 = W[0]
                }{
                    nmodl_eigen_x[0] = W[0]
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*A[0]+nmodl_eigen_x[0]*B[0]+3.0*A[1])+old_W_0)/dt
                    nmodl_eigen_j[0] = -A[0]+B[0]-1/dt
                }{
                    W[0] = nmodl_eigen_x[0]
                }{
                }
            })";
        THEN("Construct & solver linear system") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block including ODES with sparse method - array vars") {
        std::string nmodl_text = R"(
            STATE {
                M[2]
            }
            ASSIGNED {
                A[2]
                B[2]
            }
            BREAKPOINT  {
                SOLVE scheme1 METHOD sparse
            }
            DERIVATIVE scheme1 {
                M'[0] = -A[0]*M[0] + B[0]*M[1]
                M'[1] = A[1]*M[0] - B[1]*M[1]
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE scheme1 {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL old_M_0, old_M_1
                }{
                    old_M_0 = M[0]
                    old_M_1 = M[1]
                }{
                    nmodl_eigen_x[0] = M[0]
                    nmodl_eigen_x[1] = M[1]
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*A[0]+nmodl_eigen_x[1]*B[0])+old_M_0)/dt
                    nmodl_eigen_j[0] = -A[0]-1/dt
                    nmodl_eigen_j[2] = B[0]
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]*A[1]-nmodl_eigen_x[1]*B[1])+old_M_1)/dt
                    nmodl_eigen_j[1] = A[1]
                    nmodl_eigen_j[3] = -B[1]-1/dt
                }{
                    M[0] = nmodl_eigen_x[0]
                    M[1] = nmodl_eigen_x[1]
                }{
                }
            })";
        THEN("Construct & solver linear system") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block including ODES with derivimplicit method - single var in array") {
        std::string nmodl_text = R"(
            STATE {
                W[1]
            }
            ASSIGNED {
                A[2]
                B[1]
            }
            BREAKPOINT  {
                SOLVE scheme1 METHOD derivimplicit
            }
            DERIVATIVE scheme1 {
                W'[0] = -A[0]*W[0] + B[0]*W[0] + 3*A[1]
            }
        )";
        std::string expected_result = R"(
            DERIVATIVE scheme1 {
                EIGEN_NEWTON_SOLVE[1]{
                    LOCAL old_W_0
                }{
                    old_W_0 = W[0]
                }{
                    nmodl_eigen_x[0] = W[0]
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*A[0]+nmodl_eigen_x[0]*B[0]+3.0*A[1])+old_W_0)/dt
                    nmodl_eigen_j[0] = -A[0]+B[0]-1/dt
                }{
                    W[0] = nmodl_eigen_x[0]
                }{
                }
            })";
        THEN("Construct newton solve block") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN("Derivative block including ODES with derivimplicit method") {
        std::string nmodl_text = R"(
            STATE {
                m h n
            }
            BREAKPOINT  {
                SOLVE states METHOD derivimplicit
            }
            DERIVATIVE states {
                rates(v)
                m' = (minf-m)/mtau - 3*h
                h' = (hinf-h)/htau + m*m
                n' = (ninf-n)/ntau
            }
        )";
        /// new derivative block with EigenNewtonSolverBlock node
        std::string expected_result = R"(
            DERIVATIVE states {
                EIGEN_NEWTON_SOLVE[3]{
                    LOCAL old_m, old_h, old_n
                }{
                    rates(v)
                    old_m = m
                    old_h = h
                    old_n = n
                }{
                    nmodl_eigen_x[0] = m
                    nmodl_eigen_x[1] = h
                    nmodl_eigen_x[2] = n
                }{
                    nmodl_eigen_f[0] = -nmodl_eigen_x[0]/mtau-nmodl_eigen_x[0]/dt-3.0*nmodl_eigen_x[1]+minf/mtau+old_m/dt
                    nmodl_eigen_j[0] = (-dt-mtau)/(dt*mtau)
                    nmodl_eigen_j[3] = -3.0
                    nmodl_eigen_j[6] = 0
                    nmodl_eigen_f[1] = pow(nmodl_eigen_x[0], 2)-nmodl_eigen_x[1]/htau-nmodl_eigen_x[1]/dt+hinf/htau+old_h/dt
                    nmodl_eigen_j[1] = 2.0*nmodl_eigen_x[0]
                    nmodl_eigen_j[4] = (-dt-htau)/(dt*htau)
                    nmodl_eigen_j[7] = 0
                    nmodl_eigen_f[2] = (dt*(-nmodl_eigen_x[2]+ninf)+ntau*(-nmodl_eigen_x[2]+old_n))/(dt*ntau)
                    nmodl_eigen_j[2] = 0
                    nmodl_eigen_j[5] = 0
                    nmodl_eigen_j[8] = (-dt-ntau)/(dt*ntau)
                }{
                    m = nmodl_eigen_x[0]
                    h = nmodl_eigen_x[1]
                    n = nmodl_eigen_x[2]
                }{
                }
            })";
        THEN("Construct newton solve block") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            compare_blocks(result[0], reindent_text(expected_result));
        }
    }
    GIVEN("Multiple derivative blocks each with derivimplicit method") {
        std::string nmodl_text = R"(
            STATE {
                m h
            }
            BREAKPOINT {
                SOLVE states1 METHOD derivimplicit
                SOLVE states2 METHOD derivimplicit
            }

            DERIVATIVE states1 {
                m' = (minf-m)/mtau
                h' = (hinf-h)/htau + m*m
            }

            DERIVATIVE states2 {
                h' = (hinf-h)/htau + m*m
                m' = (minf-m)/mtau + h
            }
        )";
        /// EigenNewtonSolverBlock in each derivative block
        std::string expected_result_0 = R"(
            DERIVATIVE states1 {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL old_m, old_h
                }{
                    old_m = m
                    old_h = h
                }{
                    nmodl_eigen_x[0] = m
                    nmodl_eigen_x[1] = h
                }{
                    nmodl_eigen_f[0] = (dt*(-nmodl_eigen_x[0]+minf)+mtau*(-nmodl_eigen_x[0]+old_m))/(dt*mtau)
                    nmodl_eigen_j[0] = (-dt-mtau)/(dt*mtau)
                    nmodl_eigen_j[2] = 0
                    nmodl_eigen_f[1] = pow(nmodl_eigen_x[0], 2)-nmodl_eigen_x[1]/htau- nmodl_eigen_x[1]/dt+hinf/htau+old_h/dt
                    nmodl_eigen_j[1] = 2.0*nmodl_eigen_x[0]
                    nmodl_eigen_j[3] = (-dt-htau)/(dt*htau)
                }{
                    m = nmodl_eigen_x[0]
                    h = nmodl_eigen_x[1]
                }{
                }
            })";
        std::string expected_result_1 = R"(
            DERIVATIVE states2 {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL old_h, old_m
                }{
                    old_h = h
                    old_m = m
                }{
                    nmodl_eigen_x[0] = m
                    nmodl_eigen_x[1] = h
                }{
                    nmodl_eigen_f[0] = pow(nmodl_eigen_x[0], 2)-nmodl_eigen_x[1]/htau-nmodl_eigen_x[1]/dt+hinf/htau+old_h/dt
                    nmodl_eigen_j[0] = 2.0*nmodl_eigen_x[0]
                    nmodl_eigen_j[2] = (-dt-htau)/(dt*htau)
                    nmodl_eigen_f[1] = -nmodl_eigen_x[0]/mtau-nmodl_eigen_x[0]/dt+nmodl_eigen_x[1]+minf/mtau+old_m/dt
                    nmodl_eigen_j[1] = (-dt-mtau)/(dt*mtau)
                    nmodl_eigen_j[3] = 1.0
                }{
                    m = nmodl_eigen_x[0]
                    h = nmodl_eigen_x[1]
                }{
                }
            })";
        THEN("Construct newton solve block") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            CAPTURE(nmodl_text);
            compare_blocks(result[0], reindent_text(expected_result_0));
            compare_blocks(result[1], reindent_text(expected_result_1));
        }
    }
}


//=============================================================================
// LINEAR solve block tests
//=============================================================================

SCENARIO("LINEAR solve block (SympySolver Visitor)", "[sympy][linear]") {
    GIVEN("1 state-var symbolic LINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                x
            }
            LINEAR lin {
                ~ 2*a*x = 1
            })";
        std::string expected_text = R"(
            LINEAR lin {
                x = 0.5/a
            })";
        THEN("solve analytically") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
    GIVEN("2 state-var LINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            LINEAR lin {
                ~ x + 4*y = 5*a
                ~ x - y = 0
            })";
        std::string expected_text = R"(
            LINEAR lin {
                x = a
                y = a
            })";
        THEN("solve analytically") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
    GIVEN("Linear block, print in order, vectors") {
        std::string nmodl_text = R"(
            STATE {
                M[2]
            }
            LINEAR lin {
                ~ M[1] = M[0] + 1
                ~ M[0] = 2
            })";
        std::string expected_result = R"(
            LINEAR lin {
                M[1] = 3.0
                M[0] = 2.0
            })";

        THEN("Construct & solve linear system") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN("Linear block, by value replacement, interleaved") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            LINEAR lin {
                LOCAL a
                a = 0
                ~ x = y + a
                a = 1
                ~ y = a
                a = 2
            })";
        std::string expected_result = R"(
            LINEAR lin {
                LOCAL a
                a = 0
                x = 2.0*a
                a = 1
                y = a
                a = 2
            })";

        THEN("Construct & solve linear system") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN("Linear block in control flow block") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            LINEAR lin {
                LOCAL a
                if (a == 1) {
                    ~ x = y + a
                    ~ y = a
                }
            })";
        std::string expected_result = R"(
            LINEAR lin {
                LOCAL a
                IF (a == 1) {
                    x = 2.0*a
                    y = a
                }
            })";

        THEN("Construct & solve linear system") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN("Linear block, linear equations mixed with control flow blocks and reassignments") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            LINEAR lin {
                LOCAL a
                ~ x = y + a
                if (a == 1) {
                    a = a + 1
                    x = a + 1
                }
                ~ y = a
            })";
        std::string expected_result = R"(
            LINEAR lin {
                LOCAL a
                x = 2.0*a
                IF (a == 1) {
                    a = a+1
                    x = a+1
                }
                y = a
            })";

        THEN("Construct & solve linear system") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);

            compare_blocks(reindent_text(result[0]), reindent_text(expected_result));
        }
    }
    GIVEN("4 state-var LINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                w x y z
            }
            LINEAR lin {
                ~ w + z/3.2 = -2.0*y
                ~ x + 4*c*y = -5.343*a
                ~ a + x/b + z - y = 0.842*b*b
                ~ x + 1.3*y - 0.1*z/(a*a*b) = 1.43543/c
            })";
        std::string expected_text = R"(
            LINEAR lin {
                EIGEN_LINEAR_SOLVE[4]{
                }{
                }{
                    nmodl_eigen_x[0] = w
                    nmodl_eigen_x[1] = x
                    nmodl_eigen_x[2] = y
                    nmodl_eigen_x[3] = z
                    nmodl_eigen_f[0] = 0
                    nmodl_eigen_f[1] = 5.343*a
                    nmodl_eigen_f[2] = a-0.84199999999999997*pow(b, 2)
                    nmodl_eigen_f[3] = -1.43543/c
                    nmodl_eigen_j[0] = -1.0
                    nmodl_eigen_j[4] = 0
                    nmodl_eigen_j[8] = -2.0
                    nmodl_eigen_j[12] = -0.3125
                    nmodl_eigen_j[1] = 0
                    nmodl_eigen_j[5] = -1.0
                    nmodl_eigen_j[9] = -4.0*c
                    nmodl_eigen_j[13] = 0
                    nmodl_eigen_j[2] = 0
                    nmodl_eigen_j[6] = -1/b
                    nmodl_eigen_j[10] = 1.0
                    nmodl_eigen_j[14] = -1.0
                    nmodl_eigen_j[3] = 0
                    nmodl_eigen_j[7] = -1.0
                    nmodl_eigen_j[11] = -1.3
                    nmodl_eigen_j[15] = 0.10000000000000001/(pow(a, 2)*b)
                }{
                    w = nmodl_eigen_x[0]
                    x = nmodl_eigen_x[1]
                    y = nmodl_eigen_x[2]
                    z = nmodl_eigen_x[3]
                }{
                }
            })";
        THEN("return matrix system to solve") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            compare_blocks(reindent_text(result[0]), reindent_text(expected_text));
        }
    }

    GIVEN("LINEAR solve block with an explicit SOLVEFOR statement") {
        std::string nmodl_text = R"(
            STATE {
                x
                y
                z
            }
            LINEAR lin SOLVEFOR x, y {
                ~ 3 * x = v - y
                ~ x = z * y - 5
            })";
        std::string expected_text = R"(
            LINEAR lin  SOLVEFOR x,y{
                y = (v+15.0)/(3.0*z+1.0)
                x = (v*z-5.0)/(3.0*z+1.0)
            })";
        THEN("solve analytically") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
}

//=============================================================================
// NONLINEAR solve block tests
//=============================================================================

SCENARIO("Solve NONLINEAR block using SympySolver Visitor", "[visitor][solver][sympy][nonlinear]") {
    GIVEN("1 state-var numeric NONLINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                x
            }
            NONLINEAR nonlin {
                ~ x = 5
            })";
        std::string expected_text = R"(
            NONLINEAR nonlin {
                EIGEN_NEWTON_SOLVE[1]{
                }{
                }{
                    nmodl_eigen_x[0] = x
                }{
                    nmodl_eigen_f[0] = 5.0-nmodl_eigen_x[0]
                    nmodl_eigen_j[0] = -1.0
                }{
                    x = nmodl_eigen_x[0]
                }{
                }
            })";

        THEN("return F & J for newton solver") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::NON_LINEAR_BLOCK);
            compare_blocks(reindent_text(result[0]), reindent_text(expected_text));
        }
    }
    GIVEN("array state-var numeric NONLINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                s[3]
            }
            NONLINEAR nonlin {
                ~ s[0] = 1
                ~ s[1] = 3
                ~ s[2] + s[1] = s[0]
            })";
        std::string expected_text = R"(
            NONLINEAR nonlin {
                EIGEN_NEWTON_SOLVE[3]{
                }{
                }{
                    nmodl_eigen_x[0] = s[0]
                    nmodl_eigen_x[1] = s[1]
                    nmodl_eigen_x[2] = s[2]
                }{
                    nmodl_eigen_f[0] = 1.0-nmodl_eigen_x[0]
                    nmodl_eigen_f[1] = 3.0-nmodl_eigen_x[1]
                    nmodl_eigen_f[2] = nmodl_eigen_x[0]-nmodl_eigen_x[1]-nmodl_eigen_x[2]
                    nmodl_eigen_j[0] = -1.0
                    nmodl_eigen_j[3] = 0
                    nmodl_eigen_j[6] = 0
                    nmodl_eigen_j[1] = 0
                    nmodl_eigen_j[4] = -1.0
                    nmodl_eigen_j[7] = 0
                    nmodl_eigen_j[2] = 1.0
                    nmodl_eigen_j[5] = -1.0
                    nmodl_eigen_j[8] = -1.0
                }{
                    s[0] = nmodl_eigen_x[0]
                    s[1] = nmodl_eigen_x[1]
                    s[2] = nmodl_eigen_x[2]
                }{
                }
            })";
        THEN("return F & J for newton solver") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::NON_LINEAR_BLOCK);
            compare_blocks(reindent_text(result[0]), reindent_text(expected_text));
        }
    }
}
SCENARIO("Solve KINETIC block using SympySolver Visitor", "[visitor][solver][sympy][kinetic]") {
    GIVEN("KINETIC block with not inlined function should work") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE kstates METHOD sparse
            }
            STATE {
                C1
                C2
            }
            FUNCTION alfa(v(mV)) {
                alfa = v
            }
            KINETIC kstates {
                ~ C1 <-> C2 (alfa(v), alfa(v))
            })";
        std::string expected_text = R"(
            DERIVATIVE kstates {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL kf0_, kb0_, old_C1, old_C2
                }{
                    kb0_ = alfa(v)
                    kf0_ = alfa(v)
                    old_C1 = C1
                    old_C2 = C2
                }{
                    nmodl_eigen_x[0] = C1
                    nmodl_eigen_x[1] = C2
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*kf0_+nmodl_eigen_x[1]*kb0_)+old_C1)/dt
                    nmodl_eigen_j[0] = -kf0_-1/dt
                    nmodl_eigen_j[2] = kb0_
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]*kf0_-nmodl_eigen_x[1]*kb0_)+old_C2)/dt
                    nmodl_eigen_j[1] = kf0_
                    nmodl_eigen_j[3] = -kb0_-1/dt
                }{
                    C1 = nmodl_eigen_x[0]
                    C2 = nmodl_eigen_x[1]
                }{
                }
            })";
        THEN("Run Kinetic and Sympy Visitor") {
            std::vector<std::string> result;
            REQUIRE_NOTHROW(result = run_sympy_solver_visitor(
                                nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK, true));
            compare_blocks(reindent_text(result[0]), reindent_text(expected_text));
        }
    }
    GIVEN("Protected names in Sympy are respected") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE kstates METHOD sparse
            }
            STATE {
                C1
                C2
            }
            FUNCTION beta(v(mV)) {
                beta = v
            }
            FUNCTION lowergamma(v(mV)) {
                lowergamma = v
            }
            KINETIC kstates {
                ~ C1 <-> C2 (beta(v), lowergamma(v))
            })";
        std::string expected_text = R"(
            DERIVATIVE kstates {
                EIGEN_NEWTON_SOLVE[2]{
                    LOCAL kf0_, kb0_, old_C1, old_C2
                }{
                    kf0_ = beta(v)
                    kb0_ = lowergamma(v)
                    old_C1 = C1
                    old_C2 = C2
                }{
                    nmodl_eigen_x[0] = C1
                    nmodl_eigen_x[1] = C2
                }{
                    nmodl_eigen_f[0] = (-nmodl_eigen_x[0]+dt*(-nmodl_eigen_x[0]*kf0_+nmodl_eigen_x[1]*kb0_)+old_C1)/dt
                    nmodl_eigen_j[0] = -kf0_-1/dt
                    nmodl_eigen_j[2] = kb0_
                    nmodl_eigen_f[1] = (-nmodl_eigen_x[1]+dt*(nmodl_eigen_x[0]*kf0_-nmodl_eigen_x[1]*kb0_)+old_C2)/dt
                    nmodl_eigen_j[1] = kf0_
                    nmodl_eigen_j[3] = -kb0_-1/dt
                }{
                    C1 = nmodl_eigen_x[0]
                    C2 = nmodl_eigen_x[1]
                }{
                }
            })";
        THEN("Run Kinetic and Sympy Visitor") {
            std::vector<std::string> result;
            REQUIRE_NOTHROW(result = run_sympy_solver_visitor(
                                nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK, true));
            compare_blocks(reindent_text(result[0]), reindent_text(expected_text));
        }
    }
}
