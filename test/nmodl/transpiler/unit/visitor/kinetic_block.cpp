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
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/loop_unroll_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;

//=============================================================================
// KineticBlock visitor tests
//=============================================================================

std::vector<std::string> run_kinetic_block_visitor(const std::string& text) {
    std::vector<std::string> results;

    // construct AST from text including KINETIC block(s)
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor().visit_program(*ast);

    // unroll loops and fold constants
    ConstantFolderVisitor().visit_program(*ast);
    LoopUnrollVisitor().visit_program(*ast);
    ConstantFolderVisitor().visit_program(*ast);
    SymtabVisitor().visit_program(*ast);

    // run KineticBlock visitor on AST
    KineticBlockVisitor().visit_program(*ast);

    // run lookup visitor to extract DERIVATIVE block(s) from AST
    const auto& blocks = collect_nodes(*ast, {AstNodeType::DERIVATIVE_BLOCK});
    results.reserve(blocks.size());
    for (const auto& block: blocks) {
        results.push_back(to_nmodl(block));
    }


    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return results;
}

SCENARIO("Convert KINETIC to DERIVATIVE using KineticBlock visitor", "[kinetic][visitor]") {
    GIVEN("KINETIC block with << reaction statement, 1 state var") {
        static const std::string input_nmodl_text = R"(
            STATE {
                x
            }
            KINETIC states {
                ~ x << (a*c/3.2)
            })";
        static const std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = (a*c/3.2)
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with << reaction statement, 1 array state var") {
        std::string input_nmodl_text = R"(
            STATE {
                x[1]
            }
            KINETIC states {
                ~ x[0] << (a*c/3.2)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x'[0] = (a*c/3.2)
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with << reaction statement, 1 array state var, flux vars") {
        std::string input_nmodl_text = R"(
            STATE {
                x[1]
            }
            KINETIC states {
                ~ x[0] << (a*c/3.2)
                f0 = f_flux*2
                f1 = b_flux + f_flux
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                f0 = 0*2
                f1 = 0+0
                x'[0] = (a*c/3.2)
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with invalid << reaction statement with 2 state vars") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                ~ x + y << (2*z)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
            })";
        THEN("Emit warning & do not process statement") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with -> reaction statement, 1 state var, flux vars") {
        std::string input_nmodl_text = R"(
            STATE {
                x
            }
            KINETIC states {
                ~ x -> (a)
                zf = f_flux
                zb = b_flux
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                zf = a*x
                zb = 0
                x' = (-1*(a*x))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with -> reaction statement, 2 state vars") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                ~ x + y -> (f(v))
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = (-1*(f(v)*x*y))
                y' = (-1*(f(v)*x*y))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with -> reaction statement, 2 state vars, CONSERVE statement") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                ~ x + y -> (f(v))
                CONSERVE x + y = 1
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                CONSERVE y = 1-x
                x' = (-1*(f(v)*x*y))
                y' = (-1*(f(v)*x*y))
            })";
        THEN("Convert to equivalent DERIVATIVE block, rewrite CONSERVE statement") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with -> reaction statement, 2 state vars, CONSERVE & COMPARTMENT") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                COMPARTMENT a { x }
                COMPARTMENT b { y }
                ~ x + y -> (f(v))
                CONSERVE x + y = 1
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                CONSERVE y = (1-(a*1*x))/(b*1)
                x' = ((-1*(f(v)*x*y)))/(a)
                y' = ((-1*(f(v)*x*y)))/(b)
            })";
        THEN(
            "Convert to equivalent DERIVATIVE block, rewrite CONSERVE statement inc COMPARTMENT "
            "factors") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with -> reaction statement, array of 2 state var") {
        std::string input_nmodl_text = R"(
            STATE {
                x[2]
            }
            KINETIC states {
                ~ x[0] + x[1] -> (f(v))
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x'[0] = (-1*(f(v)*x[0]*x[1]))
                x'[1] = (-1*(f(v)*x[0]*x[1]))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with one reaction statement, 1 state var, 1 non-state var, flux vars") {
        // Here c is NOT a state variable
        // see 9.9.2.1 of NEURON book
        // c should be treated as a constant, i.e.
        // -the diff. eq. for x should include the contribution from c
        // -no diff. eq. should be generated for c itself
        std::string input_nmodl_text = R"(
            STATE {
                x
            }
            KINETIC states {
                ~ x <-> c (r, r)
                c1 = f_flux - b_flux
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                c1 = r*x-r*c
                x' = (-1*(r*x-r*c))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with one reaction statement, 2 state vars") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                ~ x <-> y (a, b)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = (-1*(a*x-b*y))
                y' = (1*(a*x-b*y))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with one reaction statement, 2 state vars, CONSERVE statement") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                ~ x <-> y (a, b)
                CONSERVE x + y = 0
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                CONSERVE y = 0-x
                x' = (-1*(a*x-b*y))
                y' = (1*(a*x-b*y))
            })";
        THEN("Convert to equivalent DERIVATIVE block, rewrite CONSERVE statement") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    // array vars in CONSERVE statements are implicit sums over elements
    // see p34 of http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.7812&rep=rep1&type=pdf
    GIVEN("KINETIC block with array state vars, CONSERVE statement") {
        std::string input_nmodl_text = R"(
            STATE {
                x[3] y
            }
            KINETIC states {
                ~ x[0] <-> x[1] (a, b)
                ~ x[2] <-> y (c, d)
                CONSERVE y + x = 1
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                CONSERVE x[2] = 1-y-x[0]-x[1]
                x'[0] = (-1*(a*x[0]-b*x[1]))
                x'[1] = (1*(a*x[0]-b*x[1]))
                x'[2] = (-1*(c*x[2]-d*y))
                y' = (1*(c*x[2]-d*y))
            })";
        THEN(
            "Convert to equivalent DERIVATIVE block, rewrite CONSERVE statement after summing over "
            "array elements, with last state var on LHS") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    // array vars in CONSERVE statements are implicit sums over elements
    // see p34 of http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.42.7812&rep=rep1&type=pdf
    GIVEN("KINETIC block with array state vars, re-ordered CONSERVE statement") {
        std::string input_nmodl_text = R"(
            STATE {
                x[3] y
            }
            KINETIC states {
                ~ x[0] <-> x[1] (a, b)
                ~ x[2] <-> y (c, d)
                CONSERVE x + y = 1
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                CONSERVE y = 1-x[0]-x[1]-x[2]
                x'[0] = (-1*(a*x[0]-b*x[1]))
                x'[1] = (1*(a*x[0]-b*x[1]))
                x'[2] = (-1*(c*x[2]-d*y))
                y' = (1*(c*x[2]-d*y))
            })";
        THEN(
            "Convert to equivalent DERIVATIVE block, rewrite CONSERVE statement after summing over "
            "array elements, with last state var on LHS") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with one reaction statement & 1 COMPARTMENT statement") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                COMPARTMENT c-d {x y}
                ~ x <-> y (a, b)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = ((-1*(a*x-b*y)))/(c-d)
                y' = ((1*(a*x-b*y)))/(c-d)
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with two CONSERVE statements") {
        std::string input_nmodl_text = R"(
            STATE {
                c1 o1 o2 p0 p1
            }
            KINETIC ihkin {
                evaluate_fct(v, cai)
                ~ c1 <-> o1 (alpha, beta)
                ~ p0 <-> p1 (k1ca, k2)
                ~ o1 <-> o2 (k3p, k4)
                CONSERVE p0+p1 = 1
                CONSERVE c1+o1+o2 = 1
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE ihkin {
                evaluate_fct(v, cai)
                CONSERVE p1 = 1-p0
                CONSERVE o2 = 1-c1-o1
                c1' = (-1*(alpha*c1-beta*o1))
                o1' = (1*(alpha*c1-beta*o1))+(-1*(k3p*o1-k4*o2))
                o2' = (1*(k3p*o1-k4*o2))
                p0' = (-1*(k1ca*p0-k2*p1))
                p1' = (1*(k1ca*p0-k2*p1))
            })";
        THEN("Convert to equivalent DERIVATIVE block, re-order both CONSERVE statements") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with one reaction statement & 2 COMPARTMENT statements") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                COMPARTMENT cx {x}
                COMPARTMENT cy {y}
                ~ x <-> y (a, b)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = ((-1*(a*x-b*y)))/(cx)
                y' = ((1*(a*x-b*y)))/(cy)
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with two independent reaction statements") {
        std::string input_nmodl_text = R"(
            STATE {
                w x y z
            }
            KINETIC states {
                ~ x <-> y (a, b)
                ~ w <-> z (c, d)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                w' = (-1*(c*w-d*z))
                x' = (-1*(a*x-b*y))
                y' = (1*(a*x-b*y))
                z' = (1*(c*w-d*z))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with two dependent reaction statements") {
        std::string input_nmodl_text = R"(
            STATE {
                x y z
            }
            KINETIC states {
                ~ x <-> y (a, b)
                ~ y <-> z (c, d)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = (-1*(a*x-b*y))
                y' = (1*(a*x-b*y))+(-1*(c*y-d*z))
                z' = (1*(c*y-d*z))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with two dependent reaction statements, flux vars") {
        std::string input_nmodl_text = R"(
            STATE {
                x y z
            }
            KINETIC states {
                c0 = f_flux
                ~ x <-> y (a, b)
                c1 = f_flux + b_flux
                ~ y <-> z (c, d)
                c2 = f_flux - 2*b_flux
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                c0 = 0
                c1 = a*x+b*y
                c2 = c*y-2*d*z
                x' = (-1*(a*x-b*y))
                y' = (1*(a*x-b*y))+(-1*(c*y-d*z))
                z' = (1*(c*y-d*z))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with a stoch coeff of 2") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                ~ 2x <-> y (a, b)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = (-2*(a*x*x-b*y))
                y' = (1*(a*x*x-b*y))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with duplicate state vars") {
        std::string input_nmodl_text = R"(
            STATE {
                x y
            }
            KINETIC states {
                ~ x + x <-> y (a, b)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                x' = (-2*(a*x*x-b*y))
                y' = (1*(a*x*x-b*y))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with functions for reaction rates") {
        // Example from sec 9.8, p238 of NEURON book
        std::string input_nmodl_text = R"(
            STATE {
                mc m
            }
            KINETIC states {
                ~ mc <-> m (a(v), b(v))
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                mc' = (-1*(a(v)*mc-b(v)*m))
                m' = (1*(a(v)*mc-b(v)*m))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with stoch coeff 2, coupled pair of statements") {
        // Example from sec 9.8, p239 of NEURON book
        std::string input_nmodl_text = R"(
            STATE {
                A B C D
            }
            KINETIC states {
                ~ 2A + B <-> C (k1, k2)
                ~ C + D <-> A + 2B (k3, k4)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                A' = (-2*(k1*A*A*B-k2*C))+(1*(k3*C*D-k4*A*B*B))
                B' = (-1*(k1*A*A*B-k2*C))+(2*(k3*C*D-k4*A*B*B))
                C' = (1*(k1*A*A*B-k2*C))+(-1*(k3*C*D-k4*A*B*B))
                D' = (-1*(k3*C*D-k4*A*B*B))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with loop over array variable") {
        std::string input_nmodl_text = R"(
            DEFINE N 5
            ASSIGNED {
                a
                b[N]
                c[N]
                d
            }
            STATE {
                x[N]
            }
            KINETIC kin {
                ~ x[0] << (a)
                FROM i=0 TO N-2 {
                    ~ x[i] <-> x[i+1] (b[i], c[i])
                }
                ~ x[N-1] -> (d)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE kin {
                {
                }
                x'[0] = (a)+(-1*(b[0]*x[0]-c[0]*x[1]))
                x'[1] = (1*(b[0]*x[0]-c[0]*x[1]))+(-1*(b[1]*x[1]-c[1]*x[2]))
                x'[2] = (1*(b[1]*x[1]-c[1]*x[2]))+(-1*(b[2]*x[2]-c[2]*x[3]))
                x'[3] = (1*(b[2]*x[2]-c[2]*x[3]))+(-1*(b[3]*x[3]-c[3]*x[4]))
                x'[4] = (1*(b[3]*x[3]-c[3]*x[4]))+(-1*(d*x[4]))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
}
