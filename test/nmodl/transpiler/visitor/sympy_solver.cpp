/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "catch/catch.hpp"

#include "parser/nmodl_driver.hpp"
#include "test/utils/test_utils.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/loop_unroll_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;


//=============================================================================
// SympySolver visitor tests
//=============================================================================

std::vector<std::string> run_sympy_solver_visitor(
    const std::string& text,
    bool pade = false,
    bool cse = false,
    AstNodeType ret_nodetype = AstNodeType::DIFF_EQ_EXPRESSION) {
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

    // run SympySolver on AST
    SympySolverVisitor(pade, cse).visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().visit_program(*ast);

    // run lookup visitor to extract results from AST
    AstLookupVisitor v_lookup;
    auto res = v_lookup.lookup(*ast, ret_nodetype);
    for (const auto& r: res) {
        results.push_back(to_nmodl(r));
    }

    return results;
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
            REQUIRE(result[1] == "h = -h/(c2*dt*h-1)");
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
            REQUIRE(result[1] == "X[1] = -X[1]/(c2*dt*X[1]-1)");
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
            REQUIRE(result[0] == "z' = a/z+b/z/z");
            REQUIRE(result[1] == "h = -h/(c2*dt*h-1)");
            REQUIRE(result[2] == "x = a*dt+x");
            /// sympy 1.4 able to solve ode but not older versions
            bool last_result = (result[3] == "y' = c3*y*y*y" ||
                                result[3] == "y = sqrt(-pow(y, 2)/(2*c3*dt*pow(y, 2)-1))");
            REQUIRE(last_result);
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
                    X[0] = m
                }{
                    F[0] = (-X[0]*dt+dt*mInf+mTau*(-X[0]+old_m))/mTau
                    J[0] = -(dt+mTau)/mTau
                }{
                    m = X[0]
                }{
                }
            })";
        THEN("SympySolver correctly inserts ode to block") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                LOCAL a, b, c, d, h, old_x, old_y, old_z
                old_x = x
                old_y = y
                old_z = z
                x = (-a*dt*(dt*(c*dt+2*dt*(b*dt*h+old_x)+old_y)-old_z)+(b*dt*h+old_x)*(2*a*pow(dt, 3)-d*dt+1))/(2*a*pow(dt, 3)-d*dt+1)
                y = (-2*a*pow(dt, 2)*(dt*(c*dt+2*dt*(b*dt*h+old_x)+old_y)-old_z)+(2*a*pow(dt, 3)-d*dt+1)*(c*dt+2*dt*(b*dt*h+old_x)+old_y))/(2*a*pow(dt, 3)-d*dt+1)
                z = (-dt*(c*dt+2*dt*(b*dt*h+old_x)+old_y)+old_z)/(2*a*pow(dt, 3)-d*dt+1)
            })";
        std::string expected_cse_result = R"(
            DERIVATIVE states {
                LOCAL a, b, c, d, h, old_x, old_y, old_z, tmp0, tmp1, tmp2, tmp3, tmp4, tmp5
                old_x = x
                old_y = y
                old_z = z
                tmp0 = 2*a
                tmp1 = -d*dt+pow(dt, 3)*tmp0+1
                tmp2 = 1/tmp1
                tmp3 = b*dt*h+old_x
                tmp4 = c*dt+2*dt*tmp3+old_y
                tmp5 = dt*tmp4-old_z
                x = -tmp2*(a*dt*tmp5-tmp1*tmp3)
                y = -tmp2*(pow(dt, 2)*tmp0*tmp5-tmp1*tmp4)
                z = -tmp2*tmp5
            })";

        THEN("Construct & solve linear system for backwards Euler") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            auto result_cse =
                run_sympy_solver_visitor(nmodl_text, true, true, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
            REQUIRE(result_cse[0] == reindent_text(expected_cse_result));
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
                LOCAL old_mc, old_m
                old_mc = mc
                old_m = m
                mc = (b*dt*old_m+b*dt*old_mc+old_mc)/(a*dt+b*dt+1)
                m = (a*dt*old_m+a*dt*old_mc+old_m)/(a*dt+b*dt+1)
            })";
        THEN("Construct & solve linear system") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                LOCAL old_mc
                old_mc = mc
                mc = (b*dt+old_mc)/(a*dt+b*dt+1)
                m = (a*dt-old_mc+1)/(a*dt+b*dt+1)
            })";
        THEN("Construct & solve linear system, replace ODE for m with rhs of CONSERVE statement") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                LOCAL old_mc, old_m
                old_mc = mc
                old_m = m
                mc = (b*dt*old_m+b*dt*old_mc+old_mc)/(a*dt+b*dt+1)
                m = (a*dt*old_m+a*dt*old_mc+old_m)/(a*dt+b*dt+1)
            })";
        THEN("Construct & solve linear system, ignore invalid CONSERVE statement") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                EIGEN_LINEAR_SOLVE[5]{
                    LOCAL alpha, beta, k3p, k4, k1ca, k2, old_c1, old_o1, old_p0
                }{
                    evaluate_fct(v, cai)
                    old_c1 = c1
                    old_o1 = o1
                    old_p0 = p0
                }{
                    X[0] = c1
                    X[1] = o1
                    X[2] = o2
                    X[3] = p0
                    X[4] = p1
                    F[0] = -old_c1
                    F[1] = -old_o1
                    F[2] = -1
                    F[3] = -old_p0
                    F[4] = -1
                    J[0] = -alpha*dt-1
                    J[5] = beta*dt
                    J[10] = 0
                    J[15] = 0
                    J[20] = 0
                    J[1] = alpha*dt
                    J[6] = -beta*dt-dt*k3p-1
                    J[11] = dt*k4
                    J[16] = 0
                    J[21] = 0
                    J[2] = -1
                    J[7] = -1
                    J[12] = -1
                    J[17] = 0
                    J[22] = 0
                    J[3] = 0
                    J[8] = 0
                    J[13] = 0
                    J[18] = -dt*k1ca-1
                    J[23] = dt*k2
                    J[4] = 0
                    J[9] = 0
                    J[14] = 0
                    J[19] = -1
                    J[24] = -1
                }{
                    c1 = X[0]
                    o1 = X[1]
                    o2 = X[2]
                    p0 = X[3]
                    p1 = X[4]
                }{
                }
            })";
        THEN(
            "Construct & solve linear system, replacing ODEs for p1 and o2 with CONSERVE statement "
            "algebraic relations") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                LOCAL old_W_0
                old_W_0 = W[0]
                W[0] = (3*dt*A[1]+old_W_0)/(dt*A[0]-dt*B[0]+1)
            })";
        THEN("Construct & solver linear system") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                LOCAL old_M_0, old_M_1
                old_M_0 = M[0]
                old_M_1 = M[1]
                M[0] = (dt*old_M_0*B[1]+dt*old_M_1*B[0]+old_M_0)/(pow(dt, 2)*A[0]*B[1]-pow(dt, 2)*A[1]*B[0]+dt*A[0]+dt*B[1]+1)
                M[1] = -(dt*old_M_0*A[1]+old_M_1*(dt*A[0]+1))/(pow(dt, 2)*A[1]*B[0]-(dt*A[0]+1)*(dt*B[1]+1))
            })";
        THEN("Construct & solver linear system") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                    X[0] = W[0]
                }{
                    F[0] = -X[0]*dt*A[0]+X[0]*dt*B[0]-X[0]+3*dt*A[1]+old_W_0
                    J[0] = -dt*A[0]+dt*B[0]-1
                }{
                    W[0] = X[0]
                }{
                }
            })";
        THEN("Construct newton solve block") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                    X[0] = m
                    X[1] = h
                    X[2] = n
                }{
                    F[0] = (-X[0]*dt+dt*minf+mtau*(-X[0]-3*X[1]*dt+old_m))/mtau
                    F[1] = (-X[1]*dt+dt*hinf+htau*(pow(X[0], 2)*dt-X[1]+old_h))/htau
                    F[2] = (-X[2]*dt+dt*ninf+ntau*(-X[2]+old_n))/ntau
                    J[0] = -(dt+mtau)/mtau
                    J[3] = -3*dt
                    J[6] = 0
                    J[1] = 2*X[0]*dt
                    J[4] = -(dt+htau)/htau
                    J[7] = 0
                    J[2] = 0
                    J[5] = 0
                    J[8] = -(dt+ntau)/ntau
                }{
                    m = X[0]
                    h = X[1]
                    n = X[2]
                }{
                }
            })";
        THEN("Construct newton solve block") {
            CAPTURE(nmodl_text);
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            REQUIRE(result[0] == reindent_text(expected_result));
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
                    X[0] = m
                    X[1] = h
                }{
                    F[0] = (-X[0]*dt+dt*minf+mtau*(-X[0]+old_m))/mtau
                    F[1] = (-X[1]*dt+dt*hinf+htau*(pow(X[0], 2)*dt-X[1]+old_h))/htau
                    J[0] = -(dt+mtau)/mtau
                    J[2] = 0
                    J[1] = 2*X[0]*dt
                    J[3] = -(dt+htau)/htau
                }{
                    m = X[0]
                    h = X[1]
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
                    X[0] = m
                    X[1] = h
                }{
                    F[0] = (-X[1]*dt+dt*hinf+htau*(pow(X[0], 2)*dt-X[1]+old_h))/htau
                    F[1] = (-X[0]*dt+dt*minf+mtau*(-X[0]+X[1]*dt+old_m))/mtau
                    J[0] = 2*X[0]*dt
                    J[2] = -(dt+htau)/htau
                    J[1] = -(dt+mtau)/mtau
                    J[3] = dt
                }{
                    m = X[0]
                    h = X[1]
                }{
                }
            })";
        THEN("Construct newton solve block") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::DERIVATIVE_BLOCK);
            CAPTURE(nmodl_text);
            REQUIRE(result[0] == reindent_text(expected_result_0));
            REQUIRE(result[1] == reindent_text(expected_result_1));
        }
    }
}


