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
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using nmodl::parser::NmodlDriver;


//=============================================================================
// Variable rename tests
//=============================================================================

static std::string run_var_rename_visitor(
    const std::string& text,
    const std::vector<std::pair<std::string, std::string>>& variables) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);
    for (const auto& variable: variables) {
        RenameVisitor(variable.first, variable.second).visit_program(*ast);
    }
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    return stream.str();
}

SCENARIO("Renaming any variable in mod file with RenameVisitor", "[visitor][rename]") {
    GIVEN("A mod file") {
        // sample nmodl text
        std::string input_nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar
            }

            PARAMETER {
                gNaTs2_tbar = 0.1 (S/cm2)
            }

            STATE {
                m
                h
            }

            COMMENT
                m and gNaTs2_tbar remain same here
            ENDCOMMENT

            BREAKPOINT {
                LOCAL gNaTs2_t
                gNaTs2_t = gNaTs2_tbar*m*m*m*h
                ina = gNaTs2_t*(v-ena)
            }

            FUNCTION mAlpha() {
            }
        )";

        /// expected result after renaming m, gNaTs2_tbar and mAlpha
        std::string output_nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE new_gNaTs2_tbar
            }

            PARAMETER {
                new_gNaTs2_tbar = 0.1 (S/cm2)
            }

            STATE {
                mm
                h
            }

            COMMENT
                m and gNaTs2_tbar remain same here
            ENDCOMMENT

            BREAKPOINT {
                LOCAL gNaTs2_t
                gNaTs2_t = new_gNaTs2_tbar*mm*mm*mm*h
                ina = gNaTs2_t*(v-ena)
            }

            FUNCTION mBeta() {
            }
        )";

        std::string input = reindent_text(input_nmodl_text);
        std::string expected_output = reindent_text(output_nmodl_text);

        THEN("existing variables could be renamed") {
            std::vector<std::pair<std::string, std::string>> variables = {
                {"m", "mm"},
                {"gNaTs2_tbar", "new_gNaTs2_tbar"},
                {"mAlpha", "mBeta"},
            };
            auto result = run_var_rename_visitor(input, variables);
            REQUIRE(result == expected_output);
        }

        THEN("non-existing variables will be ignored") {
            std::vector<std::pair<std::string, std::string>> variables = {
                {"unknown_variable", "doesnot_matter"}};
            auto result = run_var_rename_visitor(input, variables);
            REQUIRE(result == input);
        }
    }
}

//=============================================================================
// Local variable rename tests
//=============================================================================

std::string run_local_var_rename_visitor(const std::string& text) {
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    SymtabVisitor().visit_program(*ast);

    VerbatimVarRenameVisitor().visit_program(*ast);
    LocalVarRenameVisitor().visit_program(*ast);
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(*ast);
    return stream.str();
}

SCENARIO("Renaming with presence of local and global variables in same block",
         "[visitor][rename]") {
    GIVEN("A neuron block and procedure with same variable name") {
        std::string nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar
            }

            PROCEDURE rates() {
                LOCAL gNaTs2_tbar
                gNaTs2_tbar = 2.1 + ena
            }
        )";

        std::string expected_nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar
            }

            PROCEDURE rates() {
                LOCAL gNaTs2_tbar_r_0
                gNaTs2_tbar_r_0 = 2.1+ena
            }
        )";

        THEN("var renaming pass changes only local variables in procedure") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(expected_nmodl_text);
            auto result = run_local_var_rename_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Renaming in the absence of global blocks", "[visitor][rename]") {
    GIVEN("Procedures containing same variables") {
        std::string nmodl_text = R"(
            PROCEDURE rates_1() {
                LOCAL gNaTs2_tbar
                gNaTs2_tbar = 2.1+ena
            }

            PROCEDURE rates_2() {
                LOCAL gNaTs2_tbar
                gNaTs2_tbar = 2.1+ena
            }
        )";

        THEN("nothing gets renamed") {
            std::string input = reindent_text(nmodl_text);
            auto result = run_local_var_rename_visitor(input);
            REQUIRE(result == input);
        }
    }
}

