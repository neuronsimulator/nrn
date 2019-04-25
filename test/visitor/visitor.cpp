/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#define CATCH_CONFIG_RUNNER

#include <string>

#include "catch/catch.hpp"
#include "utils/logger.hpp"
#include <pybind11/embed.h>

#include "parser/nmodl_driver.hpp"
#include "test/utils/nmodl_constructs.hpp"
#include "test/utils/test_utils.hpp"
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/defuse_analyze_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/kinetic_block_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/loop_unroll_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/steadystate_visitor.hpp"
#include "visitors/sympy_conductance_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"

using json = nlohmann::json;

using namespace nmodl;
using namespace visitor;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;
using symtab::syminfo::NmodlType;

int main(int argc, char* argv[]) {
    // initialize python interpreter once for
    // entire catch executable
    pybind11::scoped_interpreter guard{};
    logger->set_level(spdlog::level::debug);
    int result = Catch::Session().run(argc, argv);
    return result;
}

//=============================================================================
// Verbatim visitor tests
//=============================================================================

std::vector<std::string> run_verbatim_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    VerbatimVisitor v;
    v.visit_program(ast.get());
    return v.verbatim_blocks();
}

TEST_CASE("Verbatim Visitor") {
    SECTION("Single Block") {
        std::string text = "VERBATIM int a; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 1);
        REQUIRE(blocks.front() == " int a; ");
    }

    SECTION("Multiple Blocks") {
        std::string text = "VERBATIM int a; ENDVERBATIM VERBATIM float b; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 2);
        REQUIRE(blocks[0] == " int a; ");
        REQUIRE(blocks[1] == " float b; ");
    }
}

//=============================================================================
// JSON visitor tests
//=============================================================================

std::string run_json_visitor(const std::string& text, bool compact = false) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);
    return to_json(ast.get(), compact);
}

TEST_CASE("JSON Visitor") {
    SECTION("JSON object test") {
        std::string nmodl_text = "NEURON {}";
        json expected = R"(
            {
              "Program": [
                {
                  "NeuronBlock": [
                    {
                      "StatementBlock": []
                    }
                  ]
                }
              ]
            }
        )"_json;

        auto json_text = run_json_visitor(nmodl_text);
        json result = json::parse(json_text);

        REQUIRE(expected == result);
    }

    SECTION("JSON text test (compact format)") {
        std::string nmodl_text = "NEURON {}";
        std::string expected = R"({"Program":[{"NeuronBlock":[{"StatementBlock":[]}]}]})";

        auto result = run_json_visitor(nmodl_text, true);
        REQUIRE(result == expected);
    }
}

//=============================================================================
// Symtab and Perf visitor tests
//=============================================================================

SCENARIO("Symbol table generation and Perf stat visitor pass") {
    GIVEN("A mod file and associated ast") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX NaTs2_t
                USEION na READ ena WRITE ina
                RANGE gNaTs2_tbar, A_AMPA_step  : range + anything = range (2)
                GLOBAL Rstate                   : global + anything = global
                POINTER rng                     : pointer = global
                BBCOREPOINTER coreRng           : pointer + assigned = range
            }

            PARAMETER   {
                gNaTs2_tbar = 0.00001 (S/cm2)   : range + parameter = range already
                tau_r = 0.2 (ms)                : parameter = global
                tau_d_AMPA = 1.0                : parameter = global
                tsyn_fac = 11.1                 : parameter + assigned = range
            }

            ASSIGNED    {
                v   (mV)        : only assigned = range
                ena (mV)        : only assigned = range
                tsyn_fac        : parameter + assigned = range already
                A_AMPA_step     : range + assigned = range already
                AmState         : only assigned = range
                Rstate          : global + assigned == global already
                coreRng         : pointer + assigned = range already
            }

            STATE {
                m               : state = range
                h               : state = range
            }

            BREAKPOINT  {
                CONDUCTANCE gNaTs2_t USEION na
                SOLVE states METHOD cnexp
                {
                    LOCAL gNaTs2_t
                    {
                        gNaTs2_t = gNaTs2_tbar*m*m*m*h
                    }
                    ina = gNaTs2_t*(v-ena)
                }
                {
                    m = hBetaf(11+v)
                    m = 12/gNaTs2_tbar
                }
                {
                    gNaTs2_tbar = gNaTs2_tbar*gNaTs2_tbar + 11.0
                    gNaTs2_tbar = 12.0
                }
            }

            FUNCTION hBetaf(v) {
                hBetaf = (-0.015 * (-v -60))/(1-(exp((-v -60)/6)))
            }
        )";

        NmodlDriver driver;
        auto ast = driver.parse_string(nmodl_text);

        WHEN("Symbol table generator pass runs") {
            SymtabVisitor v;
            v.visit_program(ast.get());
            auto symtab = ast->get_model_symbol_table();

            THEN("Can lookup for defined variables") {
                auto symbol = symtab->lookup("m");
                REQUIRE(symbol->has_any_property(NmodlType::dependent_def));
                REQUIRE_FALSE(symbol->has_any_property(NmodlType::local_var));

                symbol = symtab->lookup("gNaTs2_tbar");
                REQUIRE(symbol->has_any_property(NmodlType::param_assign));
                REQUIRE(symbol->has_any_property(NmodlType::range_var));

                symbol = symtab->lookup("ena");
                REQUIRE(symbol->has_any_property(NmodlType::read_ion_var));
            }
            THEN("Can lookup for defined functions") {
                auto symbol = symtab->lookup("hBetaf");
                REQUIRE(symbol->has_any_property(NmodlType::function_block));
            }
            THEN("Non existent variable lookup returns nullptr") {
                REQUIRE(symtab->lookup("xyz") == nullptr);
            }

            WHEN("Perf visitor pass runs after symtab visitor") {
                PerfVisitor v;
                v.visit_program(ast.get());

                auto result = v.get_total_perfstat();
                auto num_instance_var = v.get_instance_variable_count();
                auto num_global_var = v.get_global_variable_count();
                auto num_state_var = v.get_state_variable_count();
                auto num_const_instance_var = v.get_const_instance_variable_count();
                auto num_const_global_var = v.get_const_global_variable_count();

                THEN("Performance counters are updated") {
                    REQUIRE(result.n_add == 2);
                    REQUIRE(result.n_sub == 4);
                    REQUIRE(result.n_mul == 7);
                    REQUIRE(result.n_div == 3);
                    REQUIRE(result.n_exp == 1);
                    REQUIRE(result.n_global_read == 7);
                    REQUIRE(result.n_unique_global_read == 4);
                    REQUIRE(result.n_global_write == 3);
                    REQUIRE(result.n_unique_global_write == 2);
                    REQUIRE(result.n_constant_read == 4);
                    REQUIRE(result.n_unique_constant_read == 1);
                    REQUIRE(result.n_constant_write == 2);
                    REQUIRE(result.n_unique_constant_write == 1);
                    REQUIRE(result.n_local_read == 3);
                    REQUIRE(result.n_local_write == 2);
                    REQUIRE(result.n_ext_func_call == 1);
                    REQUIRE(result.n_int_func_call == 1);
                    REQUIRE(result.n_neg == 3);
                    REQUIRE(num_instance_var == 9);
                    REQUIRE(num_global_var == 4);
                    REQUIRE(num_state_var == 2);
                    REQUIRE(num_const_instance_var == 2);
                    REQUIRE(num_const_global_var == 2);
                }
            }
        }

        WHEN("Perf visitor pass runs before symtab visitor") {
            PerfVisitor v;
            THEN("exception is thrown") {
                REQUIRE_THROWS_WITH(v.visit_program(ast.get()), Catch::Contains("table not setup"));
            }
        }
    }
}

