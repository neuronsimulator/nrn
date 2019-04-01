/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <cmath>
#include <fmt/format.h>
#include <src/visitors/rename_visitor.hpp>

#include "codegen/codegen_ispc_visitor.hpp"
#include "codegen/codegen_naming.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/string_utils.hpp"
#include "visitors/visitor_utils.hpp"

using namespace fmt::literals;

namespace nmodl {
namespace codegen {

using symtab::syminfo::Status;

/****************************************************************************************/
/*                            Overloaded visitor methods                                */
/****************************************************************************************/

/*
 * Rename math functions for ISPC backend
 */
void CodegenIspcVisitor::visit_function_call(ast::FunctionCall* node) {
    if (!codegen) {
        return;
    }
    auto fname = node->get_name().get();
    RenameVisitor("fabs", "abs").visit_name(fname);
    RenameVisitor("exp", "vexp").visit_name(fname);
    CodegenCVisitor::visit_function_call(node);
}


/*
 * Rename special global variables
 */
void CodegenIspcVisitor::visit_var_name(ast::VarName* node) {
    if (!codegen) {
        return;
    }
    auto celsius_rename = RenameVisitor("celsius", "ispc_celsius");
    node->accept(&celsius_rename);
    CodegenCVisitor::visit_var_name(node);
}


/****************************************************************************************/
/*                      Routines must be overloaded in backend                          */
/****************************************************************************************/

std::string CodegenIspcVisitor::double_to_string(double value) {
    if (std::ceil(value) == value) {
        return "{:.1f}d"_format(value);
    }
    if ((value <= 1.0) && (value >= -1.0)) {
        return "{:f}d"_format(value);
    } else {
        auto e = std::log10(std::abs(value));
        if (e < 0.0) {
            e = std::ceil(-e);
            auto m = std::pow(10, e);
            return "{:f}d-{:d}"_format(value * m, static_cast<int>(e));
        } else {
            e = std::floor(e);
            auto m = std::pow(10, e);
            return "{:f}d{:d}"_format(value / m, static_cast<int>(e));
        }
    }
}


std::string CodegenIspcVisitor::float_to_string(float value) {
    if (std::ceil(value) == value) {
        return "{:.1f}"_format(value);
    }
    if ((value <= 1.0f) && (value >= -1.0f)) {
        return "{:f}"_format(value);
    } else {
        auto e = std::log10(std::abs(value));
        if (e < 0.0f) {
            e = std::ceil(-e);
            auto m = std::pow(10, e);
            return "{:f}e-{:d}"_format(value * m, static_cast<int>(e));
        } else {
            e = std::floor(e);
            auto m = std::pow(10, e);
            return "{:f}e{:d}"_format(value / m, static_cast<int>(e));
        }
    }
}


std::string CodegenIspcVisitor::compute_method_name(BlockType type) {
    if (type == BlockType::Initial) {
        return method_name("ispc_nrn_init");
    }
    if (type == BlockType::State) {
        return method_name("ispc_nrn_state");
    }
    if (type == BlockType::Equation) {
        return method_name("ispc_nrn_cur");
    }
    throw std::runtime_error("compute_method_name not implemented");
}


std::string CodegenIspcVisitor::net_receive_buffering_declaration() {
    auto params = ParamVector();
    params.emplace_back(param_type_qualifier(), "{}*"_format(instance_struct()),
                        param_ptr_qualifier(), "inst");
    params.emplace_back(param_type_qualifier(), "NrnThread*", param_ptr_qualifier(), "nt");
    params.emplace_back(param_type_qualifier(), "Memb_list*", param_ptr_qualifier(), "ml");

    return "export void {}({})"_format(method_name("ispc_net_buf_receive"),
                                       get_parameter_str(params));
}


void CodegenIspcVisitor::print_backend_includes() {
    printer->add_line("#include \"nmodl/fast_math.ispc\"");
    printer->add_line("#include \"coreneuron/nrnoc/nrnoc_ml.ispc\"");
    printer->add_newline();
    printer->add_newline();
}


std::string CodegenIspcVisitor::backend_name() {
    return "ispc (api-compatibility)";
}


void CodegenIspcVisitor::print_atomic_op(const std::string& lhs,
                                         const std::string& op,
                                         const std::string& rhs) {
    std::string function;
    if (op == "+") {
        function = "atomic_add_local";
    } else if (op == "-") {
        function = "atomic_subtract_local";
    } else {
        throw std::runtime_error("ISPC backend error : {} not supported"_format(op));
    }
    printer->add_line("{}(&{}, {});"_format(function, lhs, rhs));
}


void CodegenIspcVisitor::print_nrn_cur_matrix_shadow_update() {
    auto rhs_op = operator_for_rhs();
    auto d_op = operator_for_d();
    if (info.point_process) {
        stringutils::remove_character(rhs_op, '=');
        stringutils::remove_character(d_op, '=');
        print_atomic_op("vec_rhs[node_id]", rhs_op, "rhs");
        print_atomic_op("vec_d[node_id]", d_op, "g");
    } else {
        printer->add_line("vec_rhs[node_id] {} rhs;"_format(rhs_op));
        printer->add_line("vec_d[node_id] {} g;"_format(d_op));
    }
}


void CodegenIspcVisitor::print_channel_iteration_tiling_block_begin(BlockType type) {
    // no tiling for ispc backend but make sure variables are declared as uniform
    printer->add_line("int uniform start = 0;");
    printer->add_line("int uniform end = nodecount;");
}


/*
 * Depending on the backend, print condition/loop for iterating over channels
 *
 * Use ispc foreach loop
 */
void CodegenIspcVisitor::print_channel_iteration_block_begin() {
    printer->start_block("foreach (id = start ... end)");
}


void CodegenIspcVisitor::print_channel_iteration_block_end() {
    printer->end_block();
    printer->add_newline();
}


void CodegenIspcVisitor::print_net_receive_loop_begin() {
    printer->add_line("uniform int count = nrb->_displ_cnt;");
    printer->start_block("foreach (i = 0 ... count)");
}


void CodegenIspcVisitor::print_get_memb_list() {
    // do nothing
}


void CodegenIspcVisitor::print_nrn_cur_matrix_shadow_reduction() {
    // do nothing
}


void CodegenIspcVisitor::print_rhs_d_shadow_variables() {
    // do nothing
}


bool CodegenIspcVisitor::nrn_cur_reduction_loop_required() {
    return false;
}


std::string CodegenIspcVisitor::ptr_type_qualifier() {
    if (wrapper_codegen) {
        return CodegenCVisitor::ptr_type_qualifier();
    } else {
        return "uniform ";  // @note: extra space needed to separate qualifier from var name.
    }
}


std::string CodegenIspcVisitor::param_type_qualifier() {
    if (wrapper_codegen) {
        return CodegenCVisitor::param_type_qualifier();
    } else {
        return "uniform ";
    }
}


std::string CodegenIspcVisitor::param_ptr_qualifier() {
    if (wrapper_codegen) {
        return CodegenCVisitor::param_ptr_qualifier();
    } else {
        return "uniform ";
    }
}


void CodegenIspcVisitor::print_backend_namespace_start() {
    // no ispc namespace
}


void CodegenIspcVisitor::print_backend_namespace_stop() {
    // no ispc namespace
}


CodegenIspcVisitor::ParamVector CodegenIspcVisitor::get_global_function_parms(
    std::string arg_qualifier) {
    auto params = ParamVector();
    params.emplace_back(param_type_qualifier(), "{}*"_format(instance_struct()),
                        param_ptr_qualifier(), "inst");
    params.emplace_back(param_type_qualifier(), "NrnThread*", param_ptr_qualifier(), "nt");
    params.emplace_back(param_type_qualifier(), "Memb_list*", param_ptr_qualifier(), "ml");
    params.emplace_back(param_type_qualifier(), "int", "", "type");
    return params;
}


void CodegenIspcVisitor::print_global_function_common_code(BlockType type) {
    std::string method = compute_method_name(type);

    auto params = get_global_function_parms(ptr_type_qualifier());
    print_global_method_annotation();
    printer->start_block("export void {}({})"_format(method, get_parameter_str(params)));

    print_kernel_data_present_annotation_block_begin();
    printer->add_line("uniform int nodecount = ml->nodecount;");
    printer->add_line("uniform int pnodecount = ml->_nodecount_padded;");
    printer->add_line(
        "{}int* {}node_index = ml->nodeindices;"_format(k_const(), ptr_type_qualifier()));
    printer->add_line("double* {}data = ml->data;"_format(ptr_type_qualifier()));
    printer->add_line(
        "{}double* {}voltage = nt->_actual_v;"_format(k_const(), ptr_type_qualifier()));

    if (type == BlockType::Equation) {
        printer->add_line("double* {}vec_rhs = nt->_actual_rhs;"_format(ptr_type_qualifier()));
        printer->add_line("double* {}vec_d = nt->_actual_d;"_format(ptr_type_qualifier()));
        print_rhs_d_shadow_variables();
    }
    printer->add_line("Datum* {}indexes = ml->pdata;"_format(ptr_type_qualifier()));
    printer->add_line("ThreadDatum* {}thread = ml->_thread;"_format(ptr_type_qualifier()));
    printer->add_newline(1);
}


void CodegenIspcVisitor::print_compute_functions() {
    // print_top_verbatim_blocks(); @todo: see where to add this

    for (const auto& function: info.functions) {
        if (!program_symtab->lookup(function->get_node_name())
                 .get()
                 ->has_all_status(Status::inlined)) {
            print_function(function);
        }
    }
    for (const auto& procedure: info.procedures) {
        if (!program_symtab->lookup(procedure->get_node_name())
                 .get()
                 ->has_all_status(Status::inlined)) {
            print_procedure(procedure);
        }
    }
    print_net_receive_kernel();
    print_net_receive_buffering(false);
    print_nrn_init(false);
    print_nrn_cur();
    print_nrn_state();
}


// @todo : use base visitor function with provision to override specific qualifiers
void CodegenIspcVisitor::print_mechanism_global_var_structure() {
    auto float_type = default_float_data_type();
    printer->add_newline(2);
    printer->add_line("/** all global variables */");
    printer->add_line("struct {} {}"_format(global_struct(), "{"));
    printer->increase_indent();

    if (!info.ions.empty()) {
        for (const auto& ion: info.ions) {
            auto name = "{}_type"_format(ion.name);
            printer->add_line("int {};"_format(name));
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    if (info.point_process) {
        printer->add_line("int point_type;");
        codegen_global_variables.push_back(make_symbol("point_type"));
    }

    if (!info.state_vars.empty()) {
        for (const auto& var: info.state_vars) {
            auto name = var->get_name() + "0";
            auto symbol = program_symtab->lookup(name);
            if (symbol == nullptr) {
                printer->add_line("{} {};"_format(float_type, name));
                codegen_global_variables.push_back(make_symbol(name));
            }
        }
    }

    if (!info.vectorize) {
        printer->add_line("{} v;"_format(float_type));
        codegen_global_variables.push_back(make_symbol("v"));
    }

    auto& top_locals = info.top_local_variables;
    if (!info.vectorize && !top_locals.empty()) {
        for (const auto& var: top_locals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->add_line("{} {}[{}];"_format(float_type, name, length));
            } else {
                printer->add_line("{} {};"_format(float_type, name));
            }
            codegen_global_variables.push_back(var);
        }
    }

    if (!info.thread_variables.empty()) {
        printer->add_line("int thread_data_in_use;");
        printer->add_line("{} thread_data[{}];"_format(float_type, info.thread_var_data_size));
        codegen_global_variables.push_back(make_symbol("thread_data_in_use"));
        auto symbol = make_symbol("thread_data");
        symbol->set_as_array(info.thread_var_data_size);
        codegen_global_variables.push_back(symbol);
    }

    printer->add_line("int reset;");
    codegen_global_variables.push_back(make_symbol("reset"));

    printer->add_line("int mech_type;");
    codegen_global_variables.push_back(make_symbol("mech_type"));

    auto& globals = info.global_variables;
    auto& constants = info.constant_variables;

    if (!globals.empty()) {
        for (const auto& var: globals) {
            auto name = var->get_name();
            auto length = var->get_length();
            if (var->is_array()) {
                printer->add_line("{} {}[{}];"_format(float_type, name, length));
            } else {
                printer->add_line("{} {};"_format(float_type, name));
            }
            codegen_global_variables.push_back(var);
        }
    }

    if (!constants.empty()) {
        for (const auto& var: constants) {
            auto name = var->get_name();
            auto value_ptr = var->get_value();
            printer->add_line("{} {};"_format(float_type, name));
            codegen_global_variables.push_back(var);
        }
    }

    if (info.primes_size != 0) {
        printer->add_line("int* {} slist1;"_format(""));
        printer->add_line("int* {} dlist1;"_format(""));
        codegen_global_variables.push_back(make_symbol("slist1"));
        codegen_global_variables.push_back(make_symbol("dlist1"));
        if (info.derivimplicit_used) {
            printer->add_line("int* slist2;");
            codegen_global_variables.push_back(make_symbol("slist2"));
        }
    }

    if (info.table_count > 0) {
        printer->add_line("double usetable;");
        codegen_global_variables.push_back(make_symbol(naming::USE_TABLE_VARIABLE));

        for (const auto& block: info.functions_with_table) {
            auto name = block->get_node_name();
            printer->add_line("{} tmin_{};"_format(float_type, name));
            printer->add_line("{} mfac_{};"_format(float_type, name));
            codegen_global_variables.push_back(make_symbol("tmin_" + name));
            codegen_global_variables.push_back(make_symbol("mfac_" + name));
        }

        for (const auto& variable: info.table_statement_variables) {
            auto name = "t_" + variable->get_name();
            printer->add_line("{}* {};"_format(float_type, name));
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    if (info.vectorize) {
        printer->add_line("ThreadDatum* {}ext_call_thread;"_format(""));
        codegen_global_variables.push_back(make_symbol("ext_call_thread"));
    }

    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(1);
    printer->add_line("/** holds object of global variable */");
    printer->add_line("{} {}_global;"_format(global_struct(), info.mod_suffix));
}


void CodegenIspcVisitor::print_data_structures() {
    print_mechanism_global_var_structure();
    print_mechanism_range_var_structure();
    print_ion_var_structure();
}


void CodegenIspcVisitor::print_wrapper_data_structures() {
    print_mechanism_global_var_structure();
    print_mechanism_range_var_structure();
    print_ion_var_structure();
}


void CodegenIspcVisitor::print_ispc_globals() {
    printer->start_block("extern \"C\"");
    printer->add_line("extern double ispc_celsius;");
    printer->end_block();
}


void CodegenIspcVisitor::print_ion_var_constructor(const std::vector<std::string>& members) {
    /// no constructor for ispc
}


void CodegenIspcVisitor::print_ion_variable() {
    /// c syntax to zero initialize struct
    printer->add_line("IonCurVar ionvar = {0};");
}


/****************************************************************************************/
/*                    Main code printing entry points and wrappers                      */
/****************************************************************************************/

void CodegenIspcVisitor::print_net_receive_buffering_wrapper() {
    if (!net_receive_required() || info.artificial_cell) {
        return;
    }
    printer->add_newline(2);
    printer->start_block("void {}(NrnThread* nt)"_format(method_name("net_buf_receive")));
    printer->add_line("Memb_list* ml = get_memb_list(nt);");
    printer->start_block("if (ml == NULL)");
    printer->add_line("return;");
    printer->end_block(1);
    printer->add_line(
        "{0}* {1}inst = ({0}*) ml->instance;"_format(instance_struct(), ptr_type_qualifier()));

    printer->add_line("{}(inst, nt, ml);"_format(method_name("ispc_net_buf_receive")));

    printer->end_block(1);
}


void CodegenIspcVisitor::print_headers_include() {
    print_backend_includes();
}


void CodegenIspcVisitor::print_wrapper_headers_include() {
    print_standard_includes();
    print_coreneuron_includes();
}


void CodegenIspcVisitor::print_wrapper_routine(std::string wraper_function, BlockType type) {
    auto args = "NrnThread* nt, Memb_list* ml, int type";
    wraper_function = method_name(wraper_function);
    auto compute_function = compute_method_name(type);

    printer->add_newline(2);
    printer->start_block("void {}({})"_format(wraper_function, args));
    printer->add_line("int nodecount = ml->nodecount;");
    // clang-format off
    printer->add_line("{0}* {1}inst = ({0}*) ml->instance;"_format(instance_struct(), ptr_type_qualifier()));
    // clang-format on

    if (type == BlockType::Initial) {
        printer->add_newline();
        printer->add_line("setup_instance(nt, ml);");
        printer->add_line("ispc_celsius = celsius;");
        printer->add_newline();
        printer->start_block("if (_nrn_skip_initmodel)");
        printer->add_line("return;");
        printer->end_block();
        printer->add_newline();
    }

    printer->add_line("{}(inst, nt, ml, type);"_format(compute_function));
    printer->end_block();
    printer->add_newline();
}


void CodegenIspcVisitor::print_backend_compute_routine_decl() {
    auto params = get_global_function_parms("");
    auto compute_function = compute_method_name(BlockType::Initial);
    printer->add_line(
        "extern \"C\" void {}({});"_format(compute_function, get_parameter_str(params)));

    if (nrn_cur_required()) {
        compute_function = compute_method_name(BlockType::Equation);
        printer->add_line(
            "extern \"C\" void {}({});"_format(compute_function, get_parameter_str(params)));
    }

    if (nrn_state_required()) {
        compute_function = compute_method_name(BlockType::State);
        printer->add_line(
            "extern \"C\" void {}({});"_format(compute_function, get_parameter_str(params)));
    }

    if (net_receive_required()) {
        auto net_recv_params = ParamVector();
        net_recv_params.emplace_back("", "{}*"_format(instance_struct()), "", "inst");
        net_recv_params.emplace_back("", "NrnThread*", "", "nt");
        net_recv_params.emplace_back("", "Memb_list*", "", "ml");
        printer->add_line("extern \"C\" void {}({});"_format(method_name("ispc_net_buf_receive"),
                                                             get_parameter_str(net_recv_params)));
    }
}


void CodegenIspcVisitor::codegen_wrapper_routines() {
    print_wrapper_routine("nrn_init", BlockType::Initial);
    if (nrn_cur_required()) {
        print_wrapper_routine("nrn_cur", BlockType::Equation);
    }
    if (nrn_state_required()) {
        print_wrapper_routine("nrn_state", BlockType::State);
    }
}


void CodegenIspcVisitor::print_codegen_routines() {
    codegen = true;
    print_backend_info();
    print_headers_include();

    print_data_structures();

    print_compute_functions();

    // now print the ispc wrapper code
    print_codegen_wrapper_routines();
}


void CodegenIspcVisitor::print_codegen_wrapper_routines() {
    printer = wrapper_printer;
    wrapper_codegen = true;
    print_backend_info();
    print_wrapper_headers_include();
    print_ispc_globals();
    print_namespace_begin();

    print_nmodl_constant();
    print_mechanism_info();
    print_wrapper_data_structures();
    print_global_variables_for_hoc();
    print_common_getters();

    print_thread_memory_callbacks();
    print_memory_allocation_routine();
    print_global_variable_setup();
    print_instance_variable_setup();
    print_nrn_alloc();
    print_check_table_thread_function();


    print_net_init();
    print_net_send_buffering();
    print_watch_activate();
    print_watch_check();
    print_net_receive();

    print_backend_compute_routine_decl();

    print_net_receive_buffering_wrapper();
    codegen_wrapper_routines();

    print_mechanism_register();

    print_namespace_end();
}

}  // namespace codegen
}  // namespace nmodl
