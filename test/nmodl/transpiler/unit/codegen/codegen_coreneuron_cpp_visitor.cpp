/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_acc_visitor.hpp"
#include "codegen/codegen_coreneuron_cpp_visitor.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/implicit_argument_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/neuron_solve_visitor.hpp"
#include "visitors/perf_visitor.hpp"
#include "visitors/solve_block_visitor.hpp"
#include "visitors/sympy_solver_visitor.hpp"
#include "visitors/symtab_visitor.hpp"

using Catch::Matchers::ContainsSubstring;

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;
using nmodl::test_utils::reindent_text;

/// Helper for creating C codegen visitor
std::shared_ptr<CodegenCoreneuronCppVisitor> create_coreneuron_cpp_visitor(
    const std::shared_ptr<ast::Program>& ast,
    const std::string& /* text */,
    std::stringstream& ss) {
    /// construct symbol table
    SymtabVisitor().visit_program(*ast);

    /// run all necessary pass
    InlineVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    SolveBlockVisitor().visit_program(*ast);

    /// create C code generation visitor
    auto cv = std::make_shared<CodegenCoreneuronCppVisitor>("temp.mod", ss, "double", false);
    cv->setup(*ast);
    return cv;
}

/// Helper for creating OpenACC codegen visitor
std::shared_ptr<CodegenAccVisitor> create_acc_visitor(const std::shared_ptr<ast::Program>& ast,
                                                      const std::string& /* text */,
                                                      std::stringstream& ss) {
    /// construct symbol table
    SymtabVisitor().visit_program(*ast);

    /// run all necessary pass
    InlineVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    SolveBlockVisitor().visit_program(*ast);

    /// create C code generation visitor
    auto cv = std::make_shared<CodegenAccVisitor>("temp.mod", ss, "double", false);
    cv->setup(*ast);
    return cv;
}

/// print instance structure for testing purpose
std::string get_instance_var_setup_function(std::string& nmodl_text) {
    const auto& ast = NmodlDriver().parse_string(nmodl_text);
    std::stringstream ss;
    auto cvisitor = create_coreneuron_cpp_visitor(ast, nmodl_text, ss);
    cvisitor->print_instance_variable_setup();
    return reindent_text(ss.str());
}

/// print entire code
std::string get_coreneuron_cpp_code(const std::string& nmodl_text,
                                    const bool generate_gpu_code = false) {
    const auto& ast = NmodlDriver().parse_string(nmodl_text);
    std::stringstream ss;
    if (generate_gpu_code) {
        auto accvisitor = create_acc_visitor(ast, nmodl_text, ss);
        accvisitor->visit_program(*ast);
    } else {
        auto cvisitor = create_coreneuron_cpp_visitor(ast, nmodl_text, ss);
        cvisitor->visit_program(*ast);
    }
    return reindent_text(ss.str());
}

