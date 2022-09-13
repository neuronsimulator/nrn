/*************************************************************************
 * Copyright (C) 2019-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch2/catch.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_c_visitor.hpp"
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

using Catch::Matchers::Contains;  // ContainsSubstring in newer Catch2

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;
using nmodl::test_utils::reindent_text;

/// Helper for creating C codegen visitor
std::shared_ptr<CodegenCVisitor> create_c_visitor(const std::shared_ptr<ast::Program>& ast,
                                                  const std::string& /* text */,
                                                  std::stringstream& ss) {
    /// construct symbol table
    SymtabVisitor().visit_program(*ast);

    /// run all necessary pass
    InlineVisitor().visit_program(*ast);
    NeuronSolveVisitor().visit_program(*ast);
    SolveBlockVisitor().visit_program(*ast);

    /// create C code generation visitor
    auto cv = std::make_shared<CodegenCVisitor>("temp.mod", ss, "double", false);
    cv->setup(*ast);
    return cv;
}

/// print instance structure for testing purpose
std::string get_instance_var_setup_function(std::string& nmodl_text) {
    const auto& ast = NmodlDriver().parse_string(nmodl_text);
    std::stringstream ss;
    auto cvisitor = create_c_visitor(ast, nmodl_text, ss);
    cvisitor->print_instance_variable_setup();
    return reindent_text(ss.str());
}

/// print entire code
std::string get_cpp_code(const std::string& nmodl_text) {
    const auto& ast = NmodlDriver().parse_string(nmodl_text);
    std::stringstream ss;
    auto cvisitor = create_c_visitor(ast, nmodl_text, ss);
    cvisitor->visit_program(*ast);
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
            REQUIRE_THAT(result, Contains(expected));
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
            REQUIRE_THAT(result, Contains(expected));
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
              RANGE caiinf, catau, cai, ncai, lcai, eca, elca, enca, g
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
                    inst->ncai = ml->data+10*pnodecount;
                    inst->Dncai = ml->data+11*pnodecount;
                    inst->lcai = ml->data+12*pnodecount;
                    inst->Dlcai = ml->data+13*pnodecount;
                    inst->ion_ncai = nt->_data;
                    inst->ion_inca = nt->_data;
                    inst->ion_enca = nt->_data;
                    inst->style_nca = ml->pdata;
                    inst->ion_lcai = nt->_data;
                    inst->ion_ilca = nt->_data;
                    inst->ion_elca = nt->_data;
                    inst->style_lca = ml->pdata;
                }
            )";

            auto const expected = reindent_text(generated_code);
            auto const result = get_instance_var_setup_function(nmodl_text);
            REQUIRE_THAT(result, Contains(expected));
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
    CodegenCVisitor cv{"temp.mod", ss, "double", false};
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
                    double* __restrict__ invl{};
                    const double* __restrict__ burst_start{};
                    double* __restrict__ v_unused{};
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
            REQUIRE_THAT(generated, Contains("double* __restrict__ celsius{&coreneuron::celsius}"));
            REQUIRE_THAT(generated, Contains("double* __restrict__ pi{&coreneuron::pi}"));
            REQUIRE_THAT(generated,
                         Contains("int* __restrict__ secondorder{&coreneuron::secondorder}"));
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
            REQUIRE_THAT(generated, Contains("celsius"));
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
            REQUIRE_THAT(generated, !Contains("celsius"));
            REQUIRE_THAT(generated, !Contains("pi"));
            REQUIRE_THAT(generated, !Contains("secondorder"));
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
            auto const generated = get_cpp_code(nmodl_text);
            REQUIRE_THAT(generated, Contains("double t_inf[2][201]{};"));
            REQUIRE_THAT(generated, Contains("double t_tau[201]{};"));

            REQUIRE_THAT(generated, Contains("inst->global->t_inf[0][i] = (inst->inf+id*2)[0];"));
            REQUIRE_THAT(generated, Contains("inst->global->t_inf[1][i] = (inst->inf+id*2)[1];"));
            REQUIRE_THAT(generated, Contains("inst->global->t_tau[i] = inst->global->tau;"));

            REQUIRE_THAT(generated,
                         Contains("(inst->inf+id*2)[0] = inst->global->t_inf[0][index];"));

            REQUIRE_THAT(generated, Contains("(inst->inf+id*2)[0] = inst->global->t_inf[0][i]"));
            REQUIRE_THAT(generated, Contains("(inst->inf+id*2)[1] = inst->global->t_inf[1][i]"));
            REQUIRE_THAT(generated, Contains("inst->global->tau = inst->global->t_tau[i]"));
        }
    }
}