//=============================================================================
// AST to NMODL printer tests
//=============================================================================

std::string run_nmodl_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    std::stringstream stream;
    NmodlPrintVisitor v(stream);

    v.visit_program(ast.get());
    return stream.str();
}

SCENARIO("Test for AST back to NMODL transformation") {
    for (const auto& construct: nmodl_valid_constructs) {
        auto test_case = construct.second;
        std::string input_nmodl_text = reindent_text(test_case.input);
        std::string output_nmodl_text = reindent_text(test_case.output);
        GIVEN(test_case.name) {
            THEN("Visitor successfully returns : " + input_nmodl_text) {
                auto result = run_nmodl_visitor(input_nmodl_text);
                REQUIRE(result == output_nmodl_text);
            }
        }
    }
}

//=============================================================================
// Variable rename tests
//=============================================================================
std::string run_var_rename_visitor(const std::string& text,
                                   std::vector<std::pair<std::string, std::string>> variables) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);
    {
        for (const auto& variable: variables) {
            RenameVisitor v(variable.first, variable.second);
            v.visit_program(ast.get());
        }
    }
    std::stringstream stream;
    {
        NmodlPrintVisitor v(stream);
        v.visit_program(ast.get());
    }
    return stream.str();
}

SCENARIO("Renaming any variable in mod file with RenameVisitor") {
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
    auto ast = driver.parse_string(text);

    {
        SymtabVisitor v;
        v.visit_program(ast.get());
    }

    {
        VerbatimVarRenameVisitor v;
        v.visit_program(ast.get());
    }

    {
        LocalVarRenameVisitor v;
        v.visit_program(ast.get());
    }
    std::stringstream stream;
    {
        NmodlPrintVisitor v(stream);
        v.visit_program(ast.get());
    }
    return stream.str();
}

SCENARIO("Presence of local and global variables in same block") {
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

SCENARIO("Absence of global blocks") {
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

SCENARIO("Variable renaming in nested blocks") {
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


SCENARIO("Presence of local variable in verbatim block") {
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

//=============================================================================
// Procedure/Function inlining tests
//=============================================================================

std::string run_inline_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    {
        SymtabVisitor v;
        v.visit_program(ast.get());
    }

    {
        InlineVisitor v;
        v.visit_program(ast.get());
    }

    std::stringstream stream;
    {
        NmodlPrintVisitor v(stream);
        v.visit_program(ast.get());
    }
    return stream.str();
}

SCENARIO("External procedure calls") {
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

SCENARIO("Simple procedure inlining") {
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

SCENARIO("Nested procedure inlining") {
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

SCENARIO("Inline function call in procedure") {
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

SCENARIO("Function call within conditional statement") {
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

SCENARIO("Multiple function calls in same statement") {
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

SCENARIO("Nested function calls withing arguments") {
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

SCENARIO("Function call in non-binary expression") {
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


SCENARIO("Function call as standalone expression") {
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

SCENARIO("Procedure call as standalone statement as well as part of expression") {
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

SCENARIO("Procedure inlining handles local-global name conflict") {
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


//=============================================================================
// DefUseAnalyze visitor tests
//=============================================================================

std::vector<DUChain> run_defuse_visitor(const std::string& text, const std::string& variable) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    SymtabVisitor().visit_program(ast.get());
    InlineVisitor().visit_program(ast.get());

    std::vector<DUChain> chains;
    DefUseAnalyzeVisitor v(ast->get_symbol_table());

    /// analyse only derivative blocks in this test
    auto blocks = AstLookupVisitor().lookup(ast.get(), AstNodeType::DERIVATIVE_BLOCK);
    for (auto& block: blocks) {
        auto node = block.get();
        chains.push_back(v.analyze(node, variable));
    }
    return chains;
}

SCENARIO("Running defuse analyzer") {
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
                REQUIRE(m1[0].to_string() == R"({"DerivativeBlock":[{"name":"U"}]})");
                REQUIRE(h1[0].to_string() == R"({"DerivativeBlock":[{"name":"D"}]})");
                REQUIRE(tau0[0].to_string() == R"({"DerivativeBlock":[{"name":"LD"}]})");
                REQUIRE(tau1[0].to_string() == R"({"DerivativeBlock":[{"name":"LU"}]})");
                REQUIRE(tau2[0].to_string() ==
                        R"({"DerivativeBlock":[{"name":"LU"},{"name":"LD"}]})");
                REQUIRE(n0[0].to_string() == R"({"DerivativeBlock":[{"name":"U"},{"name":"D"}]})");
                REQUIRE(n1[0].to_string() == R"({"DerivativeBlock":[{"name":"U"},{"name":"D"}]})");
                REQUIRE(o0[0].to_string() == R"({"DerivativeBlock":[{"name":"D"}]})");
            }
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


//=============================================================================
// Localizer visitor tests
//=============================================================================

std::string run_localize_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    {
        SymtabVisitor v1;
        v1.visit_program(ast.get());
        InlineVisitor v2;
        v2.visit_program(ast.get());
        LocalizeVisitor v3;
        v3.visit_program(ast.get());
    }

    std::stringstream stream;
    {
        NmodlPrintVisitor v(stream);
        v.visit_program(ast.get());
    }
    return stream.str();
}


SCENARIO("Localizer test with single global block") {
    GIVEN("Single derivative block with variable definition") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                LOCAL tau
                tau = 11.1
                exp(tau)
            }
        )";

        THEN("tau variable gets localized") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

SCENARIO("Localizer test with use of verbatim block") {
    GIVEN("Verbatim block usage in one of the global block") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                VERBATIM ENDVERBATIM
            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                VERBATIM ENDVERBATIM
            }
        )";

        THEN("Localization is disabled") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}


SCENARIO("Localizer test with multiple global blocks") {
    GIVEN("Multiple global blocks with definition of variable") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, beta
            }

            INITIAL {
                LOCAL tau
                tau = beta
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                IF (1) {
                    tau = beta
                } ELSE {
                    tau = 11
                }

            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau, beta
            }

            INITIAL {
                LOCAL tau
                tau = beta
            }

            DERIVATIVE states {
                LOCAL tau
                tau = 11.1
                exp(tau)
            }

            BREAKPOINT {
                LOCAL tau
                IF (1) {
                    tau = beta
                } ELSE {
                    tau = 11
                }
            }
        )";

        THEN("Localization across multiple blocks is done") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }


    GIVEN("Two global blocks with definition and use of the variable") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
            }

            BREAKPOINT {
                IF (1) {
                    tau = 22
                } ELSE {
                    tau = exp(tau) + 11
                }

            }
        )";

        std::string output_nmodl = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
            }

            BREAKPOINT {
                IF (1) {
                    tau = 22
                } ELSE {
                    tau = exp(tau)+11
                }
            }
        )";

        THEN("Localization is not done due to use of variable") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_localize_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}


//=============================================================================
// CnexpSolve visitor tests
//=============================================================================

std::string run_cnexp_solve_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    SymtabVisitor().visit_program(ast.get());
    NeuronSolveVisitor().visit_program(ast.get());
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(ast.get());
    return stream.str();
}


SCENARIO("CnexpSolver visitor solving ODEs") {
    GIVEN("Derivative block with cnexp method in breakpoint block") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
                m = m + h
            }
        )";

        std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m = m+(1-exp(dt*((((-1)))/mTau)))*(-(((mInf))/mTau)/((((-1)))/mTau)-m)
                h = h+(1-exp(dt*((((-1)))/hTau)))*(-(((hInf))/hTau)/((((-1)))/hTau)-h)
                m = m+h
            }
        )";

        THEN("ODEs get replaced with solution") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("Derivative block without any solve method specification") {
        std::string nmodl_text = R"(
            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }
        )";

        std::string output_nmodl = R"(
            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }
        )";

        THEN("ODEs don't get solved") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("Derivative block with non-cnexp method in breakpoint block") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD derivimplicit
            }

            DERIVATIVE states {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }
        )";

        std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD derivimplicit
            }

            DERIVATIVE states {
                Dm = (mInf-m)/mTau
                Dh = (hInf-h)/hTau
            }
        )";

        THEN("ODEs don't get solved but state variables get replaced with Dstate ") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }

    GIVEN("Derivative block with ODEs that needs non-cnexp method to solve") {
        std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                A_AMPA' = tau_r_AMPA/A_AMPA
            }
        )";

        std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                A_AMPA' = tau_r_AMPA/A_AMPA
            }
        )";

        THEN("ODEs don't get replaced as cnexp is not possible") {
            std::string input = reindent_text(nmodl_text);
            auto expected_result = reindent_text(output_nmodl);
            auto result = run_cnexp_solve_visitor(input);
            REQUIRE(result == expected_result);
        }
    }
}