SCENARIO("Check instance variable definition order", "[codegen][var_order]") {
    GIVEN("cal_mig.mod: USEION variables declared as RANGE") {
        // In the below mod file, the ion variables cai and cao are also
        // declared as RANGE variables. The ordering issue was fixed in #443.
        std::string nmodl_text = R"(
            PARAMETER {
              gcalbar=.003 (mho/cm2)
              ki=.001 (mM)
              cai = 50.e-6 (mM)
              cao = 2 (mM)
            }
            NEURON {
              SUFFIX cal
              USEION ca READ cai,cao WRITE ica
              RANGE gcalbar, cai, ica, gcal, ggk
              RANGE minf, tau
            }
            STATE {
              m
            }
            ASSIGNED {
              ica (mA/cm2)
              gcal (mho/cm2)
              minf
              tau   (ms)
              ggk
            }
        )";

        THEN("ionic current variable declared as RANGE appears first") {
            std::string generated_code = R"(
                static inline void setup_instance(NrnThread* nt, Memb_list* ml) {
                    auto* const inst = static_cast<cal_Instance*>(ml->instance);
                    assert(inst);
                    assert(inst->global);
                    assert(inst->global == &cal_global);
                    assert(inst->global == ml->global_variables);
                    assert(ml->global_variables_size == sizeof(cal_Store));
                    int pnodecount = ml->_nodecount_padded;
                    Datum* indexes = ml->pdata;
                    inst->gcalbar = ml->data+0*pnodecount;
                    inst->ica = ml->data+1*pnodecount;
                    inst->gcal = ml->data+2*pnodecount;
                    inst->minf = ml->data+3*pnodecount;
                    inst->tau = ml->data+4*pnodecount;
                    inst->ggk = ml->data+5*pnodecount;
                    inst->m = ml->data+6*pnodecount;
                    inst->cai = ml->data+7*pnodecount;
                    inst->cao = ml->data+8*pnodecount;
                    inst->Dm = ml->data+9*pnodecount;
                    inst->v_unused = ml->data+10*pnodecount;
                    inst->ion_cai = nt->_data;
                    inst->ion_cao = nt->_data;
                    inst->ion_ica = nt->_data;
                    inst->ion_dicadv = nt->_data;
                }
            )";
            auto const expected = reindent_text(generated_code);
            auto const result = get_instance_var_setup_function(nmodl_text);
            REQUIRE_THAT(result, ContainsSubstring(expected));
        }
    }

    // In the below mod file, the `cao` is defined first in the PARAMETER
    // block but it appears after cai in the USEION statement. As per NEURON
    // implementation, variables should appear in the order of USEION
    // statements i.e. ion_cai should come before ion_cao. This was a bug
    // and it has been fixed in #697.
    GIVEN("LcaMig.mod: mod file from reduced_dentate model") {
        std::string nmodl_text = R"(
            PARAMETER {
              ki = .001(mM)
              cao(mM)
              tfa = 1
            }
            NEURON {
              SUFFIX lca
              USEION ca READ cai, cao VALENCE 2
              RANGE cai, ilca, elca
            }
            STATE {
              m
            }
        )";

        THEN("Ion variables are defined in the order of USEION") {
            std::string generated_code = R"(
                static inline void setup_instance(NrnThread* nt, Memb_list* ml) {
                    auto* const inst = static_cast<lca_Instance*>(ml->instance);
                    assert(inst);
                    assert(inst->global);
                    assert(inst->global == &lca_global);
                    assert(inst->global == ml->global_variables);
                    assert(ml->global_variables_size == sizeof(lca_Store));
                    int pnodecount = ml->_nodecount_padded;
                    Datum* indexes = ml->pdata;
                    inst->m = ml->data+0*pnodecount;
                    inst->cai = ml->data+1*pnodecount;
                    inst->cao = ml->data+2*pnodecount;
                    inst->Dm = ml->data+3*pnodecount;
                    inst->v_unused = ml->data+4*pnodecount;
                    inst->ion_cai = nt->_data;
                    inst->ion_cao = nt->_data;
                }
            )";

            auto const expected = reindent_text(generated_code);
            auto const result = get_instance_var_setup_function(nmodl_text);
            REQUIRE_THAT(result, ContainsSubstring(expected));
        }
    }

    // In the below mod file, ion variables ncai and lcai are declared
    // as state variables as well as range variables. The issue about
    // this mod file ordering was fixed in #443.
    // We also use example from #888 where mod file declared `g` as a
    // conductance variable in a non-threadsafe mod file and resulting
    // into duplicate definition of `g` in the instance structure.
    GIVEN("ccanl.mod: mod file from reduced_dentate model") {
        std::string nmodl_text = R"(
            NEURON {
              SUFFIX ccanl
              USEION nca READ ncai, inca, enca WRITE enca, ncai VALENCE 2
              USEION lca READ lcai, ilca, elca WRITE elca, lcai VALENCE 2
              USEION k WRITE ko
              RANGE caiinf, catau, cai, ncai, lcai, eca, elca, enca, g, ko
            }
            UNITS {
              FARADAY = 96520(coul)
              R = 8.3134(joule / degC)
            }
            PARAMETER {
              depth = 200(nm): assume volume = area * depth
              catau = 9(ms)
              caiinf = 50.e-6(mM)
              cao = 2(mM)
            }
            ASSIGNED {
              celsius(degC)
              ica(mA / cm2)
              inca(mA / cm2)
              ilca(mA / cm2)
              cai(mM)
              enca(mV)
              elca(mV)
              eca(mV)
              g(S/cm2)
              ko(mA / cm2)
            }
            STATE {
              ncai(mM)
              lcai(mM)
            }
            BREAKPOINT {}
            DISCRETE seq {}
        )";

        THEN("Ion variables are defined in the order of USEION") {
            std::string generated_code = R"(
                static inline void setup_instance(NrnThread* nt, Memb_list* ml) {
                    auto* const inst = static_cast<ccanl_Instance*>(ml->instance);
                    assert(inst);
                    assert(inst->global);
                    assert(inst->global == &ccanl_global);
                    assert(inst->global == ml->global_variables);
                    assert(ml->global_variables_size == sizeof(ccanl_Store));
                    int pnodecount = ml->_nodecount_padded;
                    Datum* indexes = ml->pdata;
                    inst->catau = ml->data+0*pnodecount;
                    inst->caiinf = ml->data+1*pnodecount;
                    inst->cai = ml->data+2*pnodecount;
                    inst->eca = ml->data+3*pnodecount;
                    inst->g = ml->data+4*pnodecount;
                    inst->ica = ml->data+5*pnodecount;
                    inst->inca = ml->data+6*pnodecount;
                    inst->ilca = ml->data+7*pnodecount;
                    inst->enca = ml->data+8*pnodecount;
                    inst->elca = ml->data+9*pnodecount;
                    inst->ko = ml->data+10*pnodecount;
                    inst->ncai = ml->data+11*pnodecount;
                    inst->Dncai = ml->data+12*pnodecount;
                    inst->lcai = ml->data+13*pnodecount;
                    inst->Dlcai = ml->data+14*pnodecount;
                    inst->ion_ncai = nt->_data;
                    inst->ion_inca = nt->_data;
                    inst->ion_enca = nt->_data;
                    inst->ion_ncao = nt->_data;
                    inst->ion_nca_erev = nt->_data;
                    inst->style_nca = ml->pdata;
                    inst->ion_lcai = nt->_data;
                    inst->ion_ilca = nt->_data;
                    inst->ion_elca = nt->_data;
                    inst->ion_lcao = nt->_data;
                    inst->ion_lca_erev = nt->_data;
                    inst->style_lca = ml->pdata;
                    inst->ion_ki = nt->_data;
                    inst->ion_ko = nt->_data;
                    inst->ion_k_erev = nt->_data;
                    inst->style_k = ml->pdata;
                }
            )";

            auto const expected = reindent_text(generated_code);
            auto const result = get_instance_var_setup_function(nmodl_text);
            REQUIRE_THAT(result, ContainsSubstring(expected));
        }
    }
}

