/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_neuron_cpp_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/function_callpath_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using Catch::Matchers::ContainsSubstring;

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;
using nmodl::test_utils::reindent_text;
using symtab::syminfo::NmodlType;

/// Helper for creating C codegen visitor
std::shared_ptr<CodegenNeuronCppVisitor> create_neuron_cpp_visitor(
    const std::shared_ptr<ast::Program>& ast,
    const std::string& /* text */,
    std::stringstream& ss) {
    /// construct symbol table
    SymtabVisitor().visit_program(*ast);

    /// run all necessary pass
    InlineVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    SolveBlockVisitor().visit_program(*ast);
    FunctionCallpathVisitor().visit_program(*ast);

    /// create C code generation visitor
    auto cv = std::make_shared<CodegenNeuronCppVisitor>("_test", ss, "double", false);
    cv->setup(*ast);
    return cv;
}


/// print entire code
std::string get_neuron_cpp_code(const std::string& nmodl_text) {
    const auto& ast = NmodlDriver().parse_string(nmodl_text);
    std::stringstream ss;
    auto cvisitor = create_neuron_cpp_visitor(ast, nmodl_text, ss);
    cvisitor->visit_program(*ast);
    return ss.str();
}


SCENARIO("Check NEURON codegen for simple MOD file", "[codegen][neuron_boilerplate]") {
    GIVEN("A simple mod file with RANGE, ARRAY and ION variables") {
        std::string const nmodl_text = R"(
            TITLE unit test based on passive membrane channel

            UNITS {
                (mV) = (millivolt)
                (mA) = (milliamp)
                (S) = (siemens)
            }

            NEURON {
                SUFFIX pas_test
                USEION na READ ena WRITE ina
                NONSPECIFIC_CURRENT i
                RANGE g, e
                RANGE ar
            }

            PARAMETER {
                g = .001	(S/cm2)	<0,1e9>
                e = -70	(mV)
            }

            ASSIGNED {
                v (mV)
                i (mA/cm2)
                ena (mV)
                ina (mA/cm2)
                ar[2]
            }

            INITIAL {
                ar[0] = 1
            }

            BREAKPOINT {
                SOLVE states METHOD cnexp
                i = g*(v - e)
                ina = g*(v - ena)
            }

            STATE {
                s
            }

            DERIVATIVE states {
                s' = ar[0]
            }
        )";
        auto const reindent_and_trim_text = [](const auto& text) {
            return reindent_text(stringutils::trim(text));
        };
        auto const generated = reindent_and_trim_text(get_neuron_cpp_code(nmodl_text));
        THEN("Correct includes are printed") {
            std::string expected_includes = R"(#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "mech_api.h"
#include "neuron/cache/mechanism_range.hpp"
#include "nrniv_mf.h"
#include "section_fwd.hpp")";

            REQUIRE_THAT(generated, ContainsSubstring(reindent_and_trim_text(expected_includes)));
        }
        THEN("Correct number of variables are printed") {
            std::string expected_num_variables =
                R"(static constexpr auto number_of_datum_variables = 3;
static constexpr auto number_of_floating_point_variables = 10;)";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_num_variables)));
        }
        THEN("Correct using-directives are printed ") {
            std::string expected_using_directives = R"(namespace {
template <typename T>
using _nrn_mechanism_std_vector = std::vector<T>;
using _nrn_model_sorted_token = neuron::model_sorted_token;
using _nrn_mechanism_cache_range = neuron::cache::MechanismRange<number_of_floating_point_variables, number_of_datum_variables>;
using _nrn_mechanism_cache_instance = neuron::cache::MechanismInstance<number_of_floating_point_variables, number_of_datum_variables>;
using _nrn_non_owning_id_without_container = neuron::container::non_owning_identifier_without_container;
template <typename T>
using _nrn_mechanism_field = neuron::mechanism::field<T>;
template <typename... Args>
void _nrn_mechanism_register_data_fields(Args&&... args) {
    neuron::mechanism::register_data_fields(std::forward<Args>(args)...);
}
}  // namespace)";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_using_directives)));
        }
        THEN("Correct namespace is printed") {
            std::string expected_namespace = R"(namespace neuron {)";

            REQUIRE_THAT(generated, ContainsSubstring(reindent_and_trim_text(expected_namespace)));
        }
        THEN("Correct channel information are printed") {
            std::string expected_channel_info = R"(/** channel information */
    static const char *mechanism_info[] = {
        "7.7.0",
        "pas_test",
        "g_pas_test",
        "e_pas_test",
        0,
        "i_pas_test",
        "ar_pas_test[2]",
        0,
        "s_pas_test",
        0,
        0
    };)";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_channel_info)));
        }
        THEN("Correct global variables are printed") {
            std::string expected_global_variables =
                R"(static neuron::container::field_index _slist1[1], _dlist1[1];)";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_global_variables)));
        }
        THEN("Correct pas_test_Instance") {
            std::string expected =
                R"(      struct pas_test_Instance  {
        double* g{};
        double* e{};
        double* i{};
        double* ar{};
        double* s{};
        double* ena{};
        double* ina{};
        double* Ds{};
        double* v_unused{};
        double* g_unused{};
        const double* const* ion_ena{};
        double* const* ion_ina{};
        double* const* ion_dinadv{};
        pas_test_Store* global{&pas_test_global};
    };)";

            REQUIRE_THAT(generated, ContainsSubstring(reindent_and_trim_text(expected)));
        }
        THEN("Correct HOC global variables are printed") {
            std::string expected_hoc_global_variables =
                R"(/** connect global (scalar) variables to hoc -- */
    static DoubScal hoc_scalar_double[] = {
        {nullptr, nullptr}
    };


    /** connect global (array) variables to hoc -- */
    static DoubVec hoc_vector_double[] = {
        {nullptr, nullptr, 0}
    };)";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_hoc_global_variables)));
        }
        THEN("Placeholder nrn_cur function is printed") {
            std::string expected_placeholder_nrn_cur =
                R"(void nrn_cur_pas_test(_nrn_model_sorted_token const& _sorted_token, NrnThread* _nt, Memb_list* _ml_arg, int _type) {})";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_placeholder_nrn_cur)));
        }
        THEN("Placeholder nrn_state function is printed") {
            std::string expected_placeholder_nrn_state =
                R"(void nrn_state_pas_test(_nrn_model_sorted_token const& _sorted_token, NrnThread* _nt, Memb_list* _ml_arg, int _type) {)";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_placeholder_nrn_state)));
        }
        THEN("Initialization function for slist/dlist is printed correctly") {
            std::string expected_initlists_s_var = "_slist1[0] = {4, 0};";
            std::string expected_initlists_Ds_var = "_dlist1[0] = {7, 0};";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_initlists_s_var)));
            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_initlists_Ds_var)));
        }
        THEN("Placeholder registration function is printed") {
            std::string expected_placeholder_reg = R"CODE(/** register channel with the simulator */
    extern "C" void __test_reg() {
        _initlists();

        register_mech(mechanism_info, nrn_alloc_pas_test, nrn_cur_pas_test, nrn_jacob_pas_test, nrn_state_pas_test, nrn_init_pas_test, hoc_nrnpointerindex, 1);

        mech_type = nrn_get_mechtype(mechanism_info[1]);
        _nrn_mechanism_register_data_fields(mech_type,
            _nrn_mechanism_field<double>{"g"} /* 0 */,
            _nrn_mechanism_field<double>{"e"} /* 1 */,
            _nrn_mechanism_field<double>{"i"} /* 2 */,
            _nrn_mechanism_field<double>{"ar", 2} /* 3 */,
            _nrn_mechanism_field<double>{"s"} /* 4 */,
            _nrn_mechanism_field<double>{"ena"} /* 5 */,
            _nrn_mechanism_field<double>{"ina"} /* 6 */,
            _nrn_mechanism_field<double>{"Ds"} /* 7 */,
            _nrn_mechanism_field<double>{"v_unused"} /* 8 */,
            _nrn_mechanism_field<double>{"g_unused"} /* 9 */,
            _nrn_mechanism_field<double*>{"ion_ena", "na_ion"} /* 0 */,
            _nrn_mechanism_field<double*>{"ion_ina", "na_ion"} /* 1 */,
            _nrn_mechanism_field<double*>{"ion_dinadv", "na_ion"} /* 2 */
        );

        hoc_register_prop_size(mech_type, 10, 3);
        hoc_register_dparam_semantics(mech_type, 0, "na_ion");
        hoc_register_dparam_semantics(mech_type, 1, "na_ion");
        hoc_register_dparam_semantics(mech_type, 2, "na_ion");
    })CODE";

            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_and_trim_text(expected_placeholder_reg)));
        }
    }
}