//=============================================================================
// SolveBlock visitor tests
//=============================================================================

std::string run_solve_block_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);
    SymtabVisitor().visit_program(ast.get());
    NeuronSolveVisitor().visit_program(ast.get());
    SolveBlockVisitor().visit_program(ast.get());
    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(ast.get());
    return stream.str();
}

TEST_CASE("SolveBlock visitor") {
    SECTION("SolveBlock add NrnState block") {
        GIVEN("Breakpoint block with single solve block in breakpoint") {
            std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m' = (mInf-m)/mTau
            }
        )";

            std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                m = m+(1-exp(dt*((((-1)))/mTau)))*(-(((mInf))/mTau)/((((-1)))/mTau)-m)
            }

            NRN_STATE SOLVE states METHOD cnexp{
                m = m+(1-exp(dt*((((-1)))/mTau)))*(-(((mInf))/mTau)/((((-1)))/mTau)-m)
            }

        )";

            THEN("Single NrnState block gets added") {
                auto result = run_solve_block_visitor(nmodl_text);
                REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
            }
        }

        GIVEN("Breakpoint block with two solve block in breakpoint") {
            std::string nmodl_text = R"(
            BREAKPOINT {
                SOLVE state1 METHOD cnexp
                SOLVE state2 METHOD cnexp
            }

            DERIVATIVE state1 {
                m' = (mInf-m)/mTau
            }

            DERIVATIVE state2 {
                h' = (mInf-h)/mTau
            }
        )";

            std::string output_nmodl = R"(
            BREAKPOINT {
                SOLVE state1 METHOD cnexp
                SOLVE state2 METHOD cnexp
            }

            DERIVATIVE state1 {
                m = m+(1-exp(dt*((((-1)))/mTau)))*(-(((mInf))/mTau)/((((-1)))/mTau)-m)
            }

            DERIVATIVE state2 {
                h = h+(1-exp(dt*((((-1)))/mTau)))*(-(((mInf))/mTau)/((((-1)))/mTau)-h)
            }

            NRN_STATE SOLVE state1 METHOD cnexp{
                m = m+(1-exp(dt*((((-1)))/mTau)))*(-(((mInf))/mTau)/((((-1)))/mTau)-m)
            }
            SOLVE state2 METHOD cnexp{
                h = h+(1-exp(dt*((((-1)))/mTau)))*(-(((mInf))/mTau)/((((-1)))/mTau)-h)
            }

        )";

            THEN("NrnState blok combining multiple solve nodes added") {
                auto result = run_solve_block_visitor(nmodl_text);
                REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
            }
        }
    }
}

//=============================================================================
// Passes can run multiple times
//=============================================================================

void run_visitor_passes(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);
    {
        SymtabVisitor v1;
        InlineVisitor v2;
        LocalizeVisitor v3;
        v1.visit_program(ast.get());
        v2.visit_program(ast.get());
        v3.visit_program(ast.get());
        v1.visit_program(ast.get());
        v1.visit_program(ast.get());
        v2.visit_program(ast.get());
        v3.visit_program(ast.get());
        v2.visit_program(ast.get());
    }
}


SCENARIO("Running visitor passes multiple time") {
    GIVEN("A mod file") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
            }
        )";

        THEN("Passes can run multiple times") {
            std::string input = reindent_text(nmodl_text);
            REQUIRE_NOTHROW(run_visitor_passes(input));
        }
    }
}


//=============================================================================
// Ast lookup visitor tests
//=============================================================================

std::vector<std::shared_ptr<ast::Ast>> run_lookup_visitor(ast::Program* node,
                                                          std::vector<AstNodeType>& types) {
    AstLookupVisitor v;
    return v.lookup(node, types);
}

SCENARIO("Searching for ast nodes using AstLookupVisitor") {
    auto to_ast = [](const std::string& text) {
        NmodlDriver driver;
        return driver.parse_string(text);
    };

    GIVEN("A mod file with nodes of type NEURON, RANGE, BinaryExpression") {
        std::string nmodl_text = R"(
            NEURON {
                RANGE tau, h
            }

            DERIVATIVE states {
                tau = 11.1
                exp(tau)
                h' = h + 2
            }

            : My comment here
        )";

        auto ast = to_ast(nmodl_text);

        WHEN("Looking for existing nodes") {
            THEN("Can find RANGE variables") {
                std::vector<AstNodeType> types{AstNodeType::RANGE_VAR};
                auto result = run_lookup_visitor(ast.get(), types);
                REQUIRE(result.size() == 2);
                REQUIRE(to_nmodl(result[0].get()) == "tau");
                REQUIRE(to_nmodl(result[1].get()) == "h");
            }

            THEN("Can find NEURON block") {
                AstLookupVisitor v(AstNodeType::NEURON_BLOCK);
                ast->accept(&v);
                auto nodes = v.get_nodes();
                REQUIRE(nodes.size() == 1);

                std::string neuron_block = R"(
                    NEURON {
                        RANGE tau, h
                    })";
                auto result = reindent_text(to_nmodl(nodes[0].get()));
                auto expected = reindent_text(neuron_block);
                REQUIRE(result == expected);
            }

            THEN("Can find Binary Expressions and function call") {
                std::vector<AstNodeType> types{AstNodeType::BINARY_EXPRESSION,
                                               AstNodeType::FUNCTION_CALL};
                auto result = run_lookup_visitor(ast.get(), types);
                REQUIRE(result.size() == 4);
            }
        }

        WHEN("Looking for missing nodes") {
            THEN("Can not find BREAKPOINT block") {
                std::vector<AstNodeType> types{AstNodeType::BREAKPOINT_BLOCK};
                auto result = run_lookup_visitor(ast.get(), types);
                REQUIRE(result.size() == 0);
            }
        }
    }
}


//=============================================================================
// KineticBlock visitor tests
//=============================================================================

std::vector<std::string> run_kinetic_block_visitor(
    const std::string& text,
    bool pade = false,
    bool cse = false,
    AstNodeType ret_nodetype = AstNodeType::DERIVATIVE_BLOCK) {
    std::vector<std::string> results;

    // construct AST from text including KINETIC block(s)
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor().visit_program(ast.get());

    // unroll loops and fold constants
    ConstantFolderVisitor().visit_program(ast.get());
    LoopUnrollVisitor().visit_program(ast.get());
    ConstantFolderVisitor().visit_program(ast.get());
    SymtabVisitor().visit_program(ast.get());

    // run KineticBlock visitor on AST
    KineticBlockVisitor().visit_program(ast.get());

    // run lookup visitor to extract DERIVATIVE block(s) from AST
    AstLookupVisitor v_lookup;
    auto res = v_lookup.lookup(ast.get(), ret_nodetype);
    for (const auto& r: res) {
        results.push_back(to_nmodl(r.get()));
    }

    return results;
}

