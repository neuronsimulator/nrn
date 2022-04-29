/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

#include "ast/program.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;

using nmodl::parser::NmodlDriver;
using symtab::syminfo::NmodlType;


//=============================================================================
// Symtab and Perf visitor tests
//=============================================================================

SCENARIO("Symbol table generation with Perf stat visitor", "[visitor][performance]") {
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
        const auto& ast = driver.parse_string(nmodl_text);

        WHEN("Symbol table generator pass runs") {
            SymtabVisitor v;
            v.visit_program(*ast);
            auto symtab = ast->get_model_symbol_table();

            THEN("Can lookup for defined variables") {
                auto symbol = symtab->lookup("m");
                REQUIRE(symbol->has_any_property(NmodlType::assigned_definition));
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
                v.visit_program(*ast);

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
                REQUIRE_THROWS_WITH(v.visit_program(*ast), Catch::Contains("table not setup"));
            }
        }
    }
}
