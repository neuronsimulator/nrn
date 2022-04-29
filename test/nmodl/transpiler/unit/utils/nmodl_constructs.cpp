/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "nmodl_constructs.hpp"

namespace nmodl {
/// custom type to represent nmodl construct for testing
namespace test_utils {

/**
 * Guidelines for adding nmodl text constructs
 *
 * As nmodl constructs are used to for testing ast to nmodl transformations,
 * consider following points:
 *  - Leading whitespaces or empty lines are removed
 *  - Use string literal to define nmodl text
 *    When ast is transformed back to nmodl, each statement has newline.
 *    Hence for easy comparison, input nmodl should be null terminated.
 *    One way to use format:
 *
 *  \code
 *          R"(
 *              TITLE nmodl title
 *          )"
 *  \endcode
 *
 *  - Do not use extra spaces (even though it's valid)
 *
 *  \code
 *     LOCAL a,b
 *  \endcode
 *
 *     instead of
 *
 *  \code
 *     LOCAL  a, b,   c
 *  \endcode
 *
 *  - Use well indented blocks
 *
 *  \code
 *    NEURON {
 *        RANGE x
 *    }
 *  \endcode
 *
 *    instead of
 *
 *  \code
 *      NEURON  {
 *        RANGE x
 *    }
 *  \endcode
 *
 * If nmodl transformation is different from input, third argument could be
 * provided with the expected nmodl.
 */

std::map<std::string, NmodlTestCase> nmodl_invalid_constructs{
    // clang-format off
    {
        "title_1",
        {
            "Title statement without any text",
            "TITLE"
        }
    },

    {
        "local_list_1",
        {
            "LOCAL statement without any variable",
            "LOCAL"
        }
    },

    {
        "local_list_2",
        {
            "LOCAL statement with empty index",
            "LOCAL gbar[]"
        }
    },

    {
        "local_list_3",
        {
            "LOCAL statement with invalid index",
            "LOCAL gbar[2.0]"
        }
    },

    {
        "define_1",
        {
            "Incomplete macro definition without value",
            "DEFINE NSTEP"
        }
    },

    {
        "model_level_1",
        {
            "Model level without any block",
            "MODEL_LEVEL 2"
        }
    },

    {
        "verbatim_block_1",
        {
            "Nested verbatim blocks",
            R"(
                VERBATIM
                VERBATIM
                    #include <cuda.h>
                ENDVERBATIM
                ENDVERBATIM
            )"
        }
    },

    {
        "comment_block_1",
        {
            "Nested comment blocks",
            R"(
                COMMENT
                COMMENT
                    some code comment here
                ENDCOMMENT
                ENDCOMMENT
            )"
        }
    },

     {
        "include_statement_1",
        {
            "Incomplete include statements",
            "INCLUDE  "
        }
    },

    {
        "parameter_block_1",
        {
            "Incomplete parameter declaration",
            R"(
                PARAMETER {
                    ampa =
                }
            )"
        }
    },

    {
        "parameter_block_2",
        {
            "Invalid parameter declaration",
            R"(
                PARAMETER {
                    ampa = 2 (ms>
                }
            )"
        }
    },

    {
        "neuron_block_1",
        {
            "Invalid CURIE statement",
            R"(
                NEURON {
                    REPRESENTS xx
                }
            )"
        }
    }


    // clang-format on
};

std::map<std::string, NmodlTestCase> nmodl_valid_constructs{
    // clang-format off
    {
        "title_1",
        {
            "Title statement",
            R"(
                TITLE nmodl title
            )"
        }
    },

    {
        "local_list_1",
        {
            "Standalone LOCAL statement with single scalar variable",
            R"(
                LOCAL gbar
            )"
        }
    },

    {
        "local_list_2",
        {
            "Standalone LOCAL statement with single vector variable",
            R"(
                LOCAL gbar[2]
            )"
        }
    },

    {
        "local_list_3",
        {
            "Standalone LOCAL statement with multiple variables",
            R"(
                LOCAL gbar, ek[2], ik
            )"
        }
    },

    {
        "local_list_4",
        {
            "Standalone LOCAL statement with multiple variables with/without whitespaces",
            R"(
                LOCAL gbar, ek[2],ik,  gk
            )",
            R"(
                LOCAL gbar, ek[2], ik, gk
            )"
        }
    },

    {
        "define_1",
        {
            "Macro definition",
            R"(
                DEFINE NSTEP 10
            )"
        }
    },

    {
        "statement_block_1",
        {
            "Statements should preserve order with blocks",
            R"(
                DEFINE ASTEP 10

                NEURON {
                    RANGE x
                }

                DEFINE BSTEP 20
            )"
        }
    },

    {
        "artificial_cell_1",
        {
            "Artificial Statement usage",
            R"(
                NEURON {
                    ARTIFICIAL_CELL NSLOC
                }
            )"
        }
    },