SCENARIO("KineticBlock visitor", "[kinetic]") {
    GIVEN("KINETIC block with << reaction statement, 1 state var") {
        std::string input_nmodl_text = R"(
            STATE {
                x
            }
            KINETIC states {
                ~ x << (a*c/3.2)
            })";
        std::string output_nmodl_text = R"(
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
    auto ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor().visit_program(ast.get());

    // unroll loops and fold constants
    ConstantFolderVisitor().visit_program(ast.get());
    LoopUnrollVisitor().visit_program(ast.get());
    ConstantFolderVisitor().visit_program(ast.get());
    SymtabVisitor().visit_program(ast.get());

    // run SympySolver on AST
    SympySolverVisitor(pade, cse).visit_program(ast.get());

    // run lookup visitor to extract results from AST
    AstLookupVisitor v_lookup;
    auto res = v_lookup.lookup(ast.get(), ret_nodetype);
    for (const auto& r: res) {
        results.push_back(to_nmodl(r.get()));
    }

    return results;
}

void run_sympy_visitor_passes(ast::Program* node) {
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

std::string ast_to_string(ast::Program* node) {
    std::stringstream stream;
    {
        NmodlPrintVisitor v(stream);
        v.visit_program(node);
    }
    return stream.str();
}

SCENARIO("SympySolver visitor: cnexp or euler", "[sympy][cnexp][euler]") {
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
            REQUIRE(result[3] == "y' = c3*y*y*y");
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
        SymtabVisitor().visit_program(ast.get());

        // run SympySolver on AST
        SympySolverVisitor().visit_program(ast.get());

        std::string AST_string = ast_to_string(ast.get());

        THEN("More SympySolver passes do nothing to the AST and don't throw") {
            REQUIRE_NOTHROW(run_sympy_visitor_passes(ast.get()));
            REQUIRE(AST_string == ast_to_string(ast.get()));
        }
    }
}