std::string get_instance_structure(std::string nmodl_text) {
    // parse mod file & print mechanism structure
    auto const ast = NmodlDriver{}.parse_string(nmodl_text);
    // add implicit arguments
    ImplicitArgumentVisitor{}.visit_program(*ast);
    // update the symbol table for PerfVisitor
    SymtabVisitor{}.visit_program(*ast);
    // we need the read/write counts so the codegen knows whether or not
    // global variables are used
    PerfVisitor{}.visit_program(*ast);
    // setup codegen
    std::stringstream ss{};
    CodegenCoreneuronCppVisitor cv{"temp.mod", ss, "double", false};
    cv.setup(*ast);
    cv.print_mechanism_range_var_structure(true);
    return ss.str();
}

SCENARIO("Check parameter constness with VERBATIM block",
         "[codegen][verbatim_variable_constness]") {
    GIVEN("A mod file containing parameter range variables that are updated in VERBATIM block") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX IntervalFire
                RANGE invl, burst_start
            }
            PARAMETER {
                invl = 10 (ms) <1e-9,1e9>
                burst_start = 0 (ms)
            }
            INITIAL {
                LOCAL temp
                : as invl is used in verbatim, it shouldn't be treated as const
                VERBATIM
                invl = 11
                ENDVERBATIM
                : burst_start time is read-only and hence can be const
                temp = burst_start
            }
        )";

        THEN("Variable used in VERBATIM shouldn't be marked as const") {
            auto const generated = get_instance_structure(nmodl_text);
            std::string expected_code = R"(
                /** all mechanism instance variables and global variables */
                struct IntervalFire_Instance  {
                    double* invl{};
                    const double* burst_start{};
                    double* v_unused{};
                    IntervalFire_Store* global{&IntervalFire_global};
                };
            )";
            REQUIRE(reindent_text(generated) == reindent_text(expected_code));
        }
    }
}

SCENARIO("Check NEURON globals are added to the instance struct on demand",
         "[codegen][global_variables]") {
    GIVEN("A MOD file that uses global variables") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX GlobalTest
                RANGE temperature
            }
            INITIAL {
                temperature = celsius + secondorder + pi
            }
        )";
        THEN("The instance struct should contain these variables") {
            auto const generated = get_instance_structure(nmodl_text);
            REQUIRE_THAT(generated, ContainsSubstring("double* celsius{&coreneuron::celsius}"));
            REQUIRE_THAT(generated, ContainsSubstring("double* pi{&coreneuron::pi}"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("int* secondorder{&coreneuron::secondorder}"));
        }
    }
    GIVEN("A MOD file that implicitly uses global variables") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX ImplicitTest
            }
            INITIAL {
                LOCAL x
                x = nrn_ghk(1, 2, 3, 4)
            }
        )";
        THEN("The instance struct should contain celsius for the implicit 5th argument") {
            auto const generated = get_instance_structure(nmodl_text);
            REQUIRE_THAT(generated, ContainsSubstring("celsius"));
        }
    }
    GIVEN("A MOD file that does not touch celsius, secondorder or pi") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX GlobalTest
            }
        )";
        THEN("The instance struct should not contain those variables") {
            auto const generated = get_instance_structure(nmodl_text);
            REQUIRE_THAT(generated, !ContainsSubstring("celsius"));
            REQUIRE_THAT(generated, !ContainsSubstring("pi"));
            REQUIRE_THAT(generated, !ContainsSubstring("secondorder"));
        }
    }
}

SCENARIO("Check code generation for TABLE statements", "[codegen][array_variables]") {
    GIVEN("A MOD file that uses global and array variables in TABLE") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX glia_Cav2_3
                RANGE inf
                GLOBAL tau
            }

            STATE { m }

            PARAMETER {
                tau = 1
            }

            ASSIGNED {
                inf[2]
            }

            BREAKPOINT {
                 SOLVE states METHOD cnexp
            }

            DERIVATIVE states {
                mhn(v)
                m' =  (inf[0] - m)/tau
            }

            PROCEDURE mhn(v (mV)) {
                TABLE inf, tau DEPEND celsius FROM -100 TO 100 WITH 200
                FROM i=0 TO 1 {
                    inf[i] = v + tau
                }
            }
        )";
        THEN("Array and global variables should be correctly generated") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            REQUIRE_THAT(generated, ContainsSubstring("double t_inf[2][201]{};"));
            REQUIRE_THAT(generated, ContainsSubstring("double t_tau[201]{};"));

            REQUIRE_THAT(generated,
                         ContainsSubstring("inst->global->t_inf[0][i] = (inst->inf+id*2)[0];"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("inst->global->t_inf[1][i] = (inst->inf+id*2)[1];"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("inst->global->t_tau[i] = inst->global->tau;"));

            REQUIRE_THAT(generated,
                         ContainsSubstring("(inst->inf+id*2)[0] = inst->global->t_inf[0][index];"));

            REQUIRE_THAT(generated,
                         ContainsSubstring("(inst->inf+id*2)[0] = inst->global->t_inf[0][i]"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("(inst->inf+id*2)[1] = inst->global->t_inf[1][i]"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("inst->global->tau = inst->global->t_tau[i]"));
        }
    }
    GIVEN("A MOD file with two table statements") {
        std::string const nmodl_text = R"(
            NEURON {
                RANGE inf, tau
            }
            PROCEDURE foo(v) {
                TABLE inf FROM 1 TO 3 WITH 100
                FROM i=0 TO 1 {
                }
                TABLE tau FROM 1 TO 3 WITH 100
                FROM i=0 TO 1 {
                }
            }
        )";
        THEN("It should throw") {
            REQUIRE_THROWS(get_coreneuron_cpp_code(nmodl_text));
        }
    }
}