/** \todo : MODEL_LEVEL is not handled in parser as it's deprecated
    {
        "model_level_1",
        {
            "Model level followed by block",
            "MODEL_LEVEL 2 NEURON {}"
        }
    },
*/

    {
        "verbatim_block_1",
        {
            "Stanadlone empty verbatim block",
            R"(
                VERBATIM
                ENDVERBATIM
            )"
        }
    },

    {
        "verbatim_block_2",
        {
            "Standalone verbatim block",
            R"(
                VERBATIM
                    #include <cuda.h>
                ENDVERBATIM
            )"
        }
    },

    {
        "comment_block_1",
        {
            "Standalone comment block",
            R"(
                COMMENT
                    some comment here
                ENDCOMMENT
            )"
        }
    },

    {
        "unit_statement_1",
        {
            "Standalone unit on/off statements",
            R"(
                UNITSON

                UNITSOFF

                UNITSON

                UNITSOFF
            )"
        }
    },

    {
        "include_statement_1",
        {
            "Standalone include statements",
            R"(
                INCLUDE "Unit.inc"
            )"
        }
    },

    {
        "parameter_block_1",
        {
            "Empty parameter block",
            R"(
                PARAMETER {
                }
            )"
        }
    },

    {
        "parameter_block_2",
        {
            "PARAMETER block with all statement types",
            R"(
                PARAMETER {
                    tau_r_AMPA = 10
                    tau_d_AMPA = 10.1 (mV)
                    tau_r_NMDA = 10 (mV) <1,2>
                    tau_d_NMDA = 10 (mV) <1.1,2.2>
                    Use (mV)
                    Dep[1] <1,2>
                    Fac[1] (mV) <1,2>
                    g
                }
            )"
        }
    },

    {
        "parameter_block_3",
        {
            "PARAMETER statement can use macro definition as a number",
            R"(
                DEFINE SIX 6

                PARAMETER {
                    tau_r_AMPA = SIX
                }
            )"
        }
    },

    {
        "parameter_block_4",
        {
            "PARAMETER block with inconsistent whitespaces",
            R"(
                PARAMETER {
                    tau_r_AMPA=10.0
                    tau_d_AMPA = 10.1(mV)
                    tau_r_NMDA = 10 (mV) <1, 2>
                    tau_d_NMDA= 10 (mV)<1.1,2.2>
                    Use (mV)
                    Dep [1] <1,2>
                    Fac[1](mV)<1,2>
                    g
                }
            )",
            R"(
                PARAMETER {
                    tau_r_AMPA = 10.0
                    tau_d_AMPA = 10.1 (mV)
                    tau_r_NMDA = 10 (mV) <1,2>
                    tau_d_NMDA = 10 (mV) <1.1,2.2>
                    Use (mV)
                    Dep[1] <1,2>
                    Fac[1] (mV) <1,2>
                    g
                }
            )"
        }
    },

    {
        "parameter_block_5",
        {
            "PARAMETER block with very small/large doubles",
            R"(
                PARAMETER {
                    tau_r_AMPA = 1e-16
                    tau_d_AMPA = 1.7e-22
                    tau_r_NMDA = 1e+21
                    tau_d_NMDA = 1.3643e+27
                }
            )"
        }
    },

    {
        "step_block_1",
        {
            "STEP block with all statement types",
            R"(
                STEPPED {
                    tau_r_AMPA = 1, -2
                    tau_d_AMPA = 1.1, -2.1
                    tau_r_NMDA = 1, 2.1, 3 (mV)
                }
            )"
        }
    },

    {
        "independent_block_1",
        {
            "INDEPENDENT block with all statement types",
            R"(
                INDEPENDENT {
                    t FROM 0 TO 1 WITH 1 (ms)
                    SWEEP u FROM 0 TO 1 WITH 1 (ms)
                }
            )"
        }
    },

    {
        "dependent_block_1",
        {
            "ASSIGNED block with all statement types",
            R"(
                ASSIGNED {
                    v
                    i_AMPA (nA)
                    i_NMDA START 2.1 (nA) <0.1>
                    A_NMDA_step[1] START 1 <0.2>
                    factor_AMPA FROM 0 TO 1
                    B_NMDA_step[2] FROM 1 TO 2 START 0 (ms) <1>
                }
            )"
        }
    },

    {
        "neuron_block_1",
        {
            "NEURON block with many statement types",
            R"(
                NEURON {
                    SUFFIX ProbAMPANMDA
                    REPRESENTS NCIT:C17145
                    REPRESENTS [NCIT:C17145]
                    USEION na READ ena
                    USEION na READ ena, kna
                    USEION na WRITE ena, kna VALENCE 2.1
                    USEION na READ ena WRITE ina
                    USEION k READ ek WRITE ik REPRESENTS CHEBI:29103
                    USEION na READ ena WRITE ina VALENCE 3.3 REPRESENTS NCIT:C17
                    NONSPECIFIC_CURRENT i
                    NONSPECIFIC_CURRENT i, j
                    ELECTRODE_CURRENT i
                    RANGE tau_r_AMPA, tau_d_AMPA
                    GLOBAL gNa, xNa
                    POINTER rng1, rng2
                    BBCOREPOINTER rng3
                    EXTERNAL extvar
                    THREADSAFE
                }
            )"
        }
    },

    {
        "unit_block_1",
        {
            "UNITS block with unit definitions",
            R"(
                UNITS {
                    (mA) = (milliamp)
                    (mV) = (millivolt)
                }
            )"
        }
    },

    {
        "unit_block_2",
        {
            "UNITS block with unit and factor definitions",
            R"(
                UNITS {
                    aa = 1 (bb)
                    cc = 1.1 (dd)
                    ee = (ff) (gg)
                    hh = (ii) -> (jj)
                }
            )"
        }
    },

    {
        "unit_block_3",
        {
            "UNITS block with empty unit (called default unit)",
            R"(
                UNITS {
                    () = (millivolt)
                }
            )"
        }
    },

    {
        "constant_block_1",
        {
            "CONSTANT block with all statement types",
            R"(
                CONSTANT {
                    xx = 1
                    yy2 = 1.1
                    ee = 1e-06 (gg)
                }
            )"
        }
    },

    {
        "constant_block_2",
        {
            "CONSTANT block with signed values",
            R"(
                CONSTANT {
                    xx = +1
                    yy2 = -1.1
                    ee = 1e-06 (gg)
                }
            )",
            R"(
                CONSTANT {
                    xx = 1
                    yy2 = -1.1
                    ee = 1e-06 (gg)
                }
            )"
        }
    },

    {
        "plot_declare_1",
        {
            "PLOT declaration with single variables",
            R"(
                PLOT x VS y
            )"
        }
    },

    {
        "plot_declare_2",
        {
            "PLOT declaration with multiple variables",
            R"(
                PLOT x, y, z VS a
            )"
        }
    },

    {
        "plot_declare_3",
        {
            "PLOT declaration with indexed variables",
            R"(
                PLOT x[1], y[2], z VS a[1]
            )"
        }
    },


    {
        "statement_list_1",
        {
            "Empty statement list",
            R"(
                INITIAL {
                }
            )"
        }
    },

    {
        "statement_list_1",
        {
            "Statement list with local variables",
            R"(
                INITIAL {
                    LOCAL a, b
                }
            )"
        }
    },

    {
        "fromstmt_1",
        {
            "From statement",
            R"(
                INITIAL {
                    LOCAL a, b
                    FROM i = 0 TO 1 {
                        tau[i] = 1
                    }
                }
            )"
        }
    },

    {
        "fromstmt_2",
        {
            "From statement with integer expressions",
            R"(
                INITIAL {
                    LOCAL a, b
                    FROM i = (0+0) TO (1+b) BY X {
                        tau[i] = 1
                    }
                }
            )"
        }
    },

    {
        "forall_statement_1",
        {
            "FORALL statement",
            R"(
                INITIAL {
                    FORALL some_name {
                        a = 1
                        tau = 2.1
                    }
                }
            )"
        }
    },

    {
        "while_statement_1",
        {
            "Empty while statement",
            R"(
                CONSTRUCTOR {
                    WHILE (1) {
                    }
                }
            )"
        }
    },

    {
        "while_statement_2",
        {
            "While statement with AND expression as condition",
            R"(
                CONSTRUCTOR {
                    WHILE ((a+1)<2 && b>100) {
                        x = 10
                        y = 20
                    }
                }
            )"
        }
    },

    {
        "while_statement_3",
        {
            "While statement with OR expression as condition",
            R"(
                CONSTRUCTOR {
                    WHILE ((a+1)<=2 || b>=100) {
                        x = 10
                        y = 20
                    }
                }
            )"
        }
    },

    {
        "while_statement_4",
        {
            "While statement with NE and NOT condition",
            R"(
                CONSTRUCTOR {
                    WHILE (a!=2 && !b) {
                        x = 10
                        y = 20
                    }
                }
            )"
        }
    },

    {
        "if_statement_1",
        {
            "Empty if statement",
            R"(
                DESTRUCTOR {
                    IF (1) {
                    }
                }
            )"
        }
    },

    {
        "if_else_statement_1",
        {
            "If else statement",
            R"(
                DESTRUCTOR {
                    IF ((2.1+1)<x) {
                        a = (a+1)/2.1
                    } ELSE {
                        b = (2.1+1)
                    }
                }
            )"
        }
    },

    {
        "if_elseif_statement_1",
        {
            "If multiple else if statements",
            R"(
                FUNCTION test() {
                    IF ((2)<x) {
                        a = (a+1)/2.1
                    } ELSE IF (a<2) {
                        b = 1
                    } ELSE IF (b>2) {
                        c = 1.1
                    }
                }
            )"
        }
    },

    {
        "if_elseif_else_statement_1",
        {
            "If with multiple else if nested statements",
            R"(
                FUNCTION test(v, x) {
                    IF (2<x) {
                        a = (a+1)
                    } ELSE IF (a<2) {
                        b = 1
                    } ELSE IF (b>2) {
                        c = 1.1
                    } ELSE {
                        IF (A) {
                        } ELSE {
                            x = 1
                        }
                    }
                }
            )"
        }
    },

    {
        "solve_block_1",
        {
            "Solve statement without method",
            R"(
                BREAKPOINT {
                    SOLVE states
                }
            )"
        }
    },

    {
        "solve_block_2",
        {
            "Solve statement with method",
            R"(
                BREAKPOINT {
                    SOLVE states METHOD cnexp
                }
            )"
        }
    },

    {
        "solve_block_3",
        {
            "Solve statement with iferror block",
            R"(
                BREAKPOINT {
                    SOLVE states METHOD cnexp IFERROR {
                        a = 1
                    }
                }
            )"
        }
    },

    {
        "solve_block_equation_1",
        {
            "Solve statement without method using EQUATION, generating BREAKPOINT",
            R"(
               EQUATION {
                   SOLVE states
               }
            )",

            R"(
               BREAKPOINT {
                   SOLVE states
               }
            )"
        }
    },

    {
        "solve_block_equation_2",
        {
            "Solve statement with method using EQUATION, generating BREAKPOINT",
            R"(
                EQUATION {
                    SOLVE states METHOD cnexp
                }
            )",
            R"(
                BREAKPOINT {
                    SOLVE states METHOD cnexp
                }
            )"

        }
    },

    {
        "solve_block_equation_3",
        {
            "Solve statement with iferror block using EQUATION, generating BREAKPOINT",
            R"(
                EQUATION {
                    SOLVE states METHOD cnexp IFERROR {
                        a = 1
                    }
                }
            )",
            R"(
                BREAKPOINT {
                    SOLVE states METHOD cnexp IFERROR {
                        a = 1
                    }
                }
            )"
        }
    },

    {
        "conduct_hint_1",
        {
            "Conductance statement",
            R"(
                BREAKPOINT {
                    CONDUCTANCE gIm
                }
            )"
        }
    },

    {
        "conduct_hint_2",
        {
            "Conductance statement with ion name",
            R"(
                BREAKPOINT {
                    CONDUCTANCE gIm USEION k
                }
            )"
        }
    },

    {
        "sens_1",
        {
            "SENS statement",
            R"(
                BREAKPOINT {
                    SENS a, b
                }
            )"
        }
    },

    {
        "conserve_1",
        {
            "CONSERVE statement",
            R"(
                KINETIC ihkin {
                    CONSERVE C+o = 1
                    CONSERVE pump+pumpca = TotalPump*parea*(1e+10)
                }
            )"
        }
    },

    {
        "compartment_1",
        {
            "COMPARTMENT statement",
            R"(
                KINETIC ihkin {
                    COMPARTMENT voli {cai}
                    COMPARTMENT diam*diam*PI/4 {qk}
                    COMPARTMENT (1e+10)*area1 {pump pumpca}
                    COMPARTMENT i, diam*diam*vol[i]*1(um) {ca CaBuffer Buffer}
                }
            )"
        }
    },

    {
        "long_diffuse_1",
        {
            "LONGITUDINAL_DIFFUSION statement",
            R"(
                KINETIC ihkin {
                    LONGITUDINAL_DIFFUSION D {nai}
                    LONGITUDINAL_DIFFUSION Dk*crossSectionalArea {ko}
                    LONGITUDINAL_DIFFUSION i, DIP3*diam*diam*vrat[i] {bufm cabufm}
                }
            )"
        }
    },

    {
        "reaction_1",
        {
            "REACTION statement",
            R"(
                KINETIC kstates {
                    ~ c1 <-> i1 (Con, Coff)
                    ~ ca[i] <-> ca[i+1] (DCa*frat[i+1], DCa*frat[i+1])
                    ~ ca[0] << (in*PI*diam*(0.1)*1(um))
                    ~ nai << (-f*ina*PI*diam*(10000)/(FARADAY))
                    ~ g -> (1/tau)
                }
            )"
        }
    },

    {
        "lag_statement_1",
        {
            "LAG statement",
            R"(
                PROCEDURE lates() {
                    LAG ina BY tau
                    neo = lag_ina_tau
                }
            )"
        }
    },

    {
        "queue_statement_1",
        {
            "PUTQ and GETQ statement",
            R"(
                PROCEDURE lates() {
                    PUTQ one_name
                    GETQ another_name
                }
            )"
        }
    },

    {
        "reset_statement_1",
        {
            "RESET statement",
            R"(
                PROCEDURE lates() {
                    RESET
                }
            )"
        }
    },

    {
        "match_block_1",
        {
            "MATCH block",
            R"(
                PROCEDURE lates() {
                    MATCH { name1 }
                    MATCH { name1 name2 }
                    MATCH { name1[INDEX](expr1+expr2) = (expr3+expr4) }
                }
            )"
        }
    },

    {
        "partial_block_partial_equation_1",
        {
            "PARTIAL block and partial equation statements",
            R"(
                PARTIAL some_name {
                    ~ a' = a*DEL2(b)+c
                    ~ DEL abc[FIRST] = (a*b/c)
                    ~ abc[LAST] = (a*b/c)
                }
            )"
        }
    },

    {
        "linear_block_1",
        {
            "LINEAR block",
            R"(
                LINEAR some_name {
                    ~ I1*bi1+C2*b01-C1*(fi1+f01) = 0
                    ~ C1+C2+C3+C4+C5+O+I1+I2+I3+I4+I5+I6 = 1
                }
            )"
        }
    },

    {
        "nonlinear_block_1",
        {
            "NONLINEAR block with solver for",
            R"(
                NONLINEAR some_name SOLVEFOR a,b {
                    ~ I1*bi1+C2*b01-C1*(fi1+f01) = 0
                    ~ C1+C2+C3+C4+C5+O+I1+I2+I3+I4+I5+I6 = 1
                }
            )"
        }
    },

    /// \todo : NONLIN1 (i.e. ~+) gets replaced with ~. This is not
    ///         a problem as ~ in NONLINEAR block gets replaced with
    ///         ~ anyway. But it would be good to keep same variable
    ///         for nmodl-format utility
    {
        "nonlinear_block_2",
        {
            "NONLINEAR block using symbol",
            R"(
                NONLINEAR some_name SOLVEFOR a,b {
                    ~+ I1*bi1+C2*b01-C1*(fi1+f01) = 0
                    ~+ C1+C2+C3+C4+C5+O+I1+I2+I3+I4+I5+I6 = 1
                }
            )",
            R"(
                NONLINEAR some_name SOLVEFOR a,b {
                    ~ I1*bi1+C2*b01-C1*(fi1+f01) = 0
                    ~ C1+C2+C3+C4+C5+O+I1+I2+I3+I4+I5+I6 = 1
                }
            )"
        }
    },

    {
        "table_statement_1",
        {
            "TABLE statements",
            R"(
                PROCEDURE rates(v) {
                    TABLE RES FROM -20 TO 20 WITH 5000
                    TABLE einf,eexp,etau DEPEND dt,celsius FROM -100 TO 100 WITH 200
                }
            )"
        }
    },

    {
        "watch_statement_1",
        {
            "WATCH statements",
            R"(
                NET_RECEIVE (w) {
                    IF (celltype == 4) {
                        WATCH (v>(vpeak-0.1*u)) 2
                    } ELSE {
                        WATCH (v>vpeak) 2
                    }
                }
            )"
        }
    },

    {
        "watch_statement_2",
        {
            "WATCH statements with multiple expressions",
            R"(
                NET_RECEIVE (w) {
                    IF (celltype == 4) {
                        WATCH (v>vpeak) 2,(v>vpeak) 3.4
                    }
                }
            )"
        }
    },

    {
        "for_netcon_statement_1",
        {
            "FOR_NETCONS statement",
            R"(
                NET_RECEIVE (w(uS), A, tpre(ms)) {
                    FOR_NETCONS (w1, A1, tp) {
                        A1 = A1+(wmax-w1-A1)*p*exp((tp-t)/ptau)
                    }
                }
            )"
        }
    },

    {
        "mutex_statement_1",
        {
            "MUTEX lock/unlock statements",
            R"(
                PROCEDURE fun(w, A, tpre) {
                    MUTEXLOCK
                    MUTEXUNLOCK
                }
            )"
        }
    },

    {
        "protect_statement_1",
        {
            "PROTECT statements",
            R"(
                INITIAL {
                    PROTECT Rtau = 1/(Alpha+Beta)
                }
            )"
        }
    },


    {
        "nonlinear_equation",
        {
            "NONLINEAR block and equation",
            R"(
                NONLINEAR peak {
                    ~ 1/taurise*exp(-tmax/taurise)-afast/taufast*exp(-tmax/taufast)-aslow/tauslow*exp(-tmax/tauslow) = 0
                }
            )"
        }
    },

    {
        "state_block_1",
        {
            "STATE block with variable names",
            R"(
                STATE {
                    m
                    h
                }
            )"
        }
    },

    {
        "state_block_2",
        {
            "STATE block with variable and associated unit",
            R"(
                STATE {
                    a (microsiemens)
                    g (uS)
                }
            )"
        }
    },

    {
        "function_call_1",
        {
            "FUNCTION call",
            R"(
                TERMINAL {
                    a = fun1()
                    b = fun2(a, 2)
                    fun3()
                }
            )"
        }
    },

    {
        "kinetic_block_1",
        {
            "KINETIC block taken from mod file",
            R"(
                KINETIC kin {
                    LOCAL qa
                    qa = q10a^((celsius-22(degC))/10(degC))
                    rates(v)
                    ~ c <-> o (alpha, beta)
                    ~ c <-> cac (kon*qa*ai/bf, koff*qa*b/bf)
                    ~ o <-> cao (kon*qa*ai, koff*qa)
                    ~ cac <-> cao (alphaa, betaa)
                    CONSERVE c+cac+o+cao = 1
                }
            )"
        }
    },

    {
        "thread_safe_1",
        {
            "Theadsafe statement",
            R"(
                NEURON {
                    THREADSAFE
                    THREADSAFE a, b
                }
            )"
        }
    },

    {
        "function_table_1",
        {
            "FUNCTION_TABLE example",
            R"(
                FUNCTION_TABLE tabmtau(v(mV)) (ms)
            )"
        }
    },

    {
        "discrete_block_1",
        {
            "DISCRETE block example",
            R"(
                DISCRETE test {
                    x = 1
                }
            )"
        }
    },

    {
        "derivative_block_1",
        {
            "DERIVATIVE block example",
            R"(
                DERIVATIVE states {
                    rates()
                    m' = (mInf-m)/mTau
                    h' = (hInf-h)/hTau
                }
            )"
        }
    },

    {
        "derivative_block_2",
        {
            "DERIVATIVE block with mixed equations",
            R"(
                DERIVATIVE states {
                    cri' = ((cui-cri)/180-0.5*icr)/(1+Csqn*Kmcsqn/((cri+Kmcsqn)*(cri+Kmcsqn)))
                }
            )"
        }
    },

    {
        "function_1",
        {
            "FUNCTION definition with other function calls",
            R"(
                FUNCTION test(v, x) (ms) {
                    IF (2<x) {
                        hello(1, 2)
                    }
                }
            )"
        }
    },

    {
        "before_block_1",
        {
            "BEFORE block example",
            R"(
                BEFORE BREAKPOINT {
                }
            )"
        }
    },

    {
        "after_block_1",
        {
            "AFTER block example",
            R"(
                AFTER STEP {
                    x = 1
                }
            )"
        }
    },

    {
        "assignment_statement_1",
        {
            "Various assignement statement types",
            R"(
                AFTER STEP {
                    x = 1
                    y[1+2] = z
                    a@1 = 12
                    a@2[1+2] = 21.1
                }
            )"
        }
    },

    {
        "steadystate_statement_1",
        {
            "SOLVE statement using STEADYSTATE",
            R"(
                INITIAL {
                    SOLVE kin STEADYSTATE sparse
                }
            )"
        }
    },

    {
        "netreceive_block_1",
        {
            "NET_RECEIVE block containing INITIAL block",
            R"(
                NET_RECEIVE (weight, weight_AMPA, weight_NMDA, Psurv, tsyn) {
                    LOCAL result
                    weight_AMPA = weight
                    weight_NMDA = weight*NMDA_ratio
                    INITIAL {
                        tsyn = t
                    }
                }
            )"
        }
    },

    /// \todo : support for comment parsing is not working
    {
        "inline_comment_1",
        {
            "Comments with ? or : are not handled yet",
            R"(
                ? comment here
                FUNCTION urand() {
                    VERBATIM
                        printf("Hello World!\n");
                    ENDVERBATIM
                }
            )",
            R"(
                FUNCTION urand() {
                    VERBATIM
                        printf("Hello World!\n");
                    ENDVERBATIM
                }
            )"
        }
    },

    {
        "inline_comment_2",
        {
            "Inline comments with ? or : are not handled yet",
            R"(
                FUNCTION urand() {
                    a = b+c  : some comment here
                    c = d*e  ? another comment here
                }
            )",
            R"(
                FUNCTION urand() {
                    a = b+c
                    c = d*e
                }
            )"
        }
    },
    {
        "section_test",
        {
            "Section token test",
            R"(
                NEURON {
                    SECTION a, b
                }
            )"
        }
    },
    {
        "empty_unit_declaration",
        {
            "Declaration with empty units",
            R"(
                FUNCTION ssCB(kdf(), kds()) (mM) {
                }
            )"
        }
    }
    // clang-format on
};