SCENARIO("SympySolver visitor: derivimplicit or sparse", "[sympy][derivimplicit][sparse]") {
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
            auto result = run_sympy_solver_visitor(nmodl_text, false, false,
                                                   AstNodeType::DERIVATIVE_BLOCK);
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
            auto result = run_sympy_solver_visitor(nmodl_text, false, false,
                                                   AstNodeType::DERIVATIVE_BLOCK);
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
            auto result = run_sympy_solver_visitor(nmodl_text, false, false,
                                                   AstNodeType::DERIVATIVE_BLOCK);
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
// SympyConductance visitor tests
//=============================================================================

std::string run_sympy_conductance_visitor(const std::string& text) {
    // construct AST from text
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor(false).visit_program(ast.get());

    // run constant folding, inlining & local renaming first
    ConstantFolderVisitor().visit_program(ast.get());
    InlineVisitor().visit_program(ast.get());
    LocalVarRenameVisitor().visit_program(ast.get());
    SymtabVisitor(true).visit_program(ast.get());

    // run SympyConductance on AST
    SympyConductanceVisitor().visit_program(ast.get());

    // run lookup visitor to extract results from AST
    AstLookupVisitor v_lookup;
    // return BREAKPOINT block as JSON string
    return reindent_text(
        nmodl::to_nmodl(v_lookup.lookup(ast.get(), AstNodeType::BREAKPOINT_BLOCK)[0].get()));
}

std::string breakpoint_to_nmodl(const std::string& text) {
    // construct AST from text
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor v_symtab;
    v_symtab.visit_program(ast.get());

    // run lookup visitor to extract results from AST
    AstLookupVisitor v_lookup;
    // return BREAKPOINT block as JSON string
    return reindent_text(
        nmodl::to_nmodl(v_lookup.lookup(ast.get(), AstNodeType::BREAKPOINT_BLOCK)[0].get()));
}

void run_sympy_conductance_passes(ast::Program* node) {
    // construct symbol table from AST
    SymtabVisitor v_symtab;
    v_symtab.visit_program(node);

    // run SympySolver on AST several times
    SympyConductanceVisitor v_sympy1;
    v_sympy1.visit_program(node);
    v_symtab.visit_program(node);
    v_sympy1.visit_program(node);
    v_symtab.visit_program(node);

    // also use a second instance of SympySolver
    SympyConductanceVisitor v_sympy2;
    v_sympy2.visit_program(node);
    v_symtab.visit_program(node);
    v_sympy1.visit_program(node);
    v_symtab.visit_program(node);
    v_sympy2.visit_program(node);
    v_symtab.visit_program(node);
}

SCENARIO("SympyConductance visitor", "[sympy]") {
    // First set of test mod files below all based on:
    // nmodldb/models/db/bluebrain/CortexSimplified/mod/Ca.mod
    GIVEN("breakpoint block containing VERBATIM statement") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                RANGE gCabar, gCa, ica
            }
            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                VERBATIM
                double z=0;
                ENDVERBATIM
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                VERBATIM
                double z=0;
                ENDVERBATIM
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
            }
        )";
        THEN("Do nothing") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("breakpoint block containing IF/ELSE block") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                RANGE gCabar, gCa, ica
            }
            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                IF(gCabar<1){
                    gCa = gCabar*m*m*h
                    ica = gCa*(v-eca)
                } ELSE {
                    gCa = 0
                    ica = 0
                }
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                IF(gCabar<1){
                    gCa = gCabar*m*m*h
                    ica = gCa*(v-eca)
                } ELSE {
                    gCa = 0
                    ica = 0
                }
            }
        )";
        THEN("Do nothing") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("ion current, existing CONDUCTANCE hint & var") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                RANGE gCabar, gCa, ica
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                gCa (S/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
            }
        )";
        THEN("Do nothing") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("ion current, no CONDUCTANCE hint, existing var") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                RANGE gCabar, gCa, ica
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                gCa (S/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
            }
        )";
        THEN("Add CONDUCTANCE hint using existing var") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("ion current, no CONDUCTANCE hint, no existing var") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                RANGE gCabar, ica
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                SOLVE states METHOD cnexp
                ica = (gCabar*m*m*h)*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                LOCAL g_ca_0
                CONDUCTANCE g_ca_0 USEION ca
                g_ca_0 = gCabar*h*pow(m, 2)
                SOLVE states METHOD cnexp
                ica = (gCabar*m*m*h)*(v-eca)
            }
        )";
        THEN("Add CONDUCTANCE hint with new local var") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("2 ion currents, 1 CONDUCTANCE hint, 1 existing var") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                USEION na READ ena WRITE ina
                RANGE gCabar, gNabar, ica, ina
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
                gNabar = 0.00005 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                ina (mA/cm2)
                gCa (S/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
                ina = (gNabar*m*h)*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                LOCAL g_na_0
                CONDUCTANCE g_na_0 USEION na
                g_na_0 = gNabar*h*m
                CONDUCTANCE gCa USEION ca
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
                ina = (gNabar*m*h)*(v-eca)
            }
        )";
        THEN("Add 1 CONDUCTANCE hint with new local var") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("2 ion currents, no CONDUCTANCE hints, 1 existing var") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                USEION na READ ena WRITE ina
                RANGE gCabar, gNabar, ica, ina
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
                gNabar = 0.00005 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                ina (mA/cm2)
                gCa (S/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
                ina = (gNabar*m*h)*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                LOCAL g_na_0
                CONDUCTANCE g_na_0 USEION na
                CONDUCTANCE gCa USEION ca
                g_na_0 = gNabar*h*m
                SOLVE states METHOD cnexp
                gCa = gCabar*m*m*h
                ica = gCa*(v-eca)
                ina = (gNabar*m*h)*(v-eca)
            }
        )";
        THEN("Add 2 CONDUCTANCE hints, 1 with existing var, 1 with new local var") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("2 ion currents, no CONDUCTANCE hints, no existing vars") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                USEION na READ ena WRITE ina
                RANGE gCabar, gNabar, ica, ina
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
                gNabar = 0.00005 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                ina (mA/cm2)
                gCa (S/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                SOLVE states METHOD cnexp
                ica = (gCabar*m*m*h)*(v-eca)
                ina = (gNabar*m*h)*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                LOCAL g_ca_0, g_na_0
                CONDUCTANCE g_na_0 USEION na
                CONDUCTANCE g_ca_0 USEION ca
                g_ca_0 = gCabar*h*pow(m, 2)
                g_na_0 = gNabar*h*m
                SOLVE states METHOD cnexp
                ica = (gCabar*m*m*h)*(v-eca)
                ina = (gNabar*m*h)*(v-eca)
            }
        )";
        THEN("Add 2 CONDUCTANCE hints with 2 new local vars") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("1 ion current, 1 nonspecific current, no CONDUCTANCE hints, no existing vars") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                NONSPECIFIC_CURRENT ihcn
                RANGE gCabar, ica
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                ihcn (mA/cm2)
                gCa (S/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                SOLVE states METHOD cnexp
                ica = (gCabar*m*m*h)*(v-eca)
                ihcn = (0.1235*m*h)*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                LOCAL g_ca_0, g__0
                CONDUCTANCE g__0
                CONDUCTANCE g_ca_0 USEION ca
                g_ca_0 = gCabar*h*pow(m, 2)
                g__0 = 0.1235*h*m
                SOLVE states METHOD cnexp
                ica = (gCabar*m*m*h)*(v-eca)
                ihcn = (0.1235*m*h)*(v-eca)
            }
        )";
        THEN("Add 2 CONDUCTANCE hints with 2 new local vars") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    GIVEN("1 ion current, 1 nonspecific current, no CONDUCTANCE hints, 1 existing var") {
        std::string nmodl_text = R"(
            NEURON  {
                SUFFIX Ca
                USEION ca READ eca WRITE ica
                NONSPECIFIC_CURRENT ihcn
                RANGE gCabar, ica, gihcn
            }

            UNITS   {
                (S) = (siemens)
                (mV) = (millivolt)
                (mA) = (milliamp)
            }

            PARAMETER   {
                gCabar = 0.00001 (S/cm2)
            }

            ASSIGNED    {
                v   (mV)
                eca (mV)
                ica (mA/cm2)
                ihcn (mA/cm2)
                gCa (S/cm2)
                gihcn (S/cm2)
                mInf
                mTau
                mAlpha
                mBeta
                hInf
                hTau
                hAlpha
                hBeta
            }

            STATE   {
                m
                h
            }

            BREAKPOINT  {
                SOLVE states METHOD cnexp
                gihcn = 0.1235*m*h
                ica = (gCabar*m*m*h)*(v-eca)
                ihcn = gihcn*(v-eca)
            }

            DERIVATIVE states   {
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }

            INITIAL{
                m = mInf
                h = hInf
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT  {
                LOCAL g_ca_0
                CONDUCTANCE gihcn
                CONDUCTANCE g_ca_0 USEION ca
                g_ca_0 = gCabar*h*pow(m, 2)
                SOLVE states METHOD cnexp
                gihcn = 0.1235*m*h
                ica = (gCabar*m*m*h)*(v-eca)
                ihcn = gihcn*(v-eca)
            }
        )";
        THEN("Add 2 CONDUCTANCE hints, 1 using existing var, 1 with new local var") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    // based on bluebrain/CortextPlastic/mod/ProbAMPANMDA.mod
    GIVEN(
        "2 ion currents, 1 nonspecific current, no CONDUCTANCE hints, indirect relation between "
        "eqns") {
        std::string nmodl_text = R"(
            NEURON {
                THREADSAFE
                    POINT_PROCESS ProbAMPANMDA
                    RANGE tau_r_AMPA, tau_d_AMPA, tau_r_NMDA, tau_d_NMDA
                    RANGE Use, u, Dep, Fac, u0, mg, NMDA_ratio
                    RANGE i, i_AMPA, i_NMDA, g_AMPA, g_NMDA, g, e
                    NONSPECIFIC_CURRENT i, i_AMPA,i_NMDA
                    POINTER rng
                    RANGE synapseID, verboseLevel
            }
            PARAMETER {
                    tau_r_AMPA = 0.2   (ms)  : dual-exponential conductance profile
                    tau_d_AMPA = 1.7    (ms)  : IMPORTANT: tau_r < tau_d
                    tau_r_NMDA = 0.29   (ms) : dual-exponential conductance profile
                    tau_d_NMDA = 43     (ms) : IMPORTANT: tau_r < tau_d
                    Use = 1.0   (1)   : Utilization of synaptic efficacy (just initial values! Use, Dep and Fac are overwritten by BlueBuilder assigned values)
                    Dep = 100   (ms)  : relaxation time constant from depression
                    Fac = 10   (ms)  :  relaxation time constant from facilitation
                    e = 0     (mV)  : AMPA and NMDA reversal potential
                    mg = 1   (mM)  : initial concentration of mg2+
                    mggate
                    gmax = .001 (uS) : weight conversion factor (from nS to uS)
                    u0 = 0 :initial value of u, which is the running value of Use
                    NMDA_ratio = 0.71 (1) : The ratio of NMDA to AMPA
                    synapseID = 0
                    verboseLevel = 0
            }
            ASSIGNED {
                    v (mV)
                    i (nA)
                    i_AMPA (nA)
                    i_NMDA (nA)
                    g_AMPA (uS)
                    g_NMDA (uS)
                    g (uS)
                    factor_AMPA
                    factor_NMDA
                    rng
            }
            STATE {
                    A_AMPA       : AMPA state variable to construct the dual-exponential profile - decays with conductance tau_r_AMPA
                    B_AMPA       : AMPA state variable to construct the dual-exponential profile - decays with conductance tau_d_AMPA
                    A_NMDA       : NMDA state variable to construct the dual-exponential profile - decays with conductance tau_r_NMDA
                    B_NMDA       : NMDA state variable to construct the dual-exponential profile - decays with conductance tau_d_NMDA
            }
            BREAKPOINT {
                    SOLVE state METHOD cnexp
                    mggate = 1.2
                    g_AMPA = gmax*(B_AMPA-A_AMPA) :compute time varying conductance as the difference of state variables B_AMPA and A_AMPA
                    g_NMDA = gmax*(B_NMDA-A_NMDA) * mggate :compute time varying conductance as the difference of state variables B_NMDA and A_NMDA and mggate kinetics
                    g = g_AMPA + g_NMDA
                    i_AMPA = g_AMPA*(v-e) :compute the AMPA driving force based on the time varying conductance, membrane potential, and AMPA reversal
                    i_NMDA = g_NMDA*(v-e) :compute the NMDA driving force based on the time varying conductance, membrane potential, and NMDA reversal
                    i = i_AMPA + i_NMDA
            }
            DERIVATIVE state{
                    A_AMPA' = -A_AMPA/tau_r_AMPA
                    B_AMPA' = -B_AMPA/tau_d_AMPA
                    A_NMDA' = -A_NMDA/tau_r_NMDA
                    B_NMDA' = -B_NMDA/tau_d_NMDA
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT {
                    CONDUCTANCE g
                    CONDUCTANCE g_NMDA
                    CONDUCTANCE g_AMPA
                    SOLVE state METHOD cnexp
                    mggate = 1.2
                    g_AMPA = gmax*(B_AMPA-A_AMPA) :compute time varying conductance as the difference of state variables B_AMPA and A_AMPA
                    g_NMDA = gmax*(B_NMDA-A_NMDA) * mggate :compute time varying conductance as the difference of state variables B_NMDA and A_NMDA and mggate kinetics
                    g = g_AMPA + g_NMDA
                    i_AMPA = g_AMPA*(v-e) :compute the AMPA driving force based on the time varying conductance, membrane potential, and AMPA reversal
                    i_NMDA = g_NMDA*(v-e) :compute the NMDA driving force based on the time varying conductance, membrane potential, and NMDA reversal
                    i = i_AMPA + i_NMDA
            }
        )";
        THEN("Add 3 CONDUCTANCE hints, using existing vars") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
    // based on neurodamus/bbp/lib/modlib/GluSynapse.mod
    GIVEN("1 nonspecific current, no CONDUCTANCE hints, many eqs & a function involved") {
        std::string nmodl_text = R"(
            NEURON {
                GLOBAL tau_r_AMPA, E_AMPA
                RANGE tau_d_AMPA, gmax_AMPA
                RANGE g_AMPA
                GLOBAL tau_r_NMDA, tau_d_NMDA, E_NMDA
                RANGE g_NMDA
                RANGE Use, Dep, Fac, Nrrp, u
                RANGE tsyn, unoccupied, occupied
                RANGE ica_NMDA
                RANGE volume_CR
                GLOBAL ljp_VDCC, vhm_VDCC, km_VDCC, mtau_VDCC, vhh_VDCC, kh_VDCC, htau_VDCC
                RANGE gca_bar_VDCC, ica_VDCC
                GLOBAL gamma_ca_CR, tau_ca_CR, min_ca_CR, cao_CR
                GLOBAL tau_GB, gamma_d_GB, gamma_p_GB, rho_star_GB, tau_Use_GB, tau_effca_GB
                RANGE theta_d_GB, theta_p_GB
                RANGE rho0_GB
                RANGE enable_GB, depress_GB, potentiate_GB
                RANGE Use_d_GB, Use_p_GB
                GLOBAL p_gen_RW, p_elim0_RW, p_elim1_RW
                RANGE enable_RW, synstate_RW
                GLOBAL mg, scale_mg, slope_mg
                RANGE vsyn, NMDA_ratio, synapseID, selected_for_report, verbose
                NONSPECIFIC_CURRENT i
            }
            UNITS {
                (nA)    = (nanoamp)
                (mV)    = (millivolt)
                (uS)    = (microsiemens)
                (nS)    = (nanosiemens)
                (pS)    = (picosiemens)
                (umho)  = (micromho)
                (um)    = (micrometers)
                (mM)    = (milli/liter)
                (uM)    = (micro/liter)
                FARADAY = (faraday) (coulomb)
                PI      = (pi)      (1)
                R       = (k-mole)  (joule/degC)
            }
            ASSIGNED {
                g_AMPA          (uS)
                g_NMDA          (uS)
                ica_NMDA        (nA)
                ica_VDCC        (nA)
                depress_GB      (1)
                potentiate_GB   (1)
                v               (mV)
                vsyn            (mV)
                i               (nA)
            }
            FUNCTION nernst(ci(mM), co(mM), z) (mV) {
                nernst = (1000) * R * (celsius + 273.15) / (z*FARADAY) * log(co/ci)
            }
            BREAKPOINT {
                LOCAL Eca_syn, mggate, i_AMPA, gmax_NMDA, i_NMDA, Pf_NMDA, gca_bar_abs_VDCC, gca_VDCC
                g_AMPA = (1e-3)*gmax_AMPA*(B_AMPA-A_AMPA)
                i_AMPA = g_AMPA*(v-E_AMPA)
                gmax_NMDA = gmax_AMPA*NMDA_ratio
                mggate = 1 / (1 + exp(slope_mg * -(v)) * (mg / scale_mg))
                g_NMDA = (1e-3)*gmax_NMDA*mggate*(B_NMDA-A_NMDA)
                i_NMDA = g_NMDA*(v-E_NMDA)
                Pf_NMDA  = (4*cao_CR) / (4*cao_CR + (1/1.38) * 120 (mM)) * 0.6
                ica_NMDA = Pf_NMDA*g_NMDA*(v-40.0)
                gca_bar_abs_VDCC = gca_bar_VDCC * 4(um2)*PI*(3(1/um3)/4*volume_CR*1/PI)^(2/3)
                gca_VDCC = (1e-3) * gca_bar_abs_VDCC * m_VDCC * m_VDCC * h_VDCC
                Eca_syn = FARADAY*nernst(cai_CR, cao_CR, 2)
                ica_VDCC = gca_VDCC*(v-Eca_syn)
                vsyn = v
                i = i_AMPA + i_NMDA + ica_VDCC
            }
        )";
        std::string breakpoint_text = R"(
            BREAKPOINT {
                LOCAL Eca_syn, mggate, i_AMPA, gmax_NMDA, i_NMDA, Pf_NMDA, gca_bar_abs_VDCC, gca_VDCC, nernst_in_0, g__0
                CONDUCTANCE g__0
                g__0 = (0.001*gmax_NMDA*mg*scale_mg*slope_mg*(A_NMDA-B_NMDA)*(E_NMDA-v)*exp(slope_mg*v)-0.001*gmax_NMDA*scale_mg*(A_NMDA-B_NMDA)*(mg+scale_mg*exp(slope_mg*v))*exp(slope_mg*v)+(g_AMPA+gca_VDCC)*pow(mg+scale_mg*exp(slope_mg*v), 2))/pow(mg+scale_mg*exp(slope_mg*v), 2)
                g_AMPA = 0.001*gmax_AMPA*(B_AMPA-A_AMPA)
                i_AMPA = g_AMPA*(v-E_AMPA)
                gmax_NMDA = gmax_AMPA*NMDA_ratio
                mggate = 1/(1+exp(slope_mg*-v)*(mg/scale_mg))
                g_NMDA = 0.001*gmax_NMDA*mggate*(B_NMDA-A_NMDA)
                i_NMDA = g_NMDA*(v-E_NMDA)
                Pf_NMDA = (4*cao_CR)/(4*cao_CR+0.7246376811594204*120(mM))*0.6
                ica_NMDA = Pf_NMDA*g_NMDA*(v-40)
                gca_bar_abs_VDCC = gca_bar_VDCC*4(um2)*PI*(3(1/um3)/4*volume_CR*1/PI)^0.6666666666666666
                gca_VDCC = 0.001*gca_bar_abs_VDCC*m_VDCC*m_VDCC*h_VDCC
                {
                    LOCAL ci_in_0, co_in_0, z_in_0
                    ci_in_0 = cai_CR
                    co_in_0 = cao_CR
                    z_in_0 = 2
                    nernst_in_0 = 1000*R*(celsius+273.15)/(z_in_0*FARADAY)*log(co_in_0/ci_in_0)
                }
                Eca_syn = FARADAY*nernst_in_0
                ica_VDCC = gca_VDCC*(v-Eca_syn)
                vsyn = v
                i = i_AMPA+i_NMDA+ica_VDCC
            }
        )";
        THEN("Add 1 CONDUCTANCE hint using new var") {
            auto result = run_sympy_conductance_visitor(nmodl_text);
            REQUIRE(result == breakpoint_to_nmodl(breakpoint_text));
        }
    }
}


