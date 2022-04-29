/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
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
static constexpr char NMODL_VERSION[] = "6.2.0";

/// derivimplicit method in nmodl
static constexpr char DERIVIMPLICIT_METHOD[] = "derivimplicit";

/// euler method in nmodl
static constexpr char EULER_METHOD[] = "euler";

/// cnexp method in nmodl
static constexpr char CNEXP_METHOD[] = "cnexp";

/// cvode method in nmodl
static constexpr char AFTER_CVODE_METHOD[] = "after_cvode";

/// sparse method in nmodl
static constexpr char SPARSE_METHOD[] = "sparse";

/// net_event function call in nmodl
static constexpr char NET_EVENT_METHOD[] = "net_event";

/// net_move function call in nmodl
static constexpr char NET_MOVE_METHOD[] = "net_move";

/// net_send function call in nmodl
static constexpr char NET_SEND_METHOD[] = "net_send";

/// artificial cell keyword in nmodl
static constexpr char ARTIFICIAL_CELL[] = "ARTIFICIAL_CELL";

/// point process keyword in nmodl
static constexpr char POINT_PROCESS[] = "POINT_PROCESS";

/// inbuilt neuron variable for diameter of the compartment
static constexpr char DIAM_VARIABLE[] = "diam";

/// inbuilt neuron variable for area of the compartment
static constexpr char NODE_AREA_VARIABLE[] = "node_area";

/// similar to node_area but user can explicitly declare it as area
static constexpr char AREA_VARIABLE[] = "area";

/// inbuilt neuron variable for point process
static constexpr char POINT_PROCESS_VARIABLE[] = "point_process";

/// inbuilt neuron variable for tqitem process
static constexpr char TQITEM_VARIABLE[] = "tqitem";

/// range variable for conductance
static constexpr char CONDUCTANCE_VARIABLE[] = "g";

/// global variable to indicate if table is used
static constexpr char USE_TABLE_VARIABLE[] = "usetable";

/// range variable when conductance is not used (for vectorized model)
static constexpr char CONDUCTANCE_UNUSED_VARIABLE[] = "g_unused";

/// range variable for voltage when unused (for vectorized model)
static constexpr char VOLTAGE_UNUSED_VARIABLE[] = "v_unused";

/// variable t indicating last execution time of net receive block
static constexpr char T_SAVE_VARIABLE[] = "tsave";

/// shadow rhs variable in neuron thread structure
static constexpr char NTHREAD_RHS_SHADOW[] = "_shadow_rhs";

/// shadow d variable in neuron thread structure
static constexpr char NTHREAD_D_SHADOW[] = "_shadow_d";

/// t variable in neuron thread structure
static constexpr char NTHREAD_T_VARIABLE[] = "t";

/// dt variable in neuron thread structure
static constexpr char NTHREAD_DT_VARIABLE[] = "dt";

/// default float variable type
static constexpr char DEFAULT_FLOAT_TYPE[] = "double";

/// default local variable type
static constexpr char DEFAULT_LOCAL_VAR_TYPE[] = "double";

/// default integer variable type
static constexpr char DEFAULT_INTEGER_TYPE[] = "int";

/// semantic type for area variable
static constexpr char AREA_SEMANTIC[] = "area";

/// semantic type for point process variable
static constexpr char POINT_PROCESS_SEMANTIC[] = "pntproc";

/// semantic type for pointer variable
static constexpr char POINTER_SEMANTIC[] = "pointer";

/// semantic type for core pointer variable
static constexpr char CORE_POINTER_SEMANTIC[] = "bbcorepointer";

/// semantic type for net send call
static constexpr char NET_SEND_SEMANTIC[] = "netsend";

/// semantic type for watch statement
static constexpr char WATCH_SEMANTIC[] = "watch";

/// semantic type for for_netcon statement
static constexpr char FOR_NETCON_SEMANTIC[] = "fornetcon";

/// nrn_init method in generated code
static constexpr char NRN_INIT_METHOD[] = "nrn_init";

/// nrn_constructor method in generated code
static constexpr char NRN_CONSTRUCTOR_METHOD[] = "nrn_constructor";

/// nrn_destructor method in generated code
static constexpr char NRN_DESTRUCTOR_METHOD[] = "nrn_destructor";

/// nrn_alloc method in generated code
static constexpr char NRN_ALLOC_METHOD[] = "nrn_alloc";

/// nrn_state method in generated code
static constexpr char NRN_STATE_METHOD[] = "nrn_state";

/// nrn_cur method in generated code
static constexpr char NRN_CUR_METHOD[] = "nrn_cur";

/// nrn_watch_check method in generated c file
static constexpr char NRN_WATCH_CHECK_METHOD[] = "nrn_watch_check";

/// verbatim name of the variable for nrn thread arguments
static constexpr char THREAD_ARGS[] = "_threadargs_";

/// verbatim name of the variable for nrn thread arguments in prototype
static constexpr char THREAD_ARGS_PROTO[] = "_threadargsproto_";

/// prefix for ion variable
static constexpr char ION_VARNAME_PREFIX[] = "ion_";


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