//=============================================================================
// LINEAR solve block tests
//=============================================================================

SCENARIO("LINEAR solve block (SympySolver Visitor)", "[sympy][linear]") {
    GIVEN("1 state-var numeric LINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                x
            }
            LINEAR lin {
                ~ x = 5
            })";
        std::string expected_text = R"(
            LINEAR lin {
                x = 5
            })";
        THEN("solve analytically") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
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
    GIVEN("2 state-var LINEAR solve block, post-solve statements") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            LINEAR lin {
                ~ x + 4*y = 5*a
                ~ x - y = 0
                x = x + 2
                y = y - a
            })";
        std::string expected_text = R"(
            LINEAR lin {
                x = a
                y = a
                x = x+2
                y = y-a
            })";
        THEN("solve analytically, insert in correct location") {
            CAPTURE(reindent_text(nmodl_text));
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
    GIVEN("2 state-var LINEAR solve block, mixed & post-solve statements") {
        std::string nmodl_text = R"(
            STATE {
                x y
            }
            LINEAR lin {
                ~ x + 4*y = 5*a
                a2 = 3*b
                ~ x - y = 0
                y = y - a
            })";
        std::string expected_text = R"(
            LINEAR lin {
                a2 = 3*b
                x = a
                y = a
                y = y-a
            })";
        THEN("solve analytically, insert in correct location") {
            CAPTURE(reindent_text(nmodl_text));
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
    GIVEN("3 state-var LINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                x y z
            }
            LINEAR lin {
                ~ x + 4*c*y = -5.343*a
                ~ a + x/b + z - y = 0.842*b*b
                ~ x + 1.3*y - 0.1*z/(a*a*b) = 1.43543/c
            })";
        std::string expected_text_sympy_13 = R"(
            LINEAR lin {
                x = (4*pow(a, 2)*pow(b, 2)*(-c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(4*c-1.3)+(1*b+4*c)*(5.343*a*c+1.43543))-(5.343*a*(1*b+4*c)-4*c*(5.343*a+b*(-1*a+0.842*pow(b, 2))))*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))/((1*b+4*c)*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
                y = (1*pow(a, 2)*pow(b, 2)*c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(4*c-1.3)-1*pow(a, 2)*pow(b, 2)*(1*b+4*c)*(5.343*a*c+1.43543)-c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))/(c*(1*b+4*c)*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
                z = pow(a, 2)*b*(c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(4*c-1.3)-(1*b+4*c)*(5.343*a*c+1.43543))/(c*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
            })";
        std::string expected_text_sympy_14 = R"(
            LINEAR lin {
                x = (4*pow(a, 2)*pow(b, 2)*(-c*(5.343*a+b*(-a+0.842*pow(b, 2)))*(4*c-1.3)+(b+4*c)*(5.343*a*c+1.43543))-(5.343*a*(b+4*c)-4*c*(5.343*a+b*(-a+0.842*pow(b, 2))))*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))/((b+4*c)*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
                y = (pow(a, 2)*pow(b, 2)*c*(5.343*a+b*(-a+0.842*pow(b, 2)))*(4*c-1.3)-pow(a, 2)*pow(b, 2)*(b+4*c)*(5.343*a*c+1.43543)-c*(5.343*a+b*(-a+0.842*pow(b, 2)))*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))/(c*(b+4*c)*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
                z = pow(a, 2)*b*(c*(5.343*a+b*(-a+0.842*pow(b, 2)))*(4*c-1.3)-(b+4*c)*(5.343*a*c+1.43543))/(c*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
            })";

        THEN("solve analytically") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            bool result_match = (reindent_text(result[0]) ==
                                     reindent_text(expected_text_sympy_13) ||
                                 reindent_text(result[0]) == reindent_text(expected_text_sympy_14));
            REQUIRE(result_match);
        }
    }
    GIVEN("array state-var numeric LINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                s[3]
            }
            LINEAR lin {
                ~ s[0] = 1
                ~ s[1] = 3
                ~ s[2] + s[1] = s[0]
            })";
        std::string expected_text = R"(
            LINEAR lin {
                s[0] = 1
                s[1] = 3
                s[2] = -2
            })";
        THEN("solve analytically") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
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
                    X[0] = w
                    X[1] = x
                    X[2] = y
                    X[3] = z
                    F[0] = 0
                    F[1] = 5.343*a
                    F[2] = a-0.842*pow(b, 2)
                    F[3] = -1.43543/c
                    J[0] = -1
                    J[4] = 0
                    J[8] = -2
                    J[12] = -0.3125
                    J[1] = 0
                    J[5] = -1
                    J[9] = -4*c
                    J[13] = 0
                    J[2] = 0
                    J[6] = -1/b
                    J[10] = 1
                    J[14] = -1
                    J[3] = 0
                    J[7] = -1
                    J[11] = -1.3
                    J[15] = 0.1/(pow(a, 2)*b)
                }{
                    w = X[0]
                    x = X[1]
                    y = X[2]
                    z = X[3]
                }{
                }
            })";
        THEN("return matrix system to solve") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
    GIVEN("12 state-var LINEAR solve block") {
        std::string nmodl_text = R"(
            STATE {
                C1 C2 C3 C4 C5 I1 I2 I3 I4 I5 I6 O
            }
            LINEAR seqinitial {
                ~          I1*bi1 + C2*b01 - C1*(    fi1+f01) = 0
                ~ C1*f01 + I2*bi2 + C3*b02 - C2*(b01+fi2+f02) = 0
                ~ C2*f02 + I3*bi3 + C4*b03 - C3*(b02+fi3+f03) = 0
                ~ C3*f03 + I4*bi4 + C5*b04 - C4*(b03+fi4+f04) = 0
                ~ C4*f04 + I5*bi5 + O*b0O  - C5*(b04+fi5+f0O) = 0
                ~ C5*f0O + I6*bin          - O*(b0O+fin)      = 0
                ~          C1*fi1 + I2*b11 - I1*(    bi1+f11) = 0
                ~ I1*f11 + C2*fi2 + I3*b12 - I2*(b11+bi2+f12) = 0
                ~ I2*f12 + C3*fi3 + I4*bi3 - I3*(b12+bi3+f13) = 0
                ~ I3*f13 + C4*fi4 + I5*b14 - I4*(b13+bi4+f14) = 0
                ~ I4*f14 + C5*fi5 + I6*b1n - I5*(b14+bi5+f1n) = 0
                ~ C1 + C2 + C3 + C4 + C5 + O + I1 + I2 + I3 + I4 + I5 + I6 = 1
            })";
        std::string expected_text = R"(
            LINEAR seqinitial {
                EIGEN_LINEAR_SOLVE[12]{
                }{
                }{
                    X[0] = C1
                    X[1] = C2
                    X[2] = C3
                    X[3] = C4
                    X[4] = C5
                    X[5] = I1
                    X[6] = I2
                    X[7] = I3
                    X[8] = I4
                    X[9] = I5
                    X[10] = I6
                    X[11] = O
                    F[0] = 0
                    F[1] = 0
                    F[2] = 0
                    F[3] = 0
                    F[4] = 0
                    F[5] = 0
                    F[6] = 0
                    F[7] = 0
                    F[8] = 0
                    F[9] = 0
                    F[10] = 0
                    F[11] = -1
                    J[0] = f01+fi1
                    J[12] = -b01
                    J[24] = 0
                    J[36] = 0
                    J[48] = 0
                    J[60] = -bi1
                    J[72] = 0
                    J[84] = 0
                    J[96] = 0
                    J[108] = 0
                    J[120] = 0
                    J[132] = 0
                    J[1] = -f01
                    J[13] = b01+f02+fi2
                    J[25] = -b02
                    J[37] = 0
                    J[49] = 0
                    J[61] = 0
                    J[73] = -bi2
                    J[85] = 0
                    J[97] = 0
                    J[109] = 0
                    J[121] = 0
                    J[133] = 0
                    J[2] = 0
                    J[14] = -f02
                    J[26] = b02+f03+fi3
                    J[38] = -b03
                    J[50] = 0
                    J[62] = 0
                    J[74] = 0
                    J[86] = -bi3
                    J[98] = 0
                    J[110] = 0
                    J[122] = 0
                    J[134] = 0
                    J[3] = 0
                    J[15] = 0
                    J[27] = -f03
                    J[39] = b03+f04+fi4
                    J[51] = -b04
                    J[63] = 0
                    J[75] = 0
                    J[87] = 0
                    J[99] = -bi4
                    J[111] = 0
                    J[123] = 0
                    J[135] = 0
                    J[4] = 0
                    J[16] = 0
                    J[28] = 0
                    J[40] = -f04
                    J[52] = b04+f0O+fi5
                    J[64] = 0
                    J[76] = 0
                    J[88] = 0
                    J[100] = 0
                    J[112] = -bi5
                    J[124] = 0
                    J[136] = -b0O
                    J[5] = 0
                    J[17] = 0
                    J[29] = 0
                    J[41] = 0
                    J[53] = -f0O
                    J[65] = 0
                    J[77] = 0
                    J[89] = 0
                    J[101] = 0
                    J[113] = 0
                    J[125] = -bin
                    J[137] = b0O+fin
                    J[6] = -fi1
                    J[18] = 0
                    J[30] = 0
                    J[42] = 0
                    J[54] = 0
                    J[66] = bi1+f11
                    J[78] = -b11
                    J[90] = 0
                    J[102] = 0
                    J[114] = 0
                    J[126] = 0
                    J[138] = 0
                    J[7] = 0
                    J[19] = -fi2
                    J[31] = 0
                    J[43] = 0
                    J[55] = 0
                    J[67] = -f11
                    J[79] = b11+bi2+f12
                    J[91] = -b12
                    J[103] = 0
                    J[115] = 0
                    J[127] = 0
                    J[139] = 0
                    J[8] = 0
                    J[20] = 0
                    J[32] = -fi3
                    J[44] = 0
                    J[56] = 0
                    J[68] = 0
                    J[80] = -f12
                    J[92] = b12+bi3+f13
                    J[104] = -bi3
                    J[116] = 0
                    J[128] = 0
                    J[140] = 0
                    J[9] = 0
                    J[21] = 0
                    J[33] = 0
                    J[45] = -fi4
                    J[57] = 0
                    J[69] = 0
                    J[81] = 0
                    J[93] = -f13
                    J[105] = b13+bi4+f14
                    J[117] = -b14
                    J[129] = 0
                    J[141] = 0
                    J[10] = 0
                    J[22] = 0
                    J[34] = 0
                    J[46] = 0
                    J[58] = -fi5
                    J[70] = 0
                    J[82] = 0
                    J[94] = 0
                    J[106] = -f14
                    J[118] = b14+bi5+f1n
                    J[130] = -b1n
                    J[142] = 0
                    J[11] = -1
                    J[23] = -1
                    J[35] = -1
                    J[47] = -1
                    J[59] = -1
                    J[71] = -1
                    J[83] = -1
                    J[95] = -1
                    J[107] = -1
                    J[119] = -1
                    J[131] = -1
                    J[143] = -1
                }{
                    C1 = X[0]
                    C2 = X[1]
                    C3 = X[2]
                    C4 = X[3]
                    C5 = X[4]
                    I1 = X[5]
                    I2 = X[6]
                    I3 = X[7]
                    I4 = X[8]
                    I5 = X[9]
                    I6 = X[10]
                    O = X[11]
                }{
                }
            })";
        THEN("return matrix system to be solved") {
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
        std::string expected_text_sympy_13 = R"(
            NONLINEAR nonlin {
                EIGEN_NEWTON_SOLVE[1]{
                }{
                }{
                    X[0] = x
                }{
                    F[0] = -X[0]+5
                    J[0] = -1
                }{
                    x = X[0]
                }{
                }
            })";
        std::string expected_text_sympy_14 = R"(
            NONLINEAR nonlin {
                EIGEN_NEWTON_SOLVE[1]{
                }{
                }{
                    X[0] = x
                }{
                    F[0] = 5-X[0]
                    J[0] = -1
                }{
                    x = X[0]
                }{
                }
            })";

        THEN("return F & J for newton solver") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::NON_LINEAR_BLOCK);
            bool result_match = (reindent_text(result[0]) ==
                                     reindent_text(expected_text_sympy_13) ||
                                 reindent_text(result[0]) == reindent_text(expected_text_sympy_14));
            REQUIRE(result_match);
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
        std::string expected_text_sympy_13 = R"(
            NONLINEAR nonlin {
                EIGEN_NEWTON_SOLVE[3]{
                }{
                }{
                    X[0] = s[0]
                    X[1] = s[1]
                    X[2] = s[2]
                }{
                    F[0] = -X[0]+1
                    F[1] = -X[1]+3
                    F[2] = X[0]-X[1]-X[2]
                    J[0] = -1
                    J[3] = 0
                    J[6] = 0
                    J[1] = 0
                    J[4] = -1
                    J[7] = 0
                    J[2] = 1
                    J[5] = -1
                    J[8] = -1
                }{
                    s[0] = X[0]
                    s[1] = X[1]
                    s[2] = X[2]
                }{
                }
            })";
        std::string expected_text_sympy_14 = R"(
            NONLINEAR nonlin {
                EIGEN_NEWTON_SOLVE[3]{
                }{
                }{
                    X[0] = s[0]
                    X[1] = s[1]
                    X[2] = s[2]
                }{
                    F[0] = 1-X[0]
                    F[1] = 3-X[1]
                    F[2] = X[0]-X[1]-X[2]
                    J[0] = -1
                    J[3] = 0
                    J[6] = 0
                    J[1] = 0
                    J[4] = -1
                    J[7] = 0
                    J[2] = 1
                    J[5] = -1
                    J[8] = -1
                }{
                    s[0] = X[0]
                    s[1] = X[1]
                    s[2] = X[2]
                }{
                }
            })";
        THEN("return F & J for newton solver") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::NON_LINEAR_BLOCK);
            bool result_match = (reindent_text(result[0]) ==
                                     reindent_text(expected_text_sympy_13) ||
                                 reindent_text(result[0]) == reindent_text(expected_text_sympy_14));
            REQUIRE(result_match);
        }
    }
}