//=============================================================================
// to_nmodl with excluding set of node types
//=============================================================================

SCENARIO("Sympy specific AST to NMODL conversion") {
    GIVEN("NMODL block with unit usage") {
        std::string nmodl = R"(
            BREAKPOINT {
                Pf_NMDA  =  (1/1.38) * 120 (mM) * 0.6
                VDCC = gca_bar_VDCC * 4(um2)*PI*3(1/um3)
                gca_bar_VDCC = 0.0372 (nS/um2)
            }
        )";

        std::string expected = R"(
            BREAKPOINT {
                Pf_NMDA = (1/1.38)*120*0.6
                VDCC = gca_bar_VDCC*4*PI*3
                gca_bar_VDCC = 0.0372
            }
        )";

        THEN("to_nmodl can ignore all units") {
            auto input = reindent_text(nmodl);
            NmodlDriver driver;
            auto ast = driver.parse_string(input);
            auto result = to_nmodl(ast.get(), {AstNodeType::UNIT});
            REQUIRE(result == reindent_text(expected));
        }
    }
}


//=============================================================================
// Constant folding tests
//=============================================================================

std::string run_constant_folding_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    SymtabVisitor().visit_program(ast.get());
    ConstantFolderVisitor().visit_program(ast.get());

    std::stringstream stream;
    NmodlPrintVisitor(stream).visit_program(ast.get());
    return stream.str();
}

