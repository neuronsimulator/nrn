/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_MAIN

#include <string>
#include <utility>

#include "catch/catch.hpp"
#include "lexer/modtoken.hpp"
#include "parser/diffeq_driver.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/utils/nmodl_constructs.hpp"
#include "test/utils/test_utils.hpp"
#include "visitors/lookup_visitor.hpp"

using namespace nmodl::test_utils;
using nmodl::visitor::AstLookupVisitor;
//=============================================================================
// Parser tests
//=============================================================================

bool is_valid_construct(const std::string& construct) {
    nmodl::parser::NmodlDriver driver;
    return driver.parse_string(construct) != nullptr;
}


SCENARIO("NMODL can define macros using DEFINE keyword", "[parser]") {
    GIVEN("A valid macro definition") {
        WHEN("DEFINE NSTEP 6") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("DEFINE NSTEP 6"));
            }
        }
        WHEN("DEFINE   NSTEP   6") {
            THEN("parser accepts without an error") {
                REQUIRE(is_valid_construct("DEFINE   NSTEP   6"));
            }
        }
    }

    GIVEN("A macro with nested definition is not supported") {
        WHEN("DEFINE SIX 6 DEFINE NSTEP SIX") {
            THEN("parser throws an error") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE SIX 6 DEFINE NSTEP SIX"),
                                    Catch::Contains("unexpected INVALID_TOKEN"));
            }
        }
    }

    GIVEN("A invalid macro definition with float value") {
        WHEN("DEFINE NSTEP 6.0") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE NSTEP 6.0"),
                                    Catch::Contains("unexpected REAL"));
            }
        }
    }

    GIVEN("A invalid macro definition with name and without value") {
        WHEN("DEFINE NSTEP") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE NSTEP"),
                                    Catch::Contains("expecting INTEGER"));
            }
        }
    }

    GIVEN("A invalid macro definition with name and value as a name") {
        WHEN("DEFINE NSTEP SIX") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE NSTEP SIX"),
                                    Catch::Contains("expecting INTEGER"));
            }
        }
    }

    GIVEN("A invalid macro definition without name but with value") {
        WHEN("DEFINE 6") {
            THEN("parser throws an exception") {
                REQUIRE_THROWS_WITH(is_valid_construct("DEFINE 6"),
                                    Catch::Contains("expecting NAME"));
            }
        }
    }
}

SCENARIO("Macros can be used anywhere in the mod file") {
    std::string nmodl_text = R"(
            DEFINE NSTEP 6
            PARAMETER {
                amp[NSTEP] (mV)
            }
        )";
    WHEN("macro is used in parameter definition") {
        THEN("parser accepts without an error") {
            REQUIRE(is_valid_construct(nmodl_text));
        }
    }
}

SCENARIO("NMODL parser accepts empty unit specification") {
    std::string nmodl_text = R"(
            FUNCTION ssCB(kdf(), kds()) (mM) {

            }
        )";
    WHEN("FUNCTION is defined with empty unit") {
        THEN("parser accepts without an error") {
            REQUIRE(is_valid_construct(nmodl_text));
        }
    }
}

SCENARIO("NMODL parser running number of valid NMODL constructs") {
    for (const auto& construct: nmodl_valid_constructs) {
        auto test_case = construct.second;
        GIVEN(test_case.name) {
            THEN("Parser successfully parses : " + test_case.input) {
                REQUIRE(is_valid_construct(test_case.input));
            }
        }
    }
}

SCENARIO("NMODL parser running number of invalid NMODL constructs") {
    for (const auto& construct: nmdol_invalid_constructs) {
        auto test_case = construct.second;
        GIVEN(test_case.name) {
            THEN("Parser throws an exception while parsing : " + test_case.input) {
                REQUIRE_THROWS(is_valid_construct(test_case.input));
            }
        }
    }
}


//=============================================================================
// Differential Equation Parser tests
//=============================================================================

std::string solve_construct(const std::string& equation, std::string method) {
    nmodl::parser::DiffeqDriver driver;
    auto solution = driver.solve(equation, std::move(method));
    return solution;
}

SCENARIO("Legacy differential equation solver from NEURON solve number of ODE types") {
    GIVEN("A differential equation") {
        int counter = 0;
        for (const auto& test_case: diff_eq_constructs) {
            auto prefix = "." + std::to_string(counter);
            WHEN(prefix + " EQUATION = " + test_case.equation + " METHOD = " + test_case.method) {
                THEN(prefix + " SOLUTION = " + test_case.solution) {
                    auto expected_result = test_case.solution;
                    auto result = solve_construct(test_case.equation, test_case.method);
                    REQUIRE(result == expected_result);
                }
            }
            counter++;
        }
    }
}


//=============================================================================
// Check if parsed NEURON block has correct token information
//=============================================================================

void parse_neuron_block_string(const std::string& name, nmodl::ModToken& value) {
    nmodl::parser::NmodlDriver driver;
    driver.parse_string(name);

    auto ast_program = driver.get_ast();
    std::vector<std::shared_ptr<nmodl::ast::Ast>> neuron_blocks =
        AstLookupVisitor().lookup(ast_program->get_shared_ptr().get(),
                                  nmodl::ast::AstNodeType::NEURON_BLOCK);
    value = *(neuron_blocks[0]->get_token());
}

SCENARIO("Check if a NEURON block is parsed with correct location info in its token") {
    GIVEN("A single NEURON block") {
        nmodl::ModToken value;

        std::stringstream ss;
        std::string neuron_block = R"(
        NEURON {
            SUFFIX it
            USEION ca READ cai,cao WRITE ica
            RANGE gcabar, m_inf, tau_m, alph1, alph2, KK, shift
        }
        )";
        parse_neuron_block_string(reindent_text(neuron_block), value);
        ss << value;
        REQUIRE(ss.str() == "         NEURON at [1.1-5.1] type 303");
    }
}
