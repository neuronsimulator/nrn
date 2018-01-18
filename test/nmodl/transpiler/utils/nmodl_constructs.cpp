#include "test/utils/nmodl_constructs.h"

/** Guidelines for adding nmodl text constructs
 *
 * As nmodl constructs are used to for testing ast to nmodl transformations,
 * consider following points:
 *
 *  - Leading whitespaces or empty lines are removed
 *
 *  - Use string literal to define nmodl text
 *    When ast is transformed back to nmodl, each statement has newline.
 *    Hence for easy comparison, input nmodl should be null terminated.
 *    One way to use format:

      R"(
            TITLE nmodl title
      )"

 *  - Do not use extra spaces (even though it's valid)

     LOCAL a,b

     instead of

     LOCAL  a, b,   c

 *  - Use well indented blocks

    NEURON {
        RANGE x
    }

    instead of

      NEURON  {
        RANGE x
    }

 *
 * If nmodl transformation is different from input, third argument could be
 * provided with the expected nmodl.
 */

std::map<std::string, NmodlTestCase> nmdol_invalid_constructs{
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
        "unit_block_1",
        {
            "UNITS block with empty unit",
            R"(
                UNITS {
                    () = (millivolt)
                }
            )"
        }
    },

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
                    USEION na READ ena
                    USEION na READ ena, kna
                    USEION na WRITE ena, kna VALENCE 2.1
                    USEION na READ ena WRITE ina
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
            "While statement with expression as condition",
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
            "Solve statement with iferor block",
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
            "SOLVE statement using STEADYSTATE (which gets replaced with METHOD)",
            R"(
                INITIAL {
                    SOLVE kin STEADYSTATE sparse
                }
            )",
            R"(
                INITIAL {
                    SOLVE kin METHOD sparse
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
    }
    // clang-format on
};