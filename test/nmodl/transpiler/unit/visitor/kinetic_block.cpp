/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/test_utils.hpp"
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

    // run KineticBlock visitor on AST
    KineticBlockVisitor().visit_program(*ast);

    // Since KineticBlockVisitor internally performs const-folding and
    // loop unrolling, we run CheckParentVisitor.
    CheckParentVisitor().check_ast(*ast);

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
                LOCAL source0_
                source0_ = a*c/3.2
                x' = (source0_)
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
                LOCAL source0_
                source0_ = a*c/3.2
                x'[0] = (source0_)
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
                LOCAL source0_
                source0_ = a*c/3.2
                f0 = 0*2
                f1 = 0+0
                x'[0] = (source0_)
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
        THEN("Fail by throwing an error.") {
            CHECK_THROWS(run_kinetic_block_visitor(input_nmodl_text));
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
                LOCAL kf0_
                kf0_ = a
                zf = kf0_*x
                zb = 0
                x' = (-1*(kf0_*x))
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
                LOCAL kf0_
                kf0_ = f(v)
                x' = (-1*(kf0_*x*y))
                y' = (-1*(kf0_*x*y))
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
                LOCAL kf0_
                kf0_ = f(v)
                CONSERVE y = 1-x
                x' = (-1*(kf0_*x*y))
                y' = (-1*(kf0_*x*y))
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
                LOCAL kf0_
                kf0_ = f(v)
                CONSERVE y = (1-(a*1*x))/(b*1)
                x' = ((-1*(kf0_*x*y)))/(a)
                y' = ((-1*(kf0_*x*y)))/(b)
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
                LOCAL kf0_
                kf0_ = f(v)
                x'[0] = (-1*(kf0_*x[0]*x[1]))
                x'[1] = (-1*(kf0_*x[0]*x[1]))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with -> reaction statement, indexed COMPARTMENT") {
        std::string input_nmodl_text = R"(
            STATE {
                x[2]
            }
            KINETIC states {
                COMPARTMENT i, vol[i] { x }
                ~ x[0] + x[1] -> (f(v))
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                LOCAL kf0_
                kf0_ = f(v)
                x'[0] = ((-1*(kf0_*x[0]*x[1])))/(vol[0])
                x'[1] = ((-1*(kf0_*x[0]*x[1])))/(vol[1])
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
                LOCAL kf0_, kb0_
                kf0_ = r
                kb0_ = r
                c1 = kf0_*x-kb0_*c
                x' = (-1*(kf0_*x-kb0_*c))
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
                LOCAL kf0_, kb0_
                kf0_ = a
                kb0_ = b
                x' = (-1*(kf0_*x-kb0_*y))
                y' = (1*(kf0_*x-kb0_*y))
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
                LOCAL kf0_, kb0_
                kf0_ = a
                kb0_ = b
                CONSERVE y = 0-x
                x' = (-1*(kf0_*x-kb0_*y))
                y' = (1*(kf0_*x-kb0_*y))
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
                LOCAL kf0_, kb0_, kf1_, kb1_
                kf0_ = a
                kb0_ = b
                kf1_ = c
                kb1_ = d
                CONSERVE x[2] = 1-y-x[0]-x[1]
                x'[0] = (-1*(kf0_*x[0]-kb0_*x[1]))
                x'[1] = (1*(kf0_*x[0]-kb0_*x[1]))
                x'[2] = (-1*(kf1_*x[2]-kb1_*y))
                y' = (1*(kf1_*x[2]-kb1_*y))
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
                LOCAL kf0_, kb0_, kf1_, kb1_
                kf0_ = a
                kb0_ = b
                kf1_ = c
                kb1_ = d
                CONSERVE y = 1-x[0]-x[1]-x[2]
                x'[0] = (-1*(kf0_*x[0]-kb0_*x[1]))
                x'[1] = (1*(kf0_*x[0]-kb0_*x[1]))
                x'[2] = (-1*(kf1_*x[2]-kb1_*y))
                y' = (1*(kf1_*x[2]-kb1_*y))
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
                LOCAL kf0_, kb0_
                kf0_ = a
                kb0_ = b
                x' = ((-1*(kf0_*x-kb0_*y)))/(c-d)
                y' = ((1*(kf0_*x-kb0_*y)))/(c-d)
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
                LOCAL kf0_, kb0_, kf1_, kb1_, kf2_, kb2_
                evaluate_fct(v, cai)
                kf0_ = alpha
                kb0_ = beta
                kf1_ = k1ca
                kb1_ = k2
                kf2_ = k3p
                kb2_ = k4
                CONSERVE p1 = 1-p0
                CONSERVE o2 = 1-c1-o1
                c1' = (-1*(kf0_*c1-kb0_*o1))
                o1' = (1*(kf0_*c1-kb0_*o1))+(-1*(kf2_*o1-kb2_*o2))
                o2' = (1*(kf2_*o1-kb2_*o2))
                p0' = (-1*(kf1_*p0-kb1_*p1))
                p1' = (1*(kf1_*p0-kb1_*p1))
            })";
        THEN("Convert to equivalent DERIVATIVE block, re-order both CONSERVE statements") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
    GIVEN("KINETIC block with one reaction statement & clashing local") {
        std::string input_nmodl_text = R"(
            ASSIGNED {
                kf0_
            }
            STATE {
                x y
            }
            KINETIC states {
                LOCAL kf0_0000, kb0_0000
                ~ x <-> y (kf0_, b)
            })";
        std::string output_nmodl_text = R"(
            DERIVATIVE states {
                LOCAL kf0_0000, kb0_0000, kf0_0001, kb0_
                kf0_0001 = kf0_
                kb0_ = b
                x' = (-1*(kf0_0001*x-kb0_*y))
                y' = (1*(kf0_0001*x-kb0_*y))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
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
                LOCAL kf0_, kb0_
                kf0_ = a
                kb0_ = b
                x' = ((-1*(kf0_*x-kb0_*y)))/(cx)
                y' = ((1*(kf0_*x-kb0_*y)))/(cy)
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
                LOCAL kf0_, kb0_, kf1_, kb1_
                kf0_ = a
                kb0_ = b
                kf1_ = c
                kb1_ = d
                w' = (-1*(kf1_*w-kb1_*z))
                x' = (-1*(kf0_*x-kb0_*y))
                y' = (1*(kf0_*x-kb0_*y))
                z' = (1*(kf1_*w-kb1_*z))
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
                LOCAL kf0_, kb0_, kf1_, kb1_
                kf0_ = a
                kb0_ = b
                kf1_ = c
                kb1_ = d
                x' = (-1*(kf0_*x-kb0_*y))
                y' = (1*(kf0_*x-kb0_*y))+(-1*(kf1_*y-kb1_*z))
                z' = (1*(kf1_*y-kb1_*z))
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
                LOCAL kf0_, kb0_, kf1_, kb1_
                c0 = 0
                kf0_ = a
                kb0_ = b
                c1 = kf0_*x+kb0_*y
                kf1_ = c
                kb1_ = d
                c2 = kf1_*y-2*kb1_*z
                x' = (-1*(kf0_*x-kb0_*y))
                y' = (1*(kf0_*x-kb0_*y))+(-1*(kf1_*y-kb1_*z))
                z' = (1*(kf1_*y-kb1_*z))
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
                LOCAL kf0_, kb0_
                kf0_ = a
                kb0_ = b
                x' = (-2*(kf0_*x*x-kb0_*y))
                y' = (1*(kf0_*x*x-kb0_*y))
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
                LOCAL kf0_, kb0_
                kf0_ = a
                kb0_ = b
                x' = (-2*(kf0_*x*x-kb0_*y))
                y' = (1*(kf0_*x*x-kb0_*y))
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
                LOCAL kf0_, kb0_
                kf0_ = a(v)
                kb0_ = b(v)
                mc' = (-1*(kf0_*mc-kb0_*m))
                m' = (1*(kf0_*mc-kb0_*m))
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
                LOCAL kf0_, kb0_, kf1_, kb1_
                kf0_ = k1
                kb0_ = k2
                kf1_ = k3
                kb1_ = k4
                A' = (-2*(kf0_*A*A*B-kb0_*C))+(1*(kf1_*C*D-kb1_*A*B*B))
                B' = (-1*(kf0_*A*A*B-kb0_*C))+(2*(kf1_*C*D-kb1_*A*B*B))
                C' = (1*(kf0_*A*A*B-kb0_*C))+(-1*(kf1_*C*D-kb1_*A*B*B))
                D' = (-1*(kf1_*C*D-kb1_*A*B*B))
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
                LOCAL kf0_, kb0_, kf1_, kb1_, kf2_, kb2_, kf3_, kb3_, source4_, kf5_
                source4_ = a
                {
                    kf0_ = b[0]
                    kb0_ = c[0]
                    kf1_ = b[1]
                    kb1_ = c[1]
                    kf2_ = b[2]
                    kb2_ = c[2]
                    kf3_ = b[3]
                    kb3_ = c[3]
                }
                kf5_ = d
                x'[0] = (source4_)+(-1*(kf0_*x[0]-kb0_*x[1]))
                x'[1] = (1*(kf0_*x[0]-kb0_*x[1]))+(-1*(kf1_*x[1]-kb1_*x[2]))
                x'[2] = (1*(kf1_*x[1]-kb1_*x[2]))+(-1*(kf2_*x[2]-kb2_*x[3]))
                x'[3] = (1*(kf2_*x[2]-kb2_*x[3]))+(-1*(kf3_*x[3]-kb3_*x[4]))
                x'[4] = (1*(kf3_*x[3]-kb3_*x[4]))+(-1*(kf5_*x[4]))
            })";
        THEN("Convert to equivalent DERIVATIVE block") {
            auto result = run_kinetic_block_visitor(input_nmodl_text);
            CAPTURE(input_nmodl_text);
            REQUIRE(result[0] == reindent_text(output_nmodl_text));
        }
    }
}