TEST_CASE("Constant Folding Visitor") {
    SECTION("Perform Successful Constant Folding") {
        GIVEN("Simple integer expression") {
            std::string nmodl_text = R"(
                PROCEDURE dummy() {
                    a = 1 + 2
                }
            )";
            std::string expected_text = R"(
                PROCEDURE dummy() {
                    a = 3
                }
            )";
            THEN("successfully folds") {
                auto result = run_constant_folding_visitor(nmodl_text);
                REQUIRE(reindent_text(result) == reindent_text(expected_text));
            }
        }

        GIVEN("Simple double expression") {
            std::string nmodl_text = R"(
                PROCEDURE dummy() {
                    a = 1.1 + 2e-10
                }
            )";
            std::string expected_text = R"(
                PROCEDURE dummy() {
                    a = 1.1000000002
                }
            )";
            THEN("successfully folds") {
                auto result = run_constant_folding_visitor(nmodl_text);
                REQUIRE(reindent_text(result) == reindent_text(expected_text));
            }
        }

        GIVEN("Complex expression") {
            std::string nmodl_text = R"(
                PROCEDURE dummy() {
                    a = 1 + (2) + (2 / 2) + (((1+((2)))))
                }
            )";
            std::string expected_text = R"(
                PROCEDURE dummy() {
                    a = 7
                }
            )";
            THEN("successfully folds") {
                auto result = run_constant_folding_visitor(nmodl_text);
                REQUIRE(reindent_text(result) == reindent_text(expected_text));
            }
        }

        GIVEN("Integer expression with define statement") {
            std::string nmodl_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = N + (2*N) + (N / 2) + (((1+((N)))))
                    FROM i = 0 TO N-2 {
                    }
                }
            )";
            std::string expected_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = 46
                    FROM i = 0 TO 8 {
                    }
                }
            )";
            THEN("successfully folds") {
                auto result = run_constant_folding_visitor(nmodl_text);
                REQUIRE(reindent_text(result) == reindent_text(expected_text));
            }
        }

        GIVEN("Only fold part of the statement") {
            std::string nmodl_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = N + 2.0 + b
                    c = a + d
                    d = 2^3
                    e = 2 || 3
                }
            )";
            std::string expected_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = 12+b
                    c = a+d
                    d = 2^3
                    e = 2 || 3
                }
            )";
            THEN("successfully folds and keep other statements untouched") {
                auto result = run_constant_folding_visitor(nmodl_text);
                REQUIRE(reindent_text(result) == reindent_text(expected_text));
            }
        }

        GIVEN("Don't remove parentheses if not simplifying") {
            std::string nmodl_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = ((N+1)+5)*(c+1+N)/(b - 2)
                }
            )";
            std::string expected_text = R"(
                DEFINE N 10

                PROCEDURE dummy() {
                    a = 16*(c+1+10)/(b-2)
                }
            )";
            THEN("successfully folds and keep other statements untouched") {
                auto result = run_constant_folding_visitor(nmodl_text);
                REQUIRE(reindent_text(result) == reindent_text(expected_text));
            }
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
        std::string expected_text = R"(
            LINEAR lin {
                x = (4*pow(a, 2)*pow(b, 2)*(-c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(4*c-1.3)+(1*b+4*c)*(5.343*a*c+1.43543))-(5.343*a*(1*b+4*c)-4*c*(5.343*a+b*(-1*a+0.842*pow(b, 2))))*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))/((1*b+4*c)*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
                y = (1*pow(a, 2)*pow(b, 2)*c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(4*c-1.3)-1*pow(a, 2)*pow(b, 2)*(1*b+4*c)*(5.343*a*c+1.43543)-c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))/(c*(1*b+4*c)*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
                z = pow(a, 2)*b*(c*(5.343*a+b*(-1*a+0.842*pow(b, 2)))*(4*c-1.3)-(1*b+4*c)*(5.343*a*c+1.43543))/(c*(pow(a, 2)*pow(b, 2)*(4*c-1.3)+0.1*b+0.4*c))
            })";
        THEN("solve analytically") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
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

SCENARIO("NONLINEAR solve block (SympySolver Visitor)", "[sympy][nonlinear]") {
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
                    X[0] = x
                }{
                    F[0] = -X[0]+5
                    J[0] = -1
                }{
                    x = X[0]
                }{
                }
            })";
        THEN("return F & J for newton solver") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::NON_LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
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
        THEN("return F & J for newton solver") {
            auto result =
                run_sympy_solver_visitor(nmodl_text, false, false, AstNodeType::NON_LINEAR_BLOCK);
            REQUIRE(reindent_text(result[0]) == reindent_text(expected_text));
        }
    }
}

//=============================================================================
// Loop unroll tests
//=============================================================================

std::string run_loop_unroll_visitor(const std::string& text) {
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    SymtabVisitor().visit_program(ast.get());
    ConstantFolderVisitor().visit_program(ast.get());
    LoopUnrollVisitor().visit_program(ast.get());
    ConstantFolderVisitor().visit_program(ast.get());
    return to_nmodl(ast.get(), {AstNodeType::DEFINE});
}

