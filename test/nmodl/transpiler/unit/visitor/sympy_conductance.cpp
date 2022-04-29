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
#include "visitors/constant_folder_visitor.hpp"
#include "visitors/inline_visitor.hpp"
#include "visitors/local_var_rename_visitor.hpp"
#include "visitors/sympy_conductance_visitor.hpp"
#include "visitors/symtab_visitor.hpp"
#include "visitors/visitor_utils.hpp"

using namespace nmodl;
using namespace visitor;
using namespace test;
using namespace test_utils;

using ast::AstNodeType;
using nmodl::parser::NmodlDriver;


//=============================================================================
// SympyConductance visitor tests
//=============================================================================

std::string run_sympy_conductance_visitor(const std::string& text) {
    // construct AST from text
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor(false).visit_program(*ast);

    // run constant folding, inlining & local renaming first
    ConstantFolderVisitor().visit_program(*ast);
    InlineVisitor().visit_program(*ast);
    LocalVarRenameVisitor().visit_program(*ast);
    SymtabVisitor(true).visit_program(*ast);

    // run SympyConductance on AST
    SympyConductanceVisitor().visit_program(*ast);

    // check that, after visitor rearrangement, parents are still up-to-date
    CheckParentVisitor().check_ast(*ast);

    // run lookup visitor to extract results from AST
    // return BREAKPOINT block as JSON string
    return reindent_text(to_nmodl(collect_nodes(*ast, {AstNodeType::BREAKPOINT_BLOCK}).front()));
}

std::string breakpoint_to_nmodl(const std::string& text) {
    // construct AST from text
    NmodlDriver driver;
    const auto& ast = driver.parse_string(text);

    // construct symbol table from AST
    SymtabVisitor().visit_program(*ast);

    // run lookup visitor to extract results from AST
    // return BREAKPOINT block as JSON string
    return reindent_text(to_nmodl(collect_nodes(*ast, {AstNodeType::BREAKPOINT_BLOCK}).front()));
}

void run_sympy_conductance_passes(ast::Program& node) {
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

SCENARIO("Addition of CONDUCTANCE using SympyConductance visitor", "[visitor][solver][sympy]") {
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
                g_AMPA = 1e-3*gmax_AMPA*(B_AMPA-A_AMPA)
                i_AMPA = g_AMPA*(v-E_AMPA)
                gmax_NMDA = gmax_AMPA*NMDA_ratio
                mggate = 1/(1+exp(slope_mg*-v)*(mg/scale_mg))
                g_NMDA = 1e-3*gmax_NMDA*mggate*(B_NMDA-A_NMDA)
                i_NMDA = g_NMDA*(v-E_NMDA)
                Pf_NMDA = (4*cao_CR)/(4*cao_CR+0.7246376811594204*120(mM))*0.6
                ica_NMDA = Pf_NMDA*g_NMDA*(v-40.0)
                gca_bar_abs_VDCC = gca_bar_VDCC*4(um2)*PI*(3(1/um3)/4*volume_CR*1/PI)^0.6666666666666666
                gca_VDCC = 1e-3*gca_bar_abs_VDCC*m_VDCC*m_VDCC*h_VDCC
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
