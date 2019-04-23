/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <string>


namespace nmodl {
namespace codegen {
/// different variable names used in code generation
namespace naming {

/// nmodl language version
/// @todo : should be moved from codegen to global scope
const std::string NMODL_VERSION("6.2.0");

/// derivimplicit method in nmodl
const std::string DERIVIMPLICIT_METHOD("derivimplicit");

/// euler method in nmodl
const std::string EULER_METHOD("euler");

/// cnexp method in nmodl
const std::string CNEXP_METHOD("cnexp");

/// sparse method in nmodl
const std::string SPARSE_METHOD("sparse");

/// net_event function call in nmodl
const std::string NET_EVENT_METHOD("net_event");

/// net_move function call in nmodl
const std::string NET_MOVE_METHOD("net_move");

/// net_send function call in nmodl
const std::string NET_SEND_METHOD("net_send");

/// artificial cell keyword in nmodl
const std::string ARTIFICIAL_CELL("ARTIFICIAL_CELL");

/// point process keyword in nmodl
const std::string POINT_PROCESS("POINT_PROCESS");

/// inbuilt neuron variable for diameter of the compartment
const std::string DIAM_VARIABLE("diam");

/// inbuilt neuron variable for area of the compartment
const std::string NODE_AREA_VARIABLE("node_area");

/// similar to node_area but user can explicitly declare it as area
const std::string AREA_VARIABLE("area");

/// inbuilt neuron variable for point process
const std::string POINT_PROCESS_VARIABLE("point_process");

/// inbuilt neuron variable for tqitem process
const std::string TQITEM_VARIABLE("tqitem");

/// range variable for conductance
const std::string CONDUCTANCE_VARIABLE("_g");

/// global variable to indicate if table is used
const std::string USE_TABLE_VARIABLE("usetable");

/// range variable when conductance is not used (for vectorized model)
const std::string CONDUCTANCE_UNUSED_VARIABLE("g_unused");

/// range variable for voltage when unused (for vectorized model)
const std::string VOLTAGE_UNUSED_VARIABLE("v_unused");

/// variable t indicating last execution time of net receive block
const std::string T_SAVE_VARIABLE("tsave");

/// shadow rhs variable in neuron thread structure
const std::string NTHREAD_RHS_SHADOW("_shadow_rhs");

/// shadow d variable in neuron thread structure
const std::string NTHREAD_D_SHADOW("_shadow_d");

/// t variable in neuron thread structure
const std::string NTHREAD_T_VARIABLE("t");

/// dt variable in neuron thread structure
const std::string NTHREAD_DT_VARIABLE("dt");

/// default float variable type
const std::string DEFAULT_FLOAT_TYPE("double");

/// default local variable type
const std::string DEFAULT_LOCAL_VAR_TYPE("double");

/// default integer variable type
const std::string DEFAULT_INTEGER_TYPE("int");

/// semantic type for area variable
const std::string AREA_SEMANTIC("area");

/// semantic type for point process variable
const std::string POINT_PROCESS_SEMANTIC("pntproc");

/// semantic type for pointer variable
const std::string POINTER_SEMANTIC("pointer");

/// semantic type for core pointer variable
const std::string CORE_POINTER_SEMANTIC("bbcorepointer");

/// semantic type for net send call
const std::string NET_SEND_SEMANTIC("netsend");

/// semantic type for watch statement
const std::string WATCH_SEMANTIC("watch");

/// semantic type for for_netcon statement
const std::string FOR_NETCON_SEMANTIC("fornetcon");

/// nrn_init method in generated code
const std::string NRN_INIT_METHOD("nrn_init");

/// nrn_alloc method in generated code
const std::string NRN_ALLOC_METHOD("nrn_alloc");

/// nrn_state method in generated code
const std::string NRN_STATE_METHOD("nrn_state");

/// nrn_cur method in generated code
const std::string NRN_CUR_METHOD("nrn_cur");

/// nrn_watch_check method in generated c file
const std::string NRN_WATCH_CHECK_METHOD("nrn_watch_check");

/// verbatim name of the variable for nrn thread arguments
const std::string THREAD_ARGS("_threadargs_");

/// verbatim name of the variable for nrn thread arguments in prototype
const std::string THREAD_ARGS_PROTO("_threadargsproto_");

/// commonly used variables in verbatim block and how they
/// should be mapped to new code generation backends
// clang-format off
        const std::map<std::string, std::string> VERBATIM_VARIABLES_MAPPING{
                {"_nt", "nt"},
                {"_p", "data"},
                {"_ppvar", "indexes"},
                {"_thread", "thread"},
                {"_iml", "id"},
                {"_cntml_padded", "pnodecount"},
                {"_cntml", "nodecount"},
                {"_tqitem", "tqitem"}};
// clang-format on

}  // namespace naming
}  // namespace codegen
}  // namespace nmodl