SCENARIO("Check that BEFORE/AFTER block are well generated", "[codegen][before/after]") {
    GIVEN("A mod file full of BEFORE/AFTER of all kinds") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX ba1
            }
            BEFORE BREAKPOINT {
                init_before_breakpoint()
                PROTECT inc = inc + 1
            }
            AFTER SOLVE {
                MUTEXLOCK
                init_after_solve()
                inc = 0
                MUTEXUNLOCK
            }
            BEFORE INITIAL {
                init_before_initial()
                inc = 0
            }
            AFTER INITIAL {
                init_after_initial()
                inc = 0
            }
            BEFORE STEP {
                init_before_step()
                inc = 0
            }
        )";
        THEN("They should be well registered") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            // BEFORE BREAKPOINT
            {
                REQUIRE_THAT(generated,
                             ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_0_ba1, "
                                               "BAType::Before + BAType::Breakpoint);"));
                // in case of PROTECT, there should not be simd or ivdep pragma
                std::string generated_code = R"(

        for (int id = 0; id < nodecount; id++) {
            int node_id = node_index[id];
            double v = voltage[node_id];
            #if NRN_PRCELLSTATE
            inst->v_unused[id] = v;
            #endif
            {
                init_before_breakpoint();
                #pragma omp atomic update
                inc = inc + 1.0;
            }
        })";
                auto const expected = generated_code;
                REQUIRE_THAT(generated, ContainsSubstring(expected));
            }
            // AFTER SOLVE
            {
                REQUIRE_THAT(generated,
                             ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_1_ba1, "
                                               "BAType::After + BAType::Solve);"));
                // in case of MUTEXLOCK/MUTEXUNLOCK, there should not be simd or ivdep pragma
                std::string generated_code = R"(

        for (int id = 0; id < nodecount; id++) {
            int node_id = node_index[id];
            double v = voltage[node_id];
            #if NRN_PRCELLSTATE
            inst->v_unused[id] = v;
            #endif
            {
                #pragma omp critical (ba1)
                {
                    init_after_solve();
                    inc = 0.0;
                }
            }
        })";
                auto const expected = generated_code;
                REQUIRE_THAT(generated, ContainsSubstring(expected));
            }
            // BEFORE INITIAL
            {
                REQUIRE_THAT(generated,
                             ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_2_ba1, "
                                               "BAType::Before + BAType::Initial);"));
                std::string generated_code = R"(
        #pragma omp simd
        #pragma ivdep
        for (int id = 0; id < nodecount; id++) {
            int node_id = node_index[id];
            double v = voltage[node_id];
            #if NRN_PRCELLSTATE
            inst->v_unused[id] = v;
            #endif
            {
                init_before_initial();
                inc = 0.0;
            }
        })";
                auto const expected = generated_code;
                REQUIRE_THAT(generated, ContainsSubstring(expected));
            }
            // AFTER INITIAL
            {
                REQUIRE_THAT(generated,
                             ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_3_ba1, "
                                               "BAType::After + BAType::Initial);"));
                std::string generated_code = R"(
        #pragma omp simd
        #pragma ivdep
        for (int id = 0; id < nodecount; id++) {
            int node_id = node_index[id];
            double v = voltage[node_id];
            #if NRN_PRCELLSTATE
            inst->v_unused[id] = v;
            #endif
            {
                init_after_initial();
                inc = 0.0;
            }
        })";
                auto const expected = generated_code;
                REQUIRE_THAT(generated, ContainsSubstring(expected));
            }
            // BEFORE STEP
            {
                REQUIRE_THAT(generated,
                             ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_4_ba1, "
                                               "BAType::Before + BAType::Step);"));
                std::string generated_code = R"(
        #pragma omp simd
        #pragma ivdep
        for (int id = 0; id < nodecount; id++) {
            int node_id = node_index[id];
            double v = voltage[node_id];
            #if NRN_PRCELLSTATE
            inst->v_unused[id] = v;
            #endif
            {
                init_before_step();
                inc = 0.0;
            }
        })";
                auto const expected = generated_code;
                REQUIRE_THAT(generated, ContainsSubstring(expected));
            }
        }
    }

    GIVEN("A mod file with several time same BEFORE or AFTER block") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX ba1
            }
            BEFORE STEP {}
            AFTER SOLVE {}
            BEFORE STEP {}
            AFTER SOLVE {}
        )";
        THEN("They should be all registered") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            REQUIRE_THAT(generated,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_0_ba1, "
                                           "BAType::Before + BAType::Step);"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_1_ba1, "
                                           "BAType::After + BAType::Solve);"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_2_ba1, "
                                           "BAType::Before + BAType::Step);"));
            REQUIRE_THAT(generated,
                         ContainsSubstring("hoc_reg_ba(mech_type, nrn_before_after_3_ba1, "
                                           "BAType::After + BAType::Solve);"));
        }
    }
}

