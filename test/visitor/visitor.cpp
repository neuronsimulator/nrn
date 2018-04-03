#define CATCH_CONFIG_MAIN

#include <string>

#include "catch/catch.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/verbatim_visitor.hpp"
#include "visitors/defuse_analyze_visitor.hpp"
#include "visitors/localize_visitor.hpp"
#include "visitors/verbatim_var_rename_visitor.hpp"
#include "visitors/cnexp_solve_visitor.hpp"
#include "test/utils/nmodl_constructs.h"
#include "test/utils/test_utils.hpp"

using json = nlohmann::json;

//=============================================================================
// Verbatim visitor tests
//=============================================================================

std::vector<std::string> run_verbatim_visitor(const std::string& text) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    VerbatimVisitor v;
    v.visit_program(ast.get());
    return v.verbatim_blocks();
}

TEST_CASE("Verbatim Visitor") {
    SECTION("Single Block") {
        std::string text = "VERBATIM int a; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 1);
        REQUIRE(blocks.front().compare(" int a; ") == 0);
    }

    SECTION("Multiple Blocks") {
        std::string text = "VERBATIM int a; ENDVERBATIM VERBATIM float b; ENDVERBATIM";
        auto blocks = run_verbatim_visitor(text);

        REQUIRE(blocks.size() == 2);
        REQUIRE(blocks[0].compare(" int a; ") == 0);
        REQUIRE(blocks[1].compare(" float b; ") == 0);
    }
}

//=============================================================================
// JSON visitor tests
//=============================================================================

std::string run_json_visitor(const std::string& text, bool compact = false) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    std::stringstream ss;
    JSONVisitor v(ss);

    /// if compact is true then we get compact json output
    v.compact_json(compact);

    v.visit_program(ast.get());
    return ss.str();
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

        nmodl::Driver driver;
        driver.parse_string(nmodl_text);
        auto ast = driver.ast();

        WHEN("Symbol table generator pass runs") {
            SymtabVisitor v;
            v.visit_program(ast.get());
            auto symtab = ast->get_model_symbol_table();

            using namespace symtab::details;

            THEN("Can lookup for defined variables") {
                auto symbol = symtab->lookup("m");
                REQUIRE(symbol->has_properties(NmodlInfo::dependent_def));
                REQUIRE_FALSE(symbol->has_properties(NmodlInfo::local_var));

                symbol = symtab->lookup("gNaTs2_tbar");
                REQUIRE(symbol->has_properties(NmodlInfo::param_assign));
                REQUIRE(symbol->has_properties(NmodlInfo::range_var));

                symbol = symtab->lookup("ena");
                REQUIRE(symbol->has_properties(NmodlInfo::read_ion_var));
            }
            THEN("Can lookup for defined functions") {
                auto symbol = symtab->lookup("hBetaf");
                REQUIRE(symbol->has_properties(NmodlInfo::function_block));
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
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    std::stringstream stream;
    NmodlPrintVisitor v(stream);

    v.visit_program(ast.get());
    return stream.str();
}

SCENARIO("Test for AST back to NMODL transformation") {
    for (const auto& construct : nmodl_valid_constructs) {
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
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();
    {
        for (const auto& variable : variables) {
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
                {"m", "mm"}, {"gNaTs2_tbar", "new_gNaTs2_tbar"}, {"mAlpha", "mBeta"},
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
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

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
                ENDVERBATIM
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
                ENDVERBATIM
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
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

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
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    {
        SymtabVisitor v;
        v.visit_program(ast.get());
    }

    {
        InlineVisitor v1;
        v1.visit_program(ast.get());
    }

    {
        std::vector<DUChain> chains;
        DefUseAnalyzeVisitor v(ast->get_symbol_table());

        for (auto& block : ast->blocks) {
            if (block->get_type() != ast::Type::NEURON_BLOCK) {
                chains.push_back(v.analyze(block.get(), variable));
            }
        }
        return chains;
    }
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
            R"({"DerivativeBlock":[{"value":"D"},{"value":"U"},{"value":"D"}]})";

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
            R"({"DerivativeBlock":[{"value":"U"},{"value":"D"},{"value":"U"}]})";

        THEN("Verbatim block is considered as use of the variable") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string(true) == expected_text);
            REQUIRE(chains[0].eval() == DUState::U);
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
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"value":"LD"}]},{"ELSE":[{"value":"D"}]}]}]})";

        THEN("Def-Use chains should return NONE") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::NONE);
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
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"value":"IF"},{"ELSE":[{"value":"U"},{"value":"D"}]}]}]})";

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
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"value":"D"},{"value":"U"}]},{"ELSE":[{"value":"D"}]}]}]})";

        THEN("Def-Use chains should return DEF") {
            std::string input = reindent_text(nmodl_text);
            auto chains = run_defuse_visitor(input, "tau");
            REQUIRE(chains[0].to_string() == expected_text);
            REQUIRE(chains[0].eval() == DUState::D);
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
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"value":"D"}]}]},{"value":"U"},{"value":"D"},{"CONDITIONAL_BLOCK":[{"value":"IF"},{"ELSEIF":[{"value":"D"}]}]}]})";

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
            R"({"DerivativeBlock":[{"CONDITIONAL_BLOCK":[{"IF":[{"value":"LD"}]}]},{"CONDITIONAL_BLOCK":[{"IF":[{"CONDITIONAL_BLOCK":[{"value":"IF"},{"ELSE":[{"value":"D"}]}]}]},{"ELSEIF":[{"CONDITIONAL_BLOCK":[{"IF":[{"CONDITIONAL_BLOCK":[{"value":"IF"},{"ELSE":[{"value":"U"}]}]}]}]},{"value":"D"}]}]}]})";

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
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

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
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();

    {
        SymtabVisitor v1;
        v1.visit_program(ast.get());
        CnexpSolveVisitor v2;
        v2.visit_program(ast.get());
    }

    std::stringstream stream;
    {
        NmodlPrintVisitor v(stream);
        v.visit_program(ast.get());
    }
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
                m' = (mInf-m)/mTau
                h' = (hInf-h)/hTau
            }
        )";

        THEN("ODEs don't get solved / replaced ") {
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
// Passes can run multiple times
//=============================================================================

void run_visitor_passes(const std::string& text) {
    nmodl::Driver driver;
    driver.parse_string(text);
    auto ast = driver.ast();
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