SCENARIO("Variable renaming in nested blocks", "[visitor][rename]") {
    GIVEN("Mod file containing procedures with nested blocks") {
        std::string input_nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar
            }

            PARAMETER {
                gNaTs2_tbar = 0.1 (S/cm2)
                tau = 11.1
            }

            STATE {
                m
                h
            }

            BREAKPOINT {
                LOCAL gNaTs2_t
                gNaTs2_t = gNaTs2_tbar*m*m*m*h
                ina = gNaTs2_t*(v-ena)
                {
                    LOCAL gNaTs2_t, h
                    gNaTs2_t = m + h
                    {
                        LOCAL m
                        m = gNaTs2_t + h
                        {
                            LOCAL m, h
                            VERBATIM
                            _lm = 12
                            ENDVERBATIM
                        }
                    }
                }
            }

            PROCEDURE rates() {
                LOCAL x, m
                m = x + gNaTs2_tbar
                {
                    {
                        LOCAL h, x, gNaTs2_tbar
                        m = h * x * gNaTs2_tbar + tau
                    }
                }
            }
        )";

        std::string expected_nmodl_text = R"(
            NEURON {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar
            }

            PARAMETER {
                gNaTs2_tbar = 0.1 (S/cm2)
                tau = 11.1
            }

            STATE {
                m
                h
            }

            BREAKPOINT {
                LOCAL gNaTs2_t
                gNaTs2_t = gNaTs2_tbar*m*m*m*h
                ina = gNaTs2_t*(v-ena)
                {
                    LOCAL gNaTs2_t_r_0, h_r_1
                    gNaTs2_t_r_0 = m+h_r_1
                    {
                        LOCAL m_r_1
                        m_r_1 = gNaTs2_t_r_0+h_r_1
                        {
                            LOCAL m_r_0, h_r_0
                            VERBATIM
                            m_r_0 = 12
                            ENDVERBATIM
                        }
                    }
                }
            }

            PROCEDURE rates() {
                LOCAL x, m_r_2
                m_r_2 = x+gNaTs2_tbar
                {
                    {
                        LOCAL h_r_2, x_r_0, gNaTs2_tbar_r_0
                        m_r_2 = h_r_2*x_r_0*gNaTs2_tbar_r_0+tau
                    }
                }
            }
        )";

        THEN("variables conflicting with global variables get renamed starting from inner block") {
            std::string input = reindent_text(input_nmodl_text);
            auto expected_result = reindent_text(expected_nmodl_text);
            auto result = run_local_var_rename_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}


SCENARIO("Renaming in presence of local variable in verbatim block", "[visitor][rename]") {
    GIVEN("A neuron block and procedure with same variable name") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE gNaTs2_tbar
            }

            PROCEDURE rates() {
                LOCAL gNaTs2_tbar, x
                VERBATIM
                _lx = _lgNaTs2_tbar
                #define my_macro_var _lgNaTs2_tbar*2
                ENDVERBATIM
                gNaTs2_tbar = my_macro_var + 1
            }

            PROCEDURE alpha() {
                VERBATIM
                _p_gNaTs2_tbar = 12
                ENDVERBATIM
            }
        )";

        std::string expected_nmodl_text = R"(
            NEURON {
                RANGE gNaTs2_tbar
            }

            PROCEDURE rates() {
                LOCAL gNaTs2_tbar_r_0, x
                VERBATIM
                x = gNaTs2_tbar_r_0
                #define my_macro_var gNaTs2_tbar_r_0*2
                ENDVERBATIM
                gNaTs2_tbar_r_0 = my_macro_var+1
            }

            PROCEDURE alpha() {
                VERBATIM
                gNaTs2_tbar = 12
                ENDVERBATIM
            }
        )";

        THEN("var renaming pass changes local & global variable in verbatim block") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(expected_nmodl_text);
            auto result = run_local_var_rename_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}