SCENARIO("Check CONSTANT variables are added to global variable structure",
         "[codegen][global_variables]") {
    GIVEN("A MOD file that use CONSTANT variables") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX CONST
                GLOBAL zGateS1
            }
            PARAMETER {
                zGateS1 = 1.2 (1)
            }
            CONSTANT {
                e0 = 1.60217646e-19 (coulombs)
                kB = 1.3806505e-23 (joule/kelvin)
                q10Fluo = 1.67 (1)
            }
        )";
        THEN("The global struct should contain these variables") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string expected_code = R"(
    struct CONST_Store {
        int reset{};
        int mech_type{};
        double zGateS1{1.2};
        double e0{1.60218e-19};
        double kB{1.38065e-23};
        double q10Fluo{1.67};
    };)";
            REQUIRE_THAT(generated,
                         ContainsSubstring(reindent_text(stringutils::trim(expected_code))));
        }
    }
}

SCENARIO("Check code generation for FUNCTION_TABLE block", "[codegen][function_table]") {
    GIVEN("A MOD file with Function table block") {
        std::string const nmodl_text = R"(
            NEURON { SUFFIX glia }
            FUNCTION_TABLE ttt(l (mV))
            FUNCTION_TABLE uuu(l, k)
        )";
        THEN("Code should be generated correctly") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            REQUIRE_THAT(generated, ContainsSubstring("double ttt_glia("));
            REQUIRE_THAT(generated, ContainsSubstring("double table_ttt_glia("));
            REQUIRE_THAT(generated,
                         ContainsSubstring("hoc_spec_table(&inst->global->_ptable_ttt, 1"));
            REQUIRE_THAT(generated, ContainsSubstring("double uuu_glia("));
            REQUIRE_THAT(generated, ContainsSubstring("double table_uuu_glia("));
            REQUIRE_THAT(generated,
                         ContainsSubstring("hoc_func_table(inst->global->_ptable_uuu, 2"));
        }
    }
}

SCENARIO("Check that loops are well generated", "[codegen][loops]") {
    GIVEN("A mod file containing for/while/if/else/FROM") {
        std::string const nmodl_text = R"(
            PROCEDURE foo() {
                LOCAL a, b
                if (a == 1) {
                    b = 5
                } else if (a == 2) {
                    b = 6
                } else {
                    b = 7 ^ 2
                }

                while (b > 0) {
                    b = b - 1
                }
                FROM a = 1 TO 10 BY 2 {
                    b = b + 1
                }
            })";

        THEN("Correct code is generated") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string expected_code = R"(double a, b;
        if (a == 1.0) {
            b = 5.0;
        } else if (a == 2.0) {
            b = 6.0;
        } else {
            b = pow(7.0, 2.0);
        }
        while (b > 0.0) {
            b = b - 1.0;
        }
        for (int a = 1; a <= 10; a += 2) {
            b = b + 1.0;
        })";
            REQUIRE_THAT(generated, ContainsSubstring(expected_code));
        }
    }
}


SCENARIO("Check that top verbatim blocks are well generated", "[codegen][top verbatim block]") {
    GIVEN("A mod file containing top verbatim block") {
        std::string const nmodl_text = R"(
            PROCEDURE foo(nt) {
            }
            VERBATIM
            // This is a top verbatim block
            double a = 2.;
            // This procedure should be replaced
            foo(_nt);
            _tqitem;
            _STRIDE;
            ENDVERBATIM
        )";

        THEN("Correct code is generated") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string expected_code = R"(using namespace coreneuron;


            double a = 2.;
            foo_(nt);
            &tqitem;
            pnodecount+id;)";
            REQUIRE_THAT(generated, ContainsSubstring(expected_code));
        }
    }
}


