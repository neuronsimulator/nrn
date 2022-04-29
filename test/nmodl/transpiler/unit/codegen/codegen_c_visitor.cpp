/*************************************************************************
 * Copyright (C) 2019-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <catch/catch.hpp>

#include "ast/program.hpp"
#include "codegen/codegen_c_visitor.hpp"
#include "codegen/codegen_helper_visitor.hpp"
#include "parser/nmodl_driver.hpp"
#include "test/unit/utils/test_utils.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace nmodl;
using namespace visitor;
using namespace codegen;

using nmodl::parser::NmodlDriver;
using nmodl::test_utils::reindent_text;

/// Helper for creating C codegen visitor
std::shared_ptr<CodegenCVisitor> create_c_visitor(const std::shared_ptr<ast::Program>& ast,
                                                  const std::string& text,
                                                  std::stringstream& ss) {
    /// construct symbol table
    SymtabVisitor().visit_program(*ast);

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
                static inline void setup_instance(NrnThread* nt, Memb_list* ml)  {
                    cal_Instance* inst = (cal_Instance*) mem_alloc(1, sizeof(cal_Instance));
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
                    ml->instance = inst;
                }
            )";
            auto expected = reindent_text(generated_code);
            auto result = get_instance_var_setup_function(nmodl_text);
            REQUIRE(result.find(expected) != std::string::npos);
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
                static inline void setup_instance(NrnThread* nt, Memb_list* ml)  {
                    lca_Instance* inst = (lca_Instance*) mem_alloc(1, sizeof(lca_Instance));
                    int pnodecount = ml->_nodecount_padded;
                    Datum* indexes = ml->pdata;
                    inst->m = ml->data+0*pnodecount;
                    inst->cai = ml->data+1*pnodecount;
                    inst->cao = ml->data+2*pnodecount;
                    inst->Dm = ml->data+3*pnodecount;
                    inst->v_unused = ml->data+4*pnodecount;
                    inst->ion_cai = nt->_data;
                    inst->ion_cao = nt->_data;
                    ml->instance = inst;
                }
            )";

            auto expected = reindent_text(generated_code);
            auto result = get_instance_var_setup_function(nmodl_text);
            REQUIRE(result.find(expected) != std::string::npos);
        }
    }

    // In the below mod file, ion variables ncai and lcai are declared
    // as state variables as well as range variables. The issue about
    // this mod file ordering was fixed in #443.
    GIVEN("ccanl.mod: mod file from reduced_dentate model") {
        std::string nmodl_text = R"(
            NEURON {
              SUFFIX ccanl
              USEION nca READ ncai, inca, enca WRITE enca, ncai VALENCE 2
              USEION lca READ lcai, ilca, elca WRITE elca, lcai VALENCE 2
              RANGE caiinf, catau, cai, ncai, lcai, eca, elca, enca
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
            }
            STATE {
              ncai(mM)
              lcai(mM)
            }
        )";

        THEN("Ion variables are defined in the order of USEION") {
            std::string generated_code = R"(
                static inline void setup_instance(NrnThread* nt, Memb_list* ml)  {
                    ccanl_Instance* inst = (ccanl_Instance*) mem_alloc(1, sizeof(ccanl_Instance));
                    int pnodecount = ml->_nodecount_padded;
                    Datum* indexes = ml->pdata;
                    inst->catau = ml->data+0*pnodecount;
                    inst->caiinf = ml->data+1*pnodecount;
                    inst->cai = ml->data+2*pnodecount;
                    inst->eca = ml->data+3*pnodecount;
                    inst->ica = ml->data+4*pnodecount;
                    inst->inca = ml->data+5*pnodecount;
                    inst->ilca = ml->data+6*pnodecount;
                    inst->enca = ml->data+7*pnodecount;
                    inst->elca = ml->data+8*pnodecount;
                    inst->ncai = ml->data+9*pnodecount;
                    inst->Dncai = ml->data+10*pnodecount;
                    inst->lcai = ml->data+11*pnodecount;
                    inst->Dlcai = ml->data+12*pnodecount;
                    inst->v_unused = ml->data+13*pnodecount;
                    inst->ion_ncai = nt->_data;
                    inst->ion_inca = nt->_data;
                    inst->ion_enca = nt->_data;
                    inst->style_nca = ml->pdata;
                    inst->ion_lcai = nt->_data;
                    inst->ion_ilca = nt->_data;
                    inst->ion_elca = nt->_data;
                    inst->style_lca = ml->pdata;
                    ml->instance = inst;
                }
            )";

            auto expected = reindent_text(generated_code);
            auto result = get_instance_var_setup_function(nmodl_text);
            REQUIRE(result.find(expected) != std::string::npos);
        }
    }
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
            std::stringstream ss;

            /// parse mod file & print mechanism structure
            const auto& ast = NmodlDriver().parse_string(nmodl_text);
            auto cvisitor = create_c_visitor(ast, nmodl_text, ss);
            cvisitor->print_mechanism_range_var_structure();

            std::string expected_code = R"(
                /** all mechanism instance variables */
                struct IntervalFire_Instance  {
                    double* __restrict__ invl;
                    const double* __restrict__ burst_start;
                    double* __restrict__ v_unused;
                };
            )";

            REQUIRE(reindent_text(ss.str()) == reindent_text(expected_code));
        }
    }
}
