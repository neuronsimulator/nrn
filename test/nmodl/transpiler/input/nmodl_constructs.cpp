#include "input/nmodl_constructs.h"

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
    // clang-format on
};

std::map<std::string, NmodlTestCase> nmodl_valid_constructs{
    // clang-format off
    {
        "title_1",
        {
            "Title statement",
            "TITLE nmodl title\n"
        }
    },

    {
        "local_list_1",
        {
            "standalone LOCAL statement with single scalar variable",
            "LOCAL gbar"
        }
    },

    {
        "local_list_2",
        {
            "standalone LOCAL statement with single vector variable",
            "LOCAL gbar[2]"
        }
    },

    {
        "local_list_3",
        {
            "standalone LOCAL statement with multiple variables",
            "LOCAL gbar, ek[2], ik"
        }
    },

    {
        "define_1",
        {
            "Macro definition",
            "DEFINE NSTEP 10"
        }
    },

    {
        "model_level_1",
        {
            "Model level followed by block",
            "MODEL_LEVEL 2 NEURON {}"
        }
    },

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
            R"(INCLUDE  "Unit.inc" )"
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
                    tau_d_AMPA = 10.0 (mV)
                    tau_r_NMDA = 10 (mV) <1,2>
                    tau_d_NMDA = 10 (mV) <1.1,2.2>
                    Use (mV)
                    Dep [1] <1,2>
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
        "step_block_1",
        {
            "STEP block with all statement types",
            R"(
                STEPPED {
                  tau_r_AMPA = 1 , -2
                  tau_d_AMPA = 1.0,-2.0
                  tau_r_NMDA = 1,2.0,3 (mV)
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
                    i_NMDA START 2.0 (nA) <1.0>
                    A_NMDA_step[1] START 1 <2.0>
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
                    USEION na READ ena WRITE ina
                    NONSPECIFIC_CURRENT i
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
    // clang-format on
};