SCENARIO("Check that codegen generate event functions well", "[codegen][net_events]") {
    GIVEN("A mod file with events") {
        std::string const nmodl_text = R"(
            NET_RECEIVE(w) {
                INITIAL {}
                if (flag == 0) {
                    net_event(t)
                    net_move(t+1)
                } else {
                    net_send(1, 1)
                }
            }
        )";

        THEN("Correct code is generated") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string cpu_net_send_expected_code =
                R"(static inline void net_send_buffering(const NrnThread* nt, NetSendBuffer_t* nsb, int type, int vdata_index, int weight_index, int point_index, double t, double flag) {
        int i = 0;
        i = nsb->_cnt++;
        if (i >= nsb->_size) {
            nsb->grow();
        }
        if (i < nsb->_size) {
            nsb->_sendtype[i] = type;
            nsb->_vdata_index[i] = vdata_index;
            nsb->_weight_index[i] = weight_index;
            nsb->_pnt_index[i] = point_index;
            nsb->_nsb_t[i] = t;
            nsb->_nsb_flag[i] = flag;
        }
    })";
            REQUIRE_THAT(generated, ContainsSubstring(cpu_net_send_expected_code));
            auto const gpu_generated = get_coreneuron_cpp_code(nmodl_text, true);
            std::string gpu_net_send_expected_code =
                R"(static inline void net_send_buffering(const NrnThread* nt, NetSendBuffer_t* nsb, int type, int vdata_index, int weight_index, int point_index, double t, double flag) {
        int i = 0;
        if (nt->compute_gpu) {
            nrn_pragma_acc(atomic capture)
            nrn_pragma_omp(atomic capture)
            i = nsb->_cnt++;
        } else {
            i = nsb->_cnt++;
        }
        if (i < nsb->_size) {
            nsb->_sendtype[i] = type;
            nsb->_vdata_index[i] = vdata_index;
            nsb->_weight_index[i] = weight_index;
            nsb->_pnt_index[i] = point_index;
            nsb->_nsb_t[i] = t;
            nsb->_nsb_flag[i] = flag;
        }
    })";
            REQUIRE_THAT(gpu_generated, ContainsSubstring(gpu_net_send_expected_code));
            std::string net_receive_kernel_expected_code =
                R"(static inline void net_receive_kernel_(double t, Point_process* pnt, _Instance* inst, NrnThread* nt, Memb_list* ml, int weight_index, double flag) {
        int tid = pnt->_tid;
        int id = pnt->_i_instance;
        double v = 0;
        int nodecount = ml->nodecount;
        int pnodecount = ml->_nodecount_padded;
        double* data = ml->data;
        double* weights = nt->weights;
        Datum* indexes = ml->pdata;
        ThreadDatum* thread = ml->_thread;

        inst->tsave[id] = t;
        {
            if (flag == 0.0) {
                net_send_buffering(nt, ml->_net_send_buffer, 1, -1, -1, point_process, t, 0.0);
                net_send_buffering(nt, ml->_net_send_buffer, 2, inst->tqitem[0*pnodecount+id], -1, point_process, t + 1.0, 0.0);
            } else {
                net_send_buffering(nt, ml->_net_send_buffer, 0, inst->tqitem[0*pnodecount+id], weight_index, point_process, t+1.0, 1.0);
            }
        }
    })";
            REQUIRE_THAT(generated, ContainsSubstring(net_receive_kernel_expected_code));
            std::string net_receive_expected_code =
                R"(static void net_receive_(Point_process* pnt, int weight_index, double flag) {
        NrnThread* nt = nrn_threads + pnt->_tid;
        Memb_list* ml = get_memb_list(nt);
        NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;
        if (nrb->_cnt >= nrb->_size) {
            realloc_net_receive_buffer(nt, ml);
        }
        int id = nrb->_cnt;
        nrb->_pnt_index[id] = pnt-nt->pntprocs;
        nrb->_weight_index[id] = weight_index;
        nrb->_nrb_t[id] = nt->_t;
        nrb->_nrb_flag[id] = flag;
        nrb->_cnt++;
    })";
            REQUIRE_THAT(generated, ContainsSubstring(net_receive_expected_code));
            std::string net_buf_receive_expected_code = R"(void net_buf_receive_(NrnThread* nt) {
        Memb_list* ml = get_memb_list(nt);
        if (!ml) {
            return;
        }

        NetReceiveBuffer_t* nrb = ml->_net_receive_buffer;
        auto* const inst = static_cast<_Instance*>(ml->instance);
        int count = nrb->_displ_cnt;
        #pragma omp simd
        #pragma ivdep
        for (int i = 0; i < count; i++) {
            int start = nrb->_displ[i];
            int end = nrb->_displ[i+1];
            for (int j = start; j < end; j++) {
                int index = nrb->_nrb_index[j];
                int offset = nrb->_pnt_index[index];
                double t = nrb->_nrb_t[index];
                int weight_index = nrb->_weight_index[index];
                double flag = nrb->_nrb_flag[index];
                Point_process* point_process = nt->pntprocs + offset;
                net_receive_kernel_(t, point_process, inst, nt, ml, weight_index, flag);
            }
        }
        nrb->_displ_cnt = 0;
        nrb->_cnt = 0;

        NetSendBuffer_t* nsb = ml->_net_send_buffer;
        for (int i=0; i < nsb->_cnt; i++) {
            int type = nsb->_sendtype[i];
            int tid = nt->id;
            double t = nsb->_nsb_t[i];
            double flag = nsb->_nsb_flag[i];
            int vdata_index = nsb->_vdata_index[i];
            int weight_index = nsb->_weight_index[i];
            int point_index = nsb->_pnt_index[i];
            net_sem_from_gpu(type, vdata_index, weight_index, tid, point_index, t, flag);
        }
        nsb->_cnt = 0;
    })";
            REQUIRE_THAT(generated, ContainsSubstring(net_buf_receive_expected_code));
            std::string net_init_expected_code =
                R"(static void net_init(Point_process* pnt, int weight_index, double flag) {
        // do nothing
    })";
            REQUIRE_THAT(generated, ContainsSubstring(net_init_expected_code));
            std::string set_pnt_receive_expected_code =
                "set_pnt_receive(mech_type, net_receive_, net_init, num_net_receive_args());";
            REQUIRE_THAT(generated, ContainsSubstring(set_pnt_receive_expected_code));
        }
    }
    GIVEN("A mod file with an INITIAL inside NET_RECEIVE") {
        std::string const nmodl_text = R"(
            NET_RECEIVE(w) {
                INITIAL {
                    a = 1
                }
            }
        )";
        THEN("It should generate a net_init") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string expected_code =
                R"(static void net_init(Point_process* pnt, int weight_index, double flag) {
        int tid = pnt->_tid;
        int id = pnt->_i_instance;
        double v = 0;
        NrnThread* nt = nrn_threads + tid;
        Memb_list* ml = nt->_ml_list[pnt->_type];
        int nodecount = ml->nodecount;
        int pnodecount = ml->_nodecount_padded;
        double* data = ml->data;
        double* weights = nt->weights;
        Datum* indexes = ml->pdata;
        ThreadDatum* thread = ml->_thread;
        auto* const inst = static_cast<_Instance*>(ml->instance);

        a = 1.0;
        auto& nsb = ml->_net_send_buffer;
    })";
            REQUIRE_THAT(generated, ContainsSubstring(expected_code));
        }
    }

    GIVEN("A mod file with an INITIAL with net_send() inside NET_RECEIVE") {
        std::string const nmodl_text = R"(
            NET_RECEIVE(w) {
                INITIAL {
                    net_send(5, 1)
                }
            }
        )";
        THEN("It should generate a net_send_buffering with weight_index as parameter variable") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string expected_code(
                "net_send_buffering(nt, ml->_net_send_buffer, 0, inst->tqitem[0*pnodecount+id], "
                "weight_index, point_process, nt->_t+5.0, 1.0);");
            REQUIRE_THAT(generated, ContainsSubstring(expected_code));
        }
    }

    GIVEN("A mod file with a top level INITIAL block with net_send()") {
        std::string const nmodl_text = R"(
            INITIAL {
                net_send(5, 1)
            }
        )";
        THEN("It should generate a net_send_buffering with weight_index parameter as 0") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string expected_code(
                "net_send_buffering(nt, ml->_net_send_buffer, 0, inst->tqitem[0*pnodecount+id], 0, "
                "point_process, nt->_t+5.0, 1.0);");
            REQUIRE_THAT(generated, ContainsSubstring(expected_code));
        }
    }

    GIVEN("A mod file with FOR_NETCONS") {
        std::string const nmodl_text = R"(
            NET_RECEIVE(w) {
                FOR_NETCONS(v) {
                    b = 2
                }
            }
        )";
        THEN("New code is generated for for_netcons") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string net_receive_kernel_expected_code =
                R"(static inline void net_receive_kernel_(double t, Point_process* pnt, _Instance* inst, NrnThread* nt, Memb_list* ml, int weight_index, double flag) {
        int tid = pnt->_tid;
        int id = pnt->_i_instance;
        double v = 0;
        int nodecount = ml->nodecount;
        int pnodecount = ml->_nodecount_padded;
        double* data = ml->data;
        double* weights = nt->weights;
        Datum* indexes = ml->pdata;
        ThreadDatum* thread = ml->_thread;

        int node_id = ml->nodeindices[id];
        v = nt->_actual_v[node_id];
        inst->tsave[id] = t;
        {
            const size_t offset = 0*pnodecount + id;
            const size_t for_netcon_start = nt->_fornetcon_perm_indices[indexes[offset]];
            const size_t for_netcon_end = nt->_fornetcon_perm_indices[indexes[offset] + 1];
            for (auto i = for_netcon_start; i < for_netcon_end; ++i) {
                b = 2.0;
            }

        }
    })";
            REQUIRE_THAT(generated, ContainsSubstring(net_receive_kernel_expected_code));
            std::string registration_expected_code = "add_nrn_fornetcons(mech_type, 0);";
            REQUIRE_THAT(generated, ContainsSubstring(registration_expected_code));
        }
    }
    GIVEN("A mod file with a net_move outside NET_RECEIVE") {
        std::string const nmodl_text = R"(
            PROCEDURE foo() {
                net_move(t+1)
            }
        )";
        THEN("It should throw") {
            REQUIRE_THROWS(get_coreneuron_cpp_code(nmodl_text));
        }
    }
}