std::vector<DiffEqTestCase> diff_eq_constructs{
    // clang-format off

        /// differential equations from BlueBrain mod files including latest V6 branch

        {
            "NaTs2_t.mod",
            "m' = (mInf-m)/mTau",
            "m = m+(1.0-exp(dt*((((-1.0)))/mTau)))*(-(((mInf))/mTau)/((((-1.0)))/mTau)-m)",
            "cnexp"
        },

        {
            "tmgExSyn.mod",
            "A' = -A/tau_r",
            "A = A+(1.0-exp(dt*((-1.0)/tau_r)))*(-(0.0)/((-1.0)/tau_r)-A)",
            "cnexp"
        },

        {
            "CaDynamics_DC0.mod",
            "cai' = -(10000)*ica*surftovol*gamma/(2*FARADAY) - (cai - minCai)/decay",
            "cai = cai+(1.0-exp(dt*((-((1.0))/decay))))*(-(((((-(10000))*(ica))*(surftovol))*(gamma))/(2*FARADAY)-(((-minCai)))/decay)/((-((1.0))/decay))-cai)",
            "cnexp"
        },

        {
            "CaDynamics_DC1.mod",
            "cai' = -(10000)*ica*surftovol*gamma/(2*FARADAY) - (cai - minCai)/decay * diamref * surftovol",
            "cai = cai+(1.0-exp(dt*((-((((1.0))/decay)*(diamref))*(surftovol)))))*(-(((((-(10000))*(ica))*(surftovol))*(gamma))/(2*FARADAY)-(((((-minCai)))/decay)*(diamref))*(surftovol))/((-((((1.0))/decay)*(diamref))*(surftovol)))-cai)",
            "cnexp"
        },

        {
            "DetAMPANMDA.mod",
            "A_AMPA' = -A_AMPA/tau_r_AMPA",
            "A_AMPA = A_AMPA+(1.0-exp(dt*((-1.0)/tau_r_AMPA)))*(-(0.0)/((-1.0)/tau_r_AMPA)-A_AMPA)",
            "cnexp"
        },

        {
            "StochKv.mod",
            "n' = a - (a + b)*n",
            "n = n+(1.0-exp(dt*((-((a+b))*(1.0)))))*(-(a)/((-((a+b))*(1.0)))-n)",
            "cnexp"
        },

        {
            "StochKv2.mod",
            "l' = al - (al + bl)*l",
            "l = l+(1.0-exp(dt*((-((al+bl))*(1.0)))))*(-(al)/((-((al+bl))*(1.0)))-l)",
            "cnexp"
        },

         {
            "_calcium.mod",
            "mcal' = ((inf-mcal)/tau)",
            "mcal = mcal+(1.0-exp(dt*(((((-1.0)))/tau))))*(-((((inf))/tau))/(((((-1.0)))/tau))-mcal)",
            "cnexp"
        },

        {
            "_calciumc_concentration.mod",
            "cai' = -(ica*surftovol/FARADAY) - cai/decay",
            "cai = cai+(1.0-exp(dt*((-(1.0)/decay))))*(-(-(((ica)*(surftovol))/FARADAY))/((-(1.0)/decay))-cai)",
            "cnexp"
        },

        {
            "_outside_calcium_concentration.mod",
            "cao' = ica*(1e8)/(fhspace*FARADAY) + (cabath-cao)/trans",
            "cao = cao+(1.0-exp(dt*((((-1.0)))/trans)))*(-(((ica)*((1e8)))/(fhspace*FARADAY)+((cabath))/trans)/((((-1.0)))/trans)-cao)",
            "cnexp"
        },


        /// glusynapse.mod for plasticity simulations


        /// using cnexp method

        {
            "GluSynapse.mod",
            "A_AMPA' = -A_AMPA/tau_r_AMPA",
            "A_AMPA = A_AMPA+(1.0-exp(dt*((-1.0)/tau_r_AMPA)))*(-(0.0)/((-1.0)/tau_r_AMPA)-A_AMPA)",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "m_VDCC' = (minf_VDCC-m_VDCC)/mtau_VDCC",
            "m_VDCC = m_VDCC+(1.0-exp(dt*((((-1.0)))/mtau_VDCC)))*(-(((minf_VDCC))/mtau_VDCC)/((((-1.0)))/mtau_VDCC)-m_VDCC)",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "cai_CR' = -(1e-9)*(ica_NMDA + ica_VDCC)*gamma_ca_CR/((1e-15)*volume_CR*2*FARADAY) - (cai_CR - min_ca_CR)/tau_ca_CR",
            "cai_CR = cai_CR+(1.0-exp(dt*((-((1.0))/tau_ca_CR))))*(-((((-(1e-9))*((ica_NMDA+ica_VDCC)))*(gamma_ca_CR))/((1e-15)*volume_CR*2*FARADAY)-(((-min_ca_CR)))/tau_ca_CR)/((-((1.0))/tau_ca_CR))-cai_CR)",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "effcai_GB' = -0.005*effcai_GB + (cai_CR - min_ca_CR)",
            "effcai_GB = effcai_GB+(1.0-exp(dt*((-0.005)*(1.0))))*(-((cai_CR-min_ca_CR))/((-0.005)*(1.0))-effcai_GB)",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "Rho_GB' = ( - Rho_GB*(1-Rho_GB)*(rho_star_GB-Rho_GB) + potentiate_GB*gamma_p_GB*(1-Rho_GB) - depress_GB* gamma_d_GB*Rho_GB ) / ((1e3)*tau_GB)",
            "Rho_GB = Rho_GB+(1.0-exp(dt*(((((((-1.0)*((1-Rho_GB))+(-Rho_GB)*(((-1.0)))))*((rho_star_GB-Rho_GB))+(-Rho_GB*(1-Rho_GB))*(((-1.0))))+(potentiate_GB*gamma_p_GB)*(((-1.0)))-(depress_GB*gamma_d_GB)*(1.0)))/((1e3)*tau_GB))))*(-Rho_GB)",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "Use_GB' = (Use_d_GB + Rho_GB*(Use_p_GB-Use_d_GB) - Use_GB) / ((1e3)*tau_Use_GB)",
            "Use_GB = Use_GB+(1.0-exp(dt*((((-1.0)))/((1e3)*tau_Use_GB))))*(-(((Use_d_GB+(Rho_GB)*((Use_p_GB-Use_d_GB))))/((1e3)*tau_Use_GB))/((((-1.0)))/((1e3)*tau_Use_GB))-Use_GB)",
            "cnexp"
        },

        /// some made-up examples to test cnexp solver implementation

        {
            "GluSynapse.mod",
            "A_AMPA' = fun(tau_r_AMPA)",
            "A_AMPA = A_AMPA-dt*(-(fun(tau_r_AMPA)))",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "A_AMPA' = fun(tau_r_AMPA, B_AMPA) + B_AMPA",
            "A_AMPA = A_AMPA-dt*(-(fun(tau_r_AMPA,B_AMPA)+B_AMPA))",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "A_AMPA' = tau_r_AMPA/A_AMPA",
            "DA_AMPA = DA_AMPA/(1.0-dt*(((tau_r_AMPA/(A_AMPA+0.001))-(tau_r_AMPA/A_AMPA))/0.001))",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "A_AMPA' = A_AMPA*A_AMPA",
            "A_AMPA = A_AMPA+(1.0-exp(dt*(((1.0)*(A_AMPA)+(A_AMPA)*(1.0)))))*(-A_AMPA)",
            "cnexp"
        },

        {
            "GluSynapse.mod",
            "A_AMPA' = tau_r_AMPA/A_AMPA",
            "DA_AMPA = DA_AMPA/(1.0-dt*(((tau_r_AMPA/(A_AMPA+0.001))-(tau_r_AMPA/A_AMPA))/0.001))",
            "derivimplicit"
        },

        {
            "GluSynapse.mod",
            "A_AMPA' = A_AMPA*A_AMPA",
            "A_AMPA = A_AMPA+dt*(A_AMPA*A_AMPA)",
            "euler"
        },


        /// using derivimplicit method
        /// note that the equation in state block gets changed by replacing state variable with Dstate.
        /// below expressions are from ode_matsol1 method

        {
                "GluSynapse.mod",
                "A_AMPA' = -A_AMPA/tau_r_AMPA",
                "DA_AMPA = DA_AMPA/(1.0-dt*((-1.0)/tau_r_AMPA))",
                "derivimplicit"
        },

        {
                "GluSynapse.mod",
                "m_VDCC' = (minf_VDCC-m_VDCC)/mtau_VDCC",
                "Dm_VDCC = Dm_VDCC/(1.0-dt*((((-1.0)))/mtau_VDCC))",
                "derivimplicit"
        },

        {
                "GluSynapse.mod",
                "cai_CR' = -(1e-9)*(ica_NMDA + ica_VDCC)*gamma_ca_CR/((1e-15)*volume_CR*2*FARADAY) - (cai_CR - min_ca_CR)/tau_ca_CR",
                "Dcai_CR = Dcai_CR/(1.0-dt*((-((1.0))/tau_ca_CR)))",
                "derivimplicit"
        },

        {
                "GluSynapse.mod",
                "effcai_GB' = -0.005*effcai_GB + (cai_CR - min_ca_CR)",
                "Deffcai_GB = Deffcai_GB/(1.0-dt*((-0.005)*(1.0)))",
                "derivimplicit"
        },

        {
                "GluSynapse.mod",
                "Rho_GB' = ( - Rho_GB*(1-Rho_GB)*(rho_star_GB-Rho_GB) + potentiate_GB*gamma_p_GB*(1-Rho_GB) - depress_GB*gamma_d_GB*Rho_GB ) / ((1e3)*tau_GB)",
                "DRho_GB = DRho_GB/(1.0-dt*(((((((-1.0)*((1-Rho_GB))+(-Rho_GB)*(((-1.0)))))*((rho_star_GB-Rho_GB))+(-Rho_GB*(1-Rho_GB))*(((-1.0))))+(potentiate_GB*gamma_p_GB)*(((-1.0)))-(depress_GB*gamma_d_GB)*(1.0)))/((1e3)*tau_GB)))",
                "derivimplicit"
        },

        {
                "GluSynapse.mod",
                "Use_GB' = (Use_d_GB + Rho_GB*(Use_p_GB-Use_d_GB) - Use_GB) / ((1e3)*tau_Use_GB)",
                "DUse_GB = DUse_GB/(1.0-dt*((((-1.0)))/((1e3)*tau_Use_GB)))",
                "derivimplicit"
        },


        /// using euler method : solutions are same as derivimplicit method

        {
                "GluSynapse.mod",
                "A_AMPA' = -A_AMPA/tau_r_AMPA",
                "A_AMPA = A_AMPA+dt*(-A_AMPA/tau_r_AMPA)",
                "euler"
        },

        {
                "GluSynapse.mod",
                "m_VDCC' = (minf_VDCC-m_VDCC)/mtau_VDCC",
                "m_VDCC = m_VDCC+dt*((minf_VDCC-m_VDCC)/mtau_VDCC)",
                "euler"
        },

        {
                "GluSynapse.mod",
                "cai_CR' = -(1e-9)*(ica_NMDA + ica_VDCC)*gamma_ca_CR/((1e-15)*volume_CR*2*FARADAY) - (cai_CR - min_ca_CR)/tau_ca_CR",
                "cai_CR = cai_CR+dt*(-(1e-9)*(ica_NMDA + ica_VDCC)*gamma_ca_CR/((1e-15)*volume_CR*2*FARADAY) - (cai_CR - min_ca_CR)/tau_ca_CR)",
                "euler"
        },

        {
                "GluSynapse.mod",
                "effcai_GB' = -0.005*effcai_GB + (cai_CR - min_ca_CR)",
                "effcai_GB = effcai_GB+dt*(-0.005*effcai_GB + (cai_CR - min_ca_CR))",
                "euler"
        },

        {
                "GluSynapse.mod",
                "Rho_GB' = ( - Rho_GB*(1-Rho_GB)*(rho_star_GB-Rho_GB) + potentiate_GB*gamma_p_GB*(1-Rho_GB) - depress_GB*gamma_d_GB*Rho_GB ) / ((1e3)*tau_GB)",
                "Rho_GB = Rho_GB+dt*(( - Rho_GB*(1-Rho_GB)*(rho_star_GB-Rho_GB) + potentiate_GB*gamma_p_GB*(1-Rho_GB) - depress_GB*gamma_d_GB*Rho_GB ) / ((1e3)*tau_GB))",
                "euler"
        },

        {
                "GluSynapse.mod",
                "Use_GB' = (Use_d_GB + Rho_GB*(Use_p_GB-Use_d_GB) - Use_GB) / ((1e3)*tau_Use_GB)",
                "Use_GB = Use_GB+dt*((Use_d_GB + Rho_GB*(Use_p_GB-Use_d_GB) - Use_GB) / ((1e3)*tau_Use_GB))",
                "euler"
        },


        /// equations of nonlinear from taken from nocmodlx/test/input/usecases/nonlinear
        /// using derivimplicit and euler method

        {
            "wc.mod",
            "uu' = -uu+f(aee*uu-aie*vv-ze+i_e)",
            "Duu = Duu/(1.0-dt*(((-(uu+0.001)+f(aee*(uu+0.001)-aie*vv-ze+i_e))-(-uu+f(aee*uu-aie*vv-ze+i_e)))/0.001))",
            "derivimplicit"
        },

        {
            "wc.mod",
            "vv' = (-vv+f(aei*uu-aii*vv-zi+i_i))/tau",
            "Dvv = Dvv/(1.0-dt*((((-(vv+0.001)+f(aei*uu-aii*(vv+0.001)-zi+i_i))/tau)-((-vv+f(aei*uu-aii*vv-zi+i_i))/tau))/0.001))",
            "derivimplicit"
        },

        {
            "ER.mod",
            "caer' = -(0.001)*( Jip3(cali,caer, ip3ip, Vip3, dact, dinh, dip3, ddis)+errel(cali,caer,kerrel,kerm)-erfil(cali,caer,kerfila,kerfilb)+erlek(cali,caer,kerlek))/(rhover/fer)",
            "Dcaer = Dcaer/(1.0-dt*(((-(0.001)*(Jip3(cali,(caer+0.001),ip3ip,Vip3,dact,dinh,dip3,ddis)+errel(cali,(caer+0.001),kerrel,kerm)-erfil(cali,(caer+0.001),kerfila,kerfilb)+erlek(cali,(caer+0.001),kerlek))/(rhover/fer))-(-(0.001)*(Jip3(cali,caer,ip3ip,Vip3,dact,dinh,dip3,ddis)+errel(cali,caer,kerrel,kerm)-erfil(cali,caer,kerfila,kerfilb)+erlek(cali,caer,kerlek))/(rhover/fer)))/0.001))",
            "derivimplicit"
        },

        {
            "ER.mod",
            "Jip3h' = (Jip3hinf(ip3ip, cali, dinh, dip3, ddis)-Jip3h)/Jip3th(ip3ip, ainh, cali, dinh, dip3, ddis)",
            "DJip3h = DJip3h/(1.0-dt*((((-1.0)))/Jip3th(ip3ip,ainh,cali,dinh,dip3,ddis)))",
            "derivimplicit"
        },

        {
            "gr_ltp1.mod",
            "messenger' = -gamma*picanmda - eta*messenger",
            "Dmessenger = Dmessenger/(1.0-dt*((-(eta)*(1.0))))",
            "derivimplicit"
        },

        {
            "gr_ltp1.mod",
            "Np' = nu1*messenger  - (pp - picanmda*gdel1)*Np +(Mp*Np*Np)/(Ap+Np*Np)",
            "DNp = DNp/(1.0-dt*(((nu1*messenger-(pp-picanmda*gdel1)*(Np+0.001)+(Mp*(Np+0.001)*(Np+0.001))/(Ap+(Np+0.001)*(Np+0.001)))-(nu1*messenger-(pp-picanmda*gdel1)*Np+(Mp*Np*Np)/(Ap+Np*Np)))/0.001))",
            "derivimplicit"
        },

        {
            "cajsracc.mod",
            "cri' = ((cui - cri)/180 - 0.5*icr)/(1 + Csqn*Kmcsqn/((cri + Kmcsqn)*(cri + Kmcsqn)))",
            "Dcri = Dcri/(1.0-dt*(((((cui-(cri+0.001))/180-0.5*icr)/(1+Csqn*Kmcsqn/(((cri+0.001)+Kmcsqn)*((cri+0.001)+Kmcsqn))))-(((cui-cri)/180-0.5*icr)/(1+Csqn*Kmcsqn/((cri+Kmcsqn)*(cri+Kmcsqn)))))/0.001))",
            "derivimplicit"
        },

        {
            "syn_bip_gan.mod",
            "s' = (s_inf-s)/((1-s_inf)*tau*s)",
            "s = s+dt*((s_inf-s)/((1-s_inf)*tau*s))",
            "euler"
        },

        {
            "syn_rod_bip.mod",
            "s' = (s_inf-s)/((1-s_inf)*tau*s)",
            "s = s+dt*((s_inf-s)/((1-s_inf)*tau*s))",
            "euler"
        },

    // clang-format on
};

}  // namespace test_utils
}  // namespace nmodl