TEST_CASE("Loop Unroll visitor") {
    SECTION("Successful unrolls with constant folding") {
        GIVEN("A loop with known iteration space") {
            std::string input_nmodl = R"(
            DEFINE N 2
            PROCEDURE rates() {
                LOCAL x[N]
                FROM i=0 TO N {
                    x[i] = x[i] + 11
                }
                FROM i=(0+(0+1)) TO (N+2-1) {
                    x[(i+0)] = x[i+1] + 11
                }
            }
            KINETIC state {
                FROM i=1 TO N+1 {
                    ~ ca[i] <-> ca[i+1] (DFree*frat[i+1]*1(um), DFree*frat[i+1]*1(um))
                }
            }
        )";
            std::string output_nmodl = R"(
            PROCEDURE rates() {
                LOCAL x[N]
                {
                    x[0] = x[0]+11
                    x[1] = x[1]+11
                    x[2] = x[2]+11
                }
                {
                    x[1] = x[2]+11
                    x[2] = x[3]+11
                    x[3] = x[4]+11
                }
            }

            KINETIC state {
                {
                    ~ ca[1] <-> ca[2] (DFree*frat[2]*1(um), DFree*frat[2]*1(um))
                    ~ ca[2] <-> ca[3] (DFree*frat[3]*1(um), DFree*frat[3]*1(um))
                    ~ ca[3] <-> ca[4] (DFree*frat[4]*1(um), DFree*frat[4]*1(um))
                }
            }
        )";
            THEN("Loop body gets correctly unrolled") {
                auto result = run_loop_unroll_visitor(input_nmodl);
                REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
            }
        }

        GIVEN("A nested loop") {
            std::string input_nmodl = R"(
            DEFINE N 1
            PROCEDURE rates() {
                LOCAL x[N]
                FROM i=0 TO N {
                    FROM j=1 TO N+1 {
                        x[i] = x[i+j] + 1
                    }
                }
            }
        )";
            std::string output_nmodl = R"(
            PROCEDURE rates() {
                LOCAL x[N]
                {
                    {
                        x[0] = x[1]+1
                        x[0] = x[2]+1
                    }
                    {
                        x[1] = x[2]+1
                        x[1] = x[3]+1
                    }
                }
            }
        )";
            THEN("Loop get unrolled recursively") {
                auto result = run_loop_unroll_visitor(input_nmodl);
                REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
            }
        }


        GIVEN("Loop with verbatim and unknown iteration space") {
            std::string input_nmodl = R"(
            DEFINE N 1
            PROCEDURE rates() {
                LOCAL x[N]
                FROM i=((0+0)) TO (((N+0))) {
                    FROM j=1 TO k {
                        x[i] = x[i+k] + 1
                    }
                }
                FROM i=0 TO N {
                    VERBATIM ENDVERBATIM
                }
            }
        )";
            std::string output_nmodl = R"(
            PROCEDURE rates() {
                LOCAL x[N]
                {
                    FROM j = 1 TO k {
                        x[0] = x[0+k]+1
                    }
                    FROM j = 1 TO k {
                        x[1] = x[1+k]+1
                    }
                }
                FROM i = 0 TO N {
                    VERBATIM ENDVERBATIM
                }
            }
        )";
            THEN("Only some loops get unrolled") {
                auto result = run_loop_unroll_visitor(input_nmodl);
                REQUIRE(reindent_text(output_nmodl) == reindent_text(result));
            }
        }
    }
}

//=============================================================================
// STEADYSTATE visitor tests
//=============================================================================
std::vector<std::string> run_steadystate_visitor(
    const std::string& text,
    std::vector<AstNodeType> ret_nodetypes = {AstNodeType::SOLVE_BLOCK,
                                              AstNodeType::DERIVATIVE_BLOCK}) {
    std::vector<std::string> results;
    // construct AST from text
    NmodlDriver driver;
    auto ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor().visit_program(ast.get());

    // unroll loops and fold constants
    ConstantFolderVisitor().visit_program(ast.get());
    LoopUnrollVisitor().visit_program(ast.get());
    ConstantFolderVisitor().visit_program(ast.get());
    SymtabVisitor().visit_program(ast.get());

    // Run kinetic block visitor first, so any kinetic blocks
    // are converted into derivative blocks
    KineticBlockVisitor().visit_program(ast.get());
    SymtabVisitor().visit_program(ast.get());

    // run SteadystateVisitor on AST
    SteadystateVisitor().visit_program(ast.get());

    // run lookup visitor to extract results from AST
    auto res = AstLookupVisitor().lookup(ast.get(), ret_nodetypes);
    for (const auto& r: res) {
        results.push_back(to_nmodl(r.get()));
    }
    return results;
}

SCENARIO("SteadystateSolver visitor", "[steadystate]") {
    GIVEN("STEADYSTATE sparse solve") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states STEADYSTATE sparse
            }
            DERIVATIVE states {
                m' = m + h
            }
        )";
        std::string expected_text1 = R"(
            DERIVATIVE states {
                m' = m+h
            })";
        std::string expected_text2 = R"(
            DERIVATIVE states_steadystate {
                LOCAL dt_saved_value
                dt_saved_value = dt
                dt = 1000000000
                m' = m+h
                dt = dt_saved_value
            })";
        THEN("Construct DERIVATIVE block with steadystate solution & update SOLVE statement") {
            auto result = run_steadystate_visitor(nmodl_text);
            REQUIRE(result.size() == 3);
            REQUIRE(result[0] == "SOLVE states_steadystate METHOD sparse");
            REQUIRE(reindent_text(result[1]) == reindent_text(expected_text1));
            REQUIRE(reindent_text(result[2]) == reindent_text(expected_text2));
        }
    }
    GIVEN("STEADYSTATE derivimplicit solve") {
        std::string nmodl_text = R"(
            BREAKPOINT  {
                SOLVE states STEADYSTATE derivimplicit
            }
            DERIVATIVE states {
                m' = m + h
            }
        )";
        std::string expected_text1 = R"(
            DERIVATIVE states {
                m' = m+h
            })";
        std::string expected_text2 = R"(
            DERIVATIVE states_steadystate {
                LOCAL dt_saved_value
                dt_saved_value = dt
                dt = 1e-09
                m' = m+h
                dt = dt_saved_value
            })";
        THEN("Construct DERIVATIVE block with steadystate solution & update SOLVE statement") {
            auto result = run_steadystate_visitor(nmodl_text);
            REQUIRE(result.size() == 3);
            REQUIRE(result[0] == "SOLVE states_steadystate METHOD derivimplicit");
            REQUIRE(reindent_text(result[1]) == reindent_text(expected_text1));
            REQUIRE(reindent_text(result[2]) == reindent_text(expected_text2));
        }
    }
    GIVEN("two STEADYSTATE solves") {
        std::string nmodl_text = R"(
            STATE {
                Z[3]
                x
            }
            BREAKPOINT  {
                SOLVE states0 STEADYSTATE derivimplicit
                SOLVE states1 STEADYSTATE sparse
            }
            DERIVATIVE states0 {
                Z'[0] = Z[1] - Z[2]
                Z'[1] = Z[0] + 2*Z[2]
                Z'[2] = Z[0]*Z[0] - 3.10
            }
            DERIVATIVE states1 {
                x' = x + c
            }
        )";
        std::string expected_text1 = R"(
            DERIVATIVE states0 {
                Z'[0] = Z[1]-Z[2]
                Z'[1] = Z[0]+2*Z[2]
                Z'[2] = Z[0]*Z[0]-3.1
            })";
        std::string expected_text2 = R"(
            DERIVATIVE states1 {
                x' = x+c
            })";
        std::string expected_text3 = R"(
            DERIVATIVE states0_steadystate {
                LOCAL dt_saved_value
                dt_saved_value = dt
                dt = 1e-09
                Z'[0] = Z[1]-Z[2]
                Z'[1] = Z[0]+2*Z[2]
                Z'[2] = Z[0]*Z[0]-3.1
                dt = dt_saved_value
            })";
        std::string expected_text4 = R"(
            DERIVATIVE states1_steadystate {
                LOCAL dt_saved_value
                dt_saved_value = dt
                dt = 1000000000
                x' = x+c
                dt = dt_saved_value
            })";
        THEN("Construct DERIVATIVE blocks with steadystate solution & update SOLVE statements") {
            auto result = run_steadystate_visitor(nmodl_text);
            REQUIRE(result.size() == 6);
            REQUIRE(result[0] == "SOLVE states0_steadystate METHOD derivimplicit");
            REQUIRE(result[1] == "SOLVE states1_steadystate METHOD sparse");
            REQUIRE(reindent_text(result[2]) == reindent_text(expected_text1));
            REQUIRE(reindent_text(result[3]) == reindent_text(expected_text2));
            REQUIRE(reindent_text(result[4]) == reindent_text(expected_text3));
            REQUIRE(reindent_text(result[5]) == reindent_text(expected_text4));
        }
    }
}