SCENARIO("Some tests on derivimplicit", "[codegen][derivimplicit_solver]") {
    GIVEN("A mod file with derivimplicit") {
        std::string const nmodl_text = R"(
            STATE {
                m
            }
            BREAKPOINT {
                SOLVE state METHOD derivimplicit
            }
            DERIVATIVE state {
               m' = 2 * m
            }
        )";
        THEN("Correct code is generated") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string newton_state_expected_code = R"(namespace {
        struct _newton_state_ {
            int operator()(int id, int pnodecount, double* data, Datum* indexes, ThreadDatum* thread, NrnThread* nt, Memb_list* ml, double v) const {
                auto* const inst = static_cast<_Instance*>(ml->instance);
                double* savstate1 = static_cast<double*>(thread[dith1()].pval);
                auto const& slist1 = inst->global->slist1;
                auto const& dlist1 = inst->global->dlist1;
                double* dlist2 = static_cast<double*>(thread[dith1()].pval) + (1*pnodecount);
                inst->Dm[id] = 2.0 * inst->m[id];
                int counter = -1;
                for (int i=0; i<1; i++) {
                    if (*deriv1_advance(thread)) {
                        dlist2[(++counter)*pnodecount+id] = data[dlist1[i]*pnodecount+id]-(data[slist1[i]*pnodecount+id]-savstate1[i*pnodecount+id])/nt->_dt;
                    } else {
                        dlist2[(++counter)*pnodecount+id] = data[slist1[i]*pnodecount+id]-savstate1[i*pnodecount+id];
                    }
                }
                return 0;
            }
        };
    })";
            REQUIRE_THAT(generated, ContainsSubstring(newton_state_expected_code));
            std::string state_expected_code =
                R"(int state_(int id, int pnodecount, double* data, Datum* indexes, ThreadDatum* thread, NrnThread* nt, Memb_list* ml, double v) {
        auto* const inst = static_cast<_Instance*>(ml->instance);
        double* savstate1 = (double*) thread[dith1()].pval;
        auto const& slist1 = inst->global->slist1;
        auto& slist2 = inst->global->slist2;
        double* dlist2 = static_cast<double*>(thread[dith1()].pval) + (1*pnodecount);
        for (int i=0; i<1; i++) {
            savstate1[i*pnodecount+id] = data[slist1[i]*pnodecount+id];
        }
        int reset = nrn_newton_thread(static_cast<NewtonSpace*>(*newtonspace1(thread)), 1, slist2, _newton_state_{}, dlist2, id, pnodecount, data, indexes, thread, nt, ml, v);
        return reset;
    })";
            REQUIRE_THAT(generated, ContainsSubstring(state_expected_code));
        }
    }
}