SCENARIO("Check whether PROCEDURE and FUNCTION need setdata call", "[codegen][needsetdata]") {
    GIVEN("mod file with GLOBAL and RANGE variables used in FUNC and PROC") {
        std::string input_nmodl = R"(
            NEURON {
                SUFFIX test
                RANGE x
                GLOBAL s
            }
            PARAMETER {
                s = 2
            }
            ASSIGNED {
                x
            }
            PROCEDURE a() {
                x = get_42()
            }
            FUNCTION b() {
                a()
            }
            FUNCTION get_42() {
                get_42 = 42
            }
        )";
        const auto& ast = NmodlDriver().parse_string(input_nmodl);
        std::stringstream ss;
        auto cvisitor = create_neuron_cpp_visitor(ast, input_nmodl, ss);
        cvisitor->visit_program(*ast);
        const auto symtab = ast->get_symbol_table();
        THEN("use_range_ptr_var property is added to needed FUNC and PROC") {
            auto use_range_ptr_var_funcs = symtab->get_variables_with_properties(
                NmodlType::use_range_ptr_var);
            REQUIRE(use_range_ptr_var_funcs.size() == 2);
            const auto a = symtab->lookup("a");
            REQUIRE(a->has_any_property(NmodlType::use_range_ptr_var));
            const auto b = symtab->lookup("b");
            REQUIRE(b->has_any_property(NmodlType::use_range_ptr_var));
            const auto get_42 = symtab->lookup("get_42");
            REQUIRE(!get_42->has_any_property(NmodlType::use_range_ptr_var));
        }
    }
}
