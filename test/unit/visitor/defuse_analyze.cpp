/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

#include "ast/binary_expression.hpp"
#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/checkparent_visitor.hpp"
#include "visitors/defuse_analyze_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;

//=============================================================================
// DefUseAnalyze visitor tests
//=============================================================================

std::vector<DUChain> run_defuse_visitor(const std::string& text, const std::string& variable) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);
    InlineVisitor().visit_program(*ast);

    std::vector<DUChain> chains;
    DefUseAnalyzeVisitor v(*ast->get_symbol_table());

    /// analyse only derivative blocks in this test
    const auto& blocks = nmodl::collect_nodes(*ast, {AstNodeType::DERIVATIVE_BLOCK});
    chains.reserve(blocks.size());
    for (const auto& block: blocks) {
        chains.push_back(v.analyze(*block, variable));
    }

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return chains;
}

SCENARIO("Perform DefUse analysis on NMODL constructs") {
    GIVEN("global variable usage in assignment statements") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            DERIVATIVE states {
                tau = 1
                tau = 1 + tau
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"name":"D"},{"name":"U"},{"name":"D"}]})";

        THEN("Def-Use chains for individual usage is printed") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");

            REQUIRE(chains[0].to_string(true) == expected_text);
            REQUIRE(chains[0].eval() == DUState::D);

            REQUIRE(to_nmodl(*chains[0].chain[0].binary_expression) == "tau = 1");
            REQUIRE(to_nmodl(*chains[0].chain[1].binary_expression) == "tau = 1+tau");
            REQUIRE(to_nmodl(*chains[0].chain[2].binary_expression) == "tau = 1+tau");
        }
    }

    GIVEN("block with use of verbatim block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            DERIVATIVE states {
                VERBATIM ENDVERBATIM
                tau = 1
                VERBATIM ENDVERBATIM
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"name":"U"},{"name":"D"},{"name":"U"}]})";

        THEN("Verbatim block is considered as use of the variable") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string(true) == expected_text);
            REQUIRE(chains[0].eval() == DUState::U);

            REQUIRE(chains[0].chain[0].binary_expression == nullptr);  // verbatim begin
            REQUIRE(to_nmodl(*chains[0].chain[1].binary_expression) == "tau = 1");
            REQUIRE(chains[0].chain[2].binary_expression == nullptr);  // verbatim end
        }
    }

    GIVEN("use of array variables") {
        std::string nmodl_text = R"(
            DEFINE N 3
            STATE {
                m[N]
                h[N]
                n[N]
                o[N]
            }
            DERIVATIVE states {
                LOCAL tau[N]
                tau[0] = 1                    : tau[0] is defined
                tau[2] = 1 + tau[1] + tau[2]  : tau[1] is used; tau[2] is defined as well as used
                m[0] = m[1]                   : m[0] is defined and used on next line; m[1] is used
                h[1] = m[0] + h[0]            : h[0] is used; h[1] is defined
                o[i] = 1                      : o[i] is defined for any i
                n[i+1] = 1 + n[i]             : n[i] is used as well as defined for any i
            }
        )";

        THEN("Def-Use analyser distinguishes variables by array index") {
            std::string input = reindent_text(nmodl_text);
            {
                auto m0 = run_defuse_visitor(input, "m[0]");
                auto m1 = run_defuse_visitor(input, "m[1]");
                auto h1 = run_defuse_visitor(input, "h[1]");
                auto tau0 = run_defuse_visitor(input, "tau[0]");
                auto tau1 = run_defuse_visitor(input, "tau[1]");
                auto tau2 = run_defuse_visitor(input, "tau[2]");
                auto n0 = run_defuse_visitor(input, "n[0]");
                auto n1 = run_defuse_visitor(input, "n[1]");
                auto o0 = run_defuse_visitor(input, "o[0]");

                REQUIRE(m0[0].to_string() == R"({"DerivativeBlock":[{"name":"D"},{"name":"U"}]})");
                REQUIRE(to_nmodl(*m0[0].chain[0].binary_expression) == "m[0] = m[1]");
                REQUIRE(to_nmodl(*m0[0].chain[1].binary_expression) == "h[1] = m[0]+h[0]");
                REQUIRE(m1[0].to_string() == R"({"DerivativeBlock":[{"name":"U"}]})");
                REQUIRE(to_nmodl(*m1[0].chain[0].binary_expression) == "m[0] = m[1]");
                REQUIRE(h1[0].to_string() == R"({"DerivativeBlock":[{"name":"D"}]})");
                REQUIRE(to_nmodl(*h1[0].chain[0].binary_expression) == "h[1] = m[0]+h[0]");
                REQUIRE(tau0[0].to_string() == R"({"DerivativeBlock":[{"name":"LD"}]})");
                REQUIRE(to_nmodl(*tau0[0].chain[0].binary_expression) == "tau[0] = 1");
                REQUIRE(tau1[0].to_string() == R"({"DerivativeBlock":[{"name":"LU"}]})");
                REQUIRE(to_nmodl(*tau1[0].chain[0].binary_expression) ==
                        "tau[2] = 1+tau[1]+tau[2]");
                REQUIRE(tau2[0].to_string() ==
                        R"({"DerivativeBlock":[{"name":"LU"},{"name":"LD"}]})");
                REQUIRE(to_nmodl(*tau2[0].chain[0].binary_expression) ==
                        "tau[2] = 1+tau[1]+tau[2]");
                REQUIRE(to_nmodl(*tau2[0].chain[1].binary_expression) ==
                        "tau[2] = 1+tau[1]+tau[2]");
                REQUIRE(n0[0].to_string() == R"({"DerivativeBlock":[{"name":"U"},{"name":"D"}]})");
                REQUIRE(to_nmodl(*n0[0].chain[0].binary_expression) == "n[i+1] = 1+n[i]");
                REQUIRE(to_nmodl(*n0[0].chain[1].binary_expression) == "n[i+1] = 1+n[i]");
                REQUIRE(n1[0].to_string() == R"({"DerivativeBlock":[{"name":"U"},{"name":"D"}]})");
                REQUIRE(to_nmodl(*n1[0].chain[0].binary_expression) == "n[i+1] = 1+n[i]");
                REQUIRE(to_nmodl(*n1[0].chain[1].binary_expression) == "n[i+1] = 1+n[i]");
                REQUIRE(o0[0].to_string() == R"({"DerivativeBlock":[{"name":"D"}]})");
                REQUIRE(to_nmodl(*o0[0].chain[0].binary_expression) == "o[i] = 1");
            }
        }
    }

    GIVEN("global variable used in if statement (lhs)") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                IF (tau == 0) {
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"U"}]}]}]})";

        THEN("tau is used") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::U);
            REQUIRE(chains[0].chain[0].binary_expression == nullptr);
            REQUIRE(chains[0].chain[0].children[0].binary_expression == nullptr);
            REQUIRE(to_nmodl(*chains[0].chain[0].children[0].children[0].binary_expression) ==
                    "tau == 0");
        }
    }

    GIVEN("global variable used in if statement (rhs)") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                IF (0 == tau) {
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"U"}]}]}]})";

        THEN("tau is used") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::U);
            REQUIRE(chains[0].chain[0].binary_expression == nullptr);
            REQUIRE(chains[0].chain[0].children[0].binary_expression == nullptr);
            REQUIRE(to_nmodl(*chains[0].chain[0].children[0].children[0].binary_expression) ==
                    "0 == tau");
        }
    }

    GIVEN("global variable definition in else block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            DERIVATIVE states {
                IF (1) {
                    LOCAL tau
                    tau = 1
                } ELSE {
                    tau = 1
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"LD"}]},{"ELSE":[{"name":"D"}]}]}]})";

        THEN("Def-Use chains should return NONE") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::CD);
        }
    }

    GIVEN("global variable use in if statement + definition and use in if and else blocks") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                IF (tau == 0) {
                    tau = 1 + tau
                } ELSE {
                    tau = 2 + tau
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"U"},{"name":"U"},{"name":"D"}]},{"ELSE":[{"name":"U"},{"name":"D"}]}]}]})";

        THEN("tau is used and then used in its definitions") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].chain[0].binary_expression == nullptr);  // CONDITIONAL_BLOCK
            REQUIRE(chains[0].chain[0].children[0].binary_expression == nullptr);  // IF
            REQUIRE(to_nmodl(*chains[0].chain[0].children[0].children[0].binary_expression) ==
                    "tau == 0");
            REQUIRE(to_nmodl(*chains[0].chain[0].children[0].children[1].binary_expression) ==
                    "tau = 1+tau");
            REQUIRE(to_nmodl(*chains[0].chain[0].children[0].children[2].binary_expression) ==
                    "tau = 1+tau");
            REQUIRE(chains[0].chain[0].children[1].binary_expression == nullptr);  // ELSE
            REQUIRE(to_nmodl(*chains[0].chain[0].children[1].children[0].binary_expression) ==
                    "tau = 2+tau");
            REQUIRE(to_nmodl(*chains[0].chain[0].children[1].children[1].binary_expression) ==
                    "tau = 2+tau");
        }
    }

    GIVEN("global variable usage in else block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            DERIVATIVE states {
                IF (1) {
                } ELSE {
                    tau = 1 + tau
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"name":"IF"},{"ELSE":[{"name":"U"},{"name":"D"}]}]}]})";

        THEN("Def-Use chains should return USE") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::U);
        }
    }

    GIVEN("global variable definition in if-else block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                IF (1) {
                    tau = 11.1
                    exp(tau)
                } ELSE {
                    tau = 1
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"D"},{"name":"U"}]},{"ELSE":[{"name":"D"}]}]}]})";

        THEN("Def-Use chains should return DEF") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::D);
        }
    }

    GIVEN("conditional definition in nested block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            DERIVATIVE states {
                IF (1) {
                    IF(11) {
                        tau = 11.1
                        exp(tau)
                    }
                } ELSE IF(1) {
                    tau = 1
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"D"},{"name":"U"}]}]}]},{"ELSEIF":[{"name":"D"}]}]}]})";

        THEN("Def-Use chains should return DEF") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::CD);
        }
    }

    GIVEN("global variable usage in if-elseif-else block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            DERIVATIVE states {
                IF (1) {
                    tau = 1
                }
                tau = 1 + tau
                IF (0) {
                    beta = 1
                } ELSE IF (2) {
                    tau = 1
                }
            }

        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"D"}]}]},{"name":"U"},{"name":"D"},{"CONDITIONAL_BLOCK":[{"name":"IF"},{"ELSEIF":[{"name":"D"}]}]}]})";

        THEN("Def-Use chains for individual usage is printed") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::U);
        }
    }

    GIVEN("global variable used in nested if-elseif-else block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            DERIVATIVE states {
                IF (1) {
                    LOCAL tau
                    tau = 1
                }
                IF (0) {
                    IF (1) {
                        beta = 1
                    } ELSE {
                        tau = 1
                    }
                } ELSE IF (2) {
                    IF (1) {
                        beta = 1
                        IF (0) {
                        } ELSE {
                            beta = 1 + exp(tau)
                        }
                    }
                    tau = 1
                }
            }
        )";

        std::string expected_text =
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"name":"LD"}]}]},{"CONDITIONAL_BLOCK":[{"IF":[{"CONDITIONAL_BLOCK":[{"name":"IF"},{"ELSE":[{"name":"D"}]}]}]},{"ELSEIF":[{"CONDITIONAL_BLOCK":[{"IF":[{"CONDITIONAL_BLOCK":[{"name":"IF"},{"ELSE":[{"name":"U"}]}]}]}]},{"name":"D"}]}]}]})";

        THEN("Def-Use chains for nested statements calculated") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::U);
        }
    }
}