SCENARIO("Some tests on euler solver", "[codegen][euler_solver]") {
    GIVEN("A mod file with euler") {
        std::string const nmodl_text = R"(
            NEURON {
                RANGE inf
            }
            INITIAL {
                inf = 2
            }
            STATE {
                n
                m
            }
            BREAKPOINT {
                SOLVE state METHOD euler
            }
            DERIVATIVE state {
               m' = 2 * m
               inf = inf * 3
               n' = (2 + m - inf) * n
            }
        )";
        THEN("Correct code is generated") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string nrn_state_expected_code = R"(inst->Dm[id] = 2.0 * inst->m[id];
            inf = inf * 3.0;
            inst->Dn[id] = (2.0 + inst->m[id] - inf) * inst->n[id];
            inst->m[id] = inst->m[id] + nt->_dt * inst->Dm[id];
            inst->n[id] = inst->n[id] + nt->_dt * inst->Dn[id];)";
            REQUIRE_THAT(generated, ContainsSubstring(nrn_state_expected_code));
        }
    }
}


SCENARIO("Check codegen for MUTEX and PROTECT", "[codegen][mutex_protect]") {
    GIVEN("A mod file containing MUTEX & PROTECT") {
        std::string const nmodl_text = R"(
            NEURON {
                SUFFIX TEST
                RANGE tmp, foo
            }
            PARAMETER {
                tmp = 10
                foo = 20
            }
            INITIAL {
                MUTEXLOCK
                tmp = 11
                MUTEXUNLOCK
                PROTECT tmp = tmp / 2.5
            }
            PROCEDURE bar() {
                PROTECT foo = foo - 21
            }
        )";

        THEN("Code with OpenMP critical sections is generated") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            // critical section for the mutex block
            std::string expected_code_initial = R"(#pragma omp critical (TEST)
                {
                    inst->tmp[id] = 11.0;
                }
                #pragma omp atomic update
                inst->tmp[id] = inst->tmp[id] / 2.5;)";

            // atomic update for the PROTECT construct
            std::string expected_code_proc = R"(#pragma omp atomic update
        inst->foo[id] = inst->foo[id] - 21.0;)";

            REQUIRE_THAT(generated, ContainsSubstring(expected_code_initial));
            REQUIRE_THAT(generated, ContainsSubstring(expected_code_proc));
        }
    }
}


SCENARIO("Array STATE variable", "[codegen][array_state]") {
    GIVEN("A mod file containing an array STATE variable") {
        std::string const nmodl_text = R"(
            DEFINE NANN  4

            NEURON {
                SUFFIX ca_test
            }
            STATE {
                ca[NANN]
                k
            }
        )";

        THEN("nrn_init is printed with proper initialization of the whole array") {
            auto const generated = get_coreneuron_cpp_code(nmodl_text);
            std::string expected_code_init =
                R"(/** initialize channel */
    void nrn_init_ca_test(NrnThread* nt, Memb_list* ml, int type) {
        int nodecount = ml->nodecount;
        int pnodecount = ml->_nodecount_padded;
        const int* node_index = ml->nodeindices;
        double* data = ml->data;
        const double* voltage = nt->_actual_v;
        Datum* indexes = ml->pdata;
        ThreadDatum* thread = ml->_thread;

        setup_instance(nt, ml);
        auto* const inst = static_cast<ca_test_Instance*>(ml->instance);

        if (_nrn_skip_initmodel == 0) {
            #pragma omp simd
            #pragma ivdep
            for (int id = 0; id < nodecount; id++) {
                int node_id = node_index[id];
                double v = voltage[node_id];
                #if NRN_PRCELLSTATE
                inst->v_unused[id] = v;
                #endif
                (inst->ca+id*4)[0] = inst->global->ca0;
                (inst->ca+id*4)[1] = inst->global->ca0;
                (inst->ca+id*4)[2] = inst->global->ca0;
                (inst->ca+id*4)[3] = inst->global->ca0;
                inst->k[id] = inst->global->k0;
            }
        }
    })";

            REQUIRE_THAT(generated, ContainsSubstring(expected_code_init));
        }
    }
}
