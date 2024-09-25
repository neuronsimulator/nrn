/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codegen/codegen_neuron_cpp_visitor.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <optional>
#include <regex>
#include <stdexcept>

#include "ast/all.hpp"
#include "codegen/codegen_cpp_visitor.hpp"
#include "codegen/codegen_utils.hpp"
#include "codegen_naming.hpp"
#include "config/config.h"
#include "solver/solver.hpp"
#include "utils/string_utils.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/var_usage_visitor.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace codegen {

using namespace ast;

using visitor::RenameVisitor;
using visitor::VarUsageVisitor;

using symtab::syminfo::NmodlType;


/****************************************************************************************/
/*                              Generic information getters                             */
/****************************************************************************************/


std::string CodegenNeuronCppVisitor::simulator_name() {
    return "NEURON";
}


std::string CodegenNeuronCppVisitor::backend_name() const {
    return "C++ (api-compatibility)";
}


std::string CodegenNeuronCppVisitor::table_thread_function_name() const {
    return "_check_table_thread";
}


/****************************************************************************************/
/*                     Common helper routines accross codegen functions                 */
/****************************************************************************************/

int CodegenNeuronCppVisitor::position_of_float_var(const std::string& name) const {
    return get_index_from_name(codegen_float_variables, name);
}


int CodegenNeuronCppVisitor::position_of_int_var(const std::string& name) const {
    return get_index_from_name(codegen_int_variables, name);
}


/****************************************************************************************/
/*                                Backend specific routines                             */
/****************************************************************************************/


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_atomic_reduction_pragma() {
    return;
}

bool CodegenNeuronCppVisitor::optimize_ion_variable_copies() const {
    if (optimize_ionvar_copies) {
        throw std::runtime_error("Not implemented.");
    }
    return false;
}


/****************************************************************************************/
/*                         Printing routines for code generation                        */
/****************************************************************************************/


void CodegenNeuronCppVisitor::print_point_process_function_definitions() {
    if (info.point_process) {
        printer->add_multi_line(R"CODE(
            /* Point Process specific functions */
            static void* _hoc_create_pnt(Object* _ho) {
                return create_point_process(_pointtype, _ho);
            }
        )CODE");
        printer->push_block("static void _hoc_destroy_pnt(void* _vptr)");
        if (info.is_watch_used() || info.for_netcon_used) {
            printer->add_line("Prop* _prop = ((Point_process*)_vptr)->prop;");
            printer->push_block("if (_prop)");
            printer->add_line("Datum* _ppvar = _nrn_mechanism_access_dparam(_prop);");
            if (info.is_watch_used()) {
                printer->fmt_line("_nrn_free_watch(_ppvar, {}, {});",
                                  info.watch_count,
                                  info.is_watch_used());
            }
            if (info.for_netcon_used) {
                auto fornetcon_data = get_variable_name("fornetcon_data", false);
                printer->fmt_line("_nrn_free_fornetcon(&{});", fornetcon_data);
            }
            printer->pop_block();
        }
        printer->add_line("destroy_point_process(_vptr);");
        printer->pop_block();
        printer->add_multi_line(R"CODE(
            static double _hoc_loc_pnt(void* _vptr) {
                return loc_point_process(_pointtype, _vptr);
            }

            static double _hoc_has_loc(void* _vptr) {
                return has_loc_point(_vptr);
            }

            static double _hoc_get_loc_pnt(void* _vptr) {
                return (get_loc_point_process(_vptr));
            }
        )CODE");
    }
}


void CodegenNeuronCppVisitor::print_check_table_entrypoint() {
    if (info.table_count == 0) {
        return;
    }

    // print declarations of `check_*` functions
    for (const auto& function: info.functions_with_table) {
        auto name = function->get_node_name();
        auto internal_params = internal_method_parameters();
        printer->fmt_line("void {}({});",
                          table_update_function_name(name),
                          get_parameter_str(internal_params));
    }

    ParamVector args = {{"", "Memb_list*", "", "_ml"},
                        {"", "size_t", "", "id"},
                        {"", "Datum*", "", "_ppvar"},
                        {"", "Datum*", "", "_thread"},
                        {"", "double*", "", "_globals"},
                        {"", "NrnThread*", "", "nt"},
                        {"", "int", "", "_type"},
                        {"", "const _nrn_model_sorted_token&", "", "_sorted_token"}};

    // definition of `_check_table_thread` function
    // signature must be same as the `nrn_thread_table_check_t` type
    printer->fmt_line("static void {}({})", table_thread_function_name(), get_parameter_str(args));
    printer->push_block();
    printer->add_line("_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml, _type};");
    printer->fmt_line("auto inst = make_instance_{}(_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(*nt, *_ml);", info.mod_suffix);
    }
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }

    for (const auto& function: info.functions_with_table) {
        auto method_name = function->get_node_name();
        auto method_args = get_arg_str(internal_method_parameters());
        printer->fmt_line("{}({});", table_update_function_name(method_name), method_args);
    }
    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_setdata_functions() {
    printer->add_line("/* Neuron setdata functions */");
    printer->add_line("extern void _nrn_setdata_reg(int, void(*)(Prop*));");
    printer->push_block("static void _setdata(Prop* _prop)");
    if (!info.point_process) {
        printer->add_multi_line(R"CODE(
            _extcall_prop = _prop;
            _prop_id = _nrn_get_prop_id(_prop);
        )CODE");
    }
    printer->pop_block();

    if (info.point_process) {
        printer->push_block("static void _hoc_setdata(void* _vptr)");
        printer->add_multi_line(R"CODE(
            Prop* _prop;
            _prop = ((Point_process*)_vptr)->prop;
            _setdata(_prop);
        )CODE");
    } else {
        printer->push_block("static void _hoc_setdata()");
        printer->add_multi_line(R"CODE(
            Prop *_prop = hoc_getdata_range(mech_type);
            _setdata(_prop);
            hoc_retpushx(1.);
        )CODE");
    }
    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_function_prototypes() {
    printer->add_newline(2);

    auto print_decl = [this](const auto& callables) {
        for (const auto& node: callables) {
            print_function_declaration(*node, node->get_node_name());
            printer->add_text(';');
            printer->add_newline();
        }
    };

    printer->add_line("/* Mechanism procedures and functions */");
    print_decl(info.functions);
    print_decl(info.procedures);

    for (const auto& node: info.function_tables) {
        auto [params, table_params] = function_table_parameters(*node);
        printer->fmt_line("double {}({});",
                          method_name(node->get_node_name()),
                          get_parameter_str(params));
        printer->fmt_line("double {}({});",
                          method_name("table_" + node->get_node_name()),
                          get_parameter_str(table_params));
    }
}


void CodegenNeuronCppVisitor::print_function_or_procedure(
    const ast::Block& node,
    const std::string& name,
    const std::unordered_set<CppObjectSpecifier>& specifiers) {
    printer->add_newline(2);
    print_function_declaration(node, name, specifiers);
    printer->add_text(" ");
    printer->push_block();

    // function requires return variable declaration
    if (node.is_function_block()) {
        auto type = default_float_data_type();
        printer->fmt_line("{} ret_{} = 0.0;", type, name);
    } else {
        printer->fmt_line("int ret_{} = 0;", name);
    }

    if (!info.artificial_cell) {
        printer->add_line("auto v = node_data.node_voltages[node_data.nodeindices[id]];");
    }

    print_statement_block(*node.get_statement_block(), false, false);
    printer->fmt_line("return ret_{};", name);
    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_function_procedure_helper(const ast::Block& node) {
    auto name = node.get_node_name();
    if (info.function_uses_table(name)) {
        auto new_name = "f_" + name;
        print_function_or_procedure(node,
                                    new_name,
                                    {CppObjectSpecifier::Static, CppObjectSpecifier::Inline});
        print_table_check_function(node);
        print_table_replacement_function(node);
    } else {
        print_function_or_procedure(node, name);
    }
}


void CodegenNeuronCppVisitor::print_hoc_py_wrapper_function_body(
    const ast::Block* function_or_procedure_block,
    InterpreterWrapper wrapper_type) {
    if (info.point_process && wrapper_type == InterpreterWrapper::Python) {
        return;
    }
    const auto block_name = function_or_procedure_block->get_node_name();
    if (info.point_process) {
        printer->fmt_push_block("static double _hoc_{}(void* _vptr)", block_name);
    } else if (wrapper_type == InterpreterWrapper::HOC) {
        printer->fmt_push_block("static void _hoc_{}(void)", block_name);
    } else {
        printer->fmt_push_block("static double _npy_{}(Prop* _prop)", block_name);
    }
    printer->add_multi_line(R"CODE(
        double _r{};
        Datum* _ppvar;
        Datum* _thread;
        NrnThread* nt;
    )CODE");

    std::string prop_name;
    if (info.point_process) {
        printer->add_multi_line(R"CODE(
            auto* const _pnt = static_cast<Point_process*>(_vptr);
            auto* const _p = _pnt->prop;
            if (!_p) {
                hoc_execerror("POINT_PROCESS data instance not valid", nullptr);
            }
            _nrn_mechanism_cache_instance _lmc{_p};
            size_t const id{};
            _ppvar = _nrn_mechanism_access_dparam(_p);
            _thread = _extcall_thread.data();
            nt = static_cast<NrnThread*>(_pnt->_vnt);
        )CODE");

        prop_name = "_p";
    } else if (wrapper_type == InterpreterWrapper::HOC) {
        if (program_symtab->lookup(block_name)->has_all_properties(NmodlType::use_range_ptr_var)) {
            printer->push_block("if (!_prop_id)");
            printer->fmt_line(
                "hoc_execerror(\"No data for {}_{}. Requires prior call to setdata_{} and that the "
                "specified mechanism instance still be in existence.\", nullptr);",
                function_or_procedure_block->get_node_name(),
                info.mod_suffix,
                info.mod_suffix);
            printer->pop_block();
            printer->add_line("Prop* _local_prop = _extcall_prop;");
        } else {
            printer->add_line("Prop* _local_prop = _prop_id ? _extcall_prop : nullptr;");
        }
        printer->add_multi_line(R"CODE(
            _nrn_mechanism_cache_instance _lmc{_local_prop};
            size_t const id{};
            _ppvar = _local_prop ? _nrn_mechanism_access_dparam(_local_prop) : nullptr;
            _thread = _extcall_thread.data();
            nt = nrn_threads;
        )CODE");
        prop_name = "_local_prop";
    } else {  // wrapper_type == InterpreterWrapper::Python
        printer->add_multi_line(R"CODE(
            _nrn_mechanism_cache_instance _lmc{_prop};
            size_t const id = 0;
            _ppvar = _nrn_mechanism_access_dparam(_prop);
            _thread = _extcall_thread.data();
            nt = nrn_threads;
        )CODE");
        prop_name = "_prop";
    }

    printer->fmt_line("auto inst = make_instance_{}(_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}({});", info.mod_suffix, prop_name);
    }
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }
    if (info.function_uses_table(block_name)) {
        printer->fmt_line("{}({});",
                          table_update_function_name(block_name),
                          internal_method_arguments());
    }
    const auto get_func_call_str = [&]() {
        const auto& params = function_or_procedure_block->get_parameters();
        const auto func_proc_name = block_name + "_" + info.mod_suffix;
        auto func_call = fmt::format("{}({}", func_proc_name, internal_method_arguments());
        for (int i = 0; i < params.size(); ++i) {
            func_call.append(fmt::format(", *getarg({})", i + 1));
        }
        func_call.append(")");
        return func_call;
    };
    if (function_or_procedure_block->is_function_block()) {
        printer->add_indent();
        printer->fmt_text("_r = {};", get_func_call_str());
        printer->add_newline();
    } else {
        printer->add_line("_r = 1.;");
        printer->fmt_line("{};", get_func_call_str());
    }
    if (info.point_process || wrapper_type != InterpreterWrapper::HOC) {
        printer->add_line("return(_r);");
    } else if (wrapper_type == InterpreterWrapper::HOC) {
        printer->add_line("hoc_retpushx(_r);");
    }
    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_hoc_py_wrapper_function_definitions() {
    auto print_wrappers = [this](const auto& callables) {
        for (const auto& callable: callables) {
            print_hoc_py_wrapper_function_body(callable, InterpreterWrapper::HOC);
            print_hoc_py_wrapper_function_body(callable, InterpreterWrapper::Python);
        }
    };

    print_wrappers(info.procedures);
    print_wrappers(info.functions);

    for (const auto& node: info.function_tables) {
        auto name = node->get_node_name();
        auto table_name = "table_" + node->get_node_name();


        auto args = std::vector<std::string>{};
        for (size_t i = 0; i < node->get_parameters().size(); ++i) {
            args.push_back(fmt::format("*getarg({})", i + 1));
        }

        // HOC
        printer->fmt_push_block("static void {}()", hoc_function_name(name));
        printer->fmt_line("hoc_retpushx({}({}));", method_name(name), fmt::join(args, ", "));
        printer->pop_block();

        printer->fmt_push_block("static void {}()", hoc_function_name(table_name));
        printer->fmt_line("hoc_retpushx({}());", method_name(table_name));
        printer->pop_block();

        // Python
        printer->fmt_push_block("static double {}(Prop* _prop)", py_function_name(name));
        printer->fmt_line("return {}({});", method_name(name), fmt::join(args, ", "));
        printer->pop_block();

        printer->fmt_push_block("static double {}(Prop* _prop)", py_function_name(table_name));
        printer->fmt_line("return {}();", method_name(table_name));
        printer->pop_block();
    }
}

/****************************************************************************************/
/*                           Code-specific helper routines                              */
/****************************************************************************************/

void CodegenNeuronCppVisitor::add_variable_tqitem(std::vector<IndexVariableInfo>& variables) {
    if (info.net_send_used) {
        variables.emplace_back(make_symbol(naming::TQITEM_VARIABLE), false, false, true);
        variables.back().is_constant = true;
        info.tqitem_index = static_cast<int>(variables.size() - 1);
    }
}

void CodegenNeuronCppVisitor::add_variable_point_process(
    std::vector<IndexVariableInfo>& variables) {
    variables.emplace_back(make_symbol(naming::POINT_PROCESS_VARIABLE), false, false, true);
    variables.back().is_constant = true;
}

std::string CodegenNeuronCppVisitor::internal_method_arguments() {
    const auto& args = internal_method_parameters();
    return get_arg_str(args);
}


CodegenCppVisitor::ParamVector CodegenNeuronCppVisitor::internal_method_parameters() {
    ParamVector params;
    params.emplace_back("", "_nrn_mechanism_cache_range&", "", "_lmc");
    params.emplace_back("", fmt::format("{}&", instance_struct()), "", "inst");
    if (!info.artificial_cell) {
        params.emplace_back("", fmt::format("{}&", node_data_struct()), "", "node_data");
    }
    params.emplace_back("", "size_t", "", "id");
    params.emplace_back("", "Datum*", "", "_ppvar");
    params.emplace_back("", "Datum*", "", "_thread");
    if (!codegen_thread_variables.empty()) {
        params.emplace_back("", fmt::format("{}&", thread_variables_struct()), "", "_thread_vars");
    }
    params.emplace_back("", "NrnThread*", "", "nt");
    return params;
}


/// TODO: Edit for NEURON
const std::string CodegenNeuronCppVisitor::external_method_arguments() noexcept {
    return {};
}


/// TODO: Edit for NEURON
const CodegenCppVisitor::ParamVector CodegenNeuronCppVisitor::external_method_parameters(
    bool table) noexcept {
    return {};
}


/// TODO: Edit for NEURON
std::string CodegenNeuronCppVisitor::nrn_thread_arguments() const {
    return {};
}


/// TODO: Edit for NEURON
std::string CodegenNeuronCppVisitor::nrn_thread_internal_arguments() {
    return {};
}

std::pair<CodegenNeuronCppVisitor::ParamVector, CodegenNeuronCppVisitor::ParamVector>
CodegenNeuronCppVisitor::function_table_parameters(const ast::FunctionTableBlock& node) {
    auto params = ParamVector{};

    for (const auto& i: node.get_parameters()) {
        params.emplace_back("", "double", "", i->get_node_name());
    }
    return {params, {}};
}

/// TODO: Write for NEURON
std::string CodegenNeuronCppVisitor::process_verbatim_text(std::string const& text) {
    return {};
}


/// TODO: Write for NEURON
std::string CodegenNeuronCppVisitor::register_mechanism_arguments() const {
    return {};
};


std::string CodegenNeuronCppVisitor::hoc_function_name(
    const std::string& function_or_procedure_name) const {
    return fmt::format("_hoc_{}", function_or_procedure_name);
}


std::string CodegenNeuronCppVisitor::hoc_function_signature(
    const std::string& function_or_procedure_name) const {
    return fmt::format("static {} {}(void{})",
                       info.point_process ? "double" : "void",
                       hoc_function_name(function_or_procedure_name),
                       info.point_process ? "*" : "");
}


std::string CodegenNeuronCppVisitor::py_function_name(
    const std::string& function_or_procedure_name) const {
    return fmt::format("_npy_{}", function_or_procedure_name);
}


std::string CodegenNeuronCppVisitor::py_function_signature(
    const std::string& function_or_procedure_name) const {
    return fmt::format("static double {}(Prop*)", py_function_name(function_or_procedure_name));
}


/****************************************************************************************/
/*               Code-specific printing routines for code generation                    */
/****************************************************************************************/

std::string CodegenNeuronCppVisitor::namespace_name() {
    return "neuron";
}


void CodegenNeuronCppVisitor::append_conc_write_statements(
    std::vector<ShadowUseStatement>& statements,
    const Ion& ion,
    const std::string& /* concentration */) {
    auto ion_name = ion.name;
    int dparam_index = get_int_variable_index(fmt::format("style_{}", ion_name));

    auto style_name = fmt::format("_style_{}", ion_name);
    auto style_stmt = fmt::format("int {} = *(_ppvar[{}].get<int*>())", style_name, dparam_index);
    statements.push_back(ShadowUseStatement{style_stmt, "", ""});


    auto wrote_conc_stmt = fmt::format("nrn_wrote_conc(_{}_sym, {}, {}, {}, {})",
                                       ion_name,
                                       get_variable_name(ion.rev_potential_pointer_name()),
                                       get_variable_name(ion.intra_conc_pointer_name()),
                                       get_variable_name(ion.extra_conc_pointer_name()),
                                       style_name);
    statements.push_back(ShadowUseStatement{wrote_conc_stmt, "", ""});
}

/****************************************************************************************/
/*                         Routines for returning variable name                         */
/****************************************************************************************/


std::string CodegenNeuronCppVisitor::float_variable_name(const SymbolType& symbol,
                                                         bool use_instance) const {
    if (!use_instance) {
        throw std::runtime_error("Printing non-instance variables is not implemented.");
    }

    auto name = symbol->get_name();
    auto dimension = symbol->get_length();
    if (symbol->is_array()) {
        return fmt::format("(inst.{}+id*{})", name, dimension);
    } else {
        return fmt::format("inst.{}[id]", name);
    }
}


std::string CodegenNeuronCppVisitor::int_variable_name(const IndexVariableInfo& symbol,
                                                       const std::string& name,
                                                       bool use_instance) const {
    auto position = position_of_int_var(name);

    if (info.semantics[position].name == naming::RANDOM_SEMANTIC) {
        return fmt::format("_ppvar[{}].literal_value<void*>()", position);
    }

    if (info.semantics[position].name == naming::FOR_NETCON_SEMANTIC) {
        return fmt::format("_ppvar[{}].literal_value<void*>()", position);
    }

    if (info.semantics[position].name == naming::POINTER_SEMANTIC) {
        return fmt::format("(*_ppvar[{}].get<double*>())", position);
    }

    if (symbol.is_index) {
        if (use_instance) {
            throw std::runtime_error("Not implemented. [wiejo]");
            // return fmt::format("inst->{}[{}]", name, position);
        }
        throw std::runtime_error("Not implemented. [ncuwi]");
        // return fmt::format("indexes[{}]", position);
    }
    if (symbol.is_integer) {
        if (use_instance) {
            return fmt::format("inst.{}[id]", name);
        }
        return fmt::format("_ppvar[{}]", position);
    }
    if (use_instance) {
        return fmt::format("(*inst.{}[id])", name);
    }


    throw std::runtime_error("Not implemented. [nvueir]");
}


std::string CodegenNeuronCppVisitor::thread_variable_name(const ThreadVariableInfo& var_info,
                                                          bool use_instance) const {
    auto i_var = var_info.offset;
    auto var_name = var_info.symbol->get_name();

    if (use_instance) {
        if (var_info.symbol->is_array()) {
            return fmt::format("(_thread_vars.{}_ptr(id))", var_name);
        } else {
            return fmt::format("_thread_vars.{}(id)", var_name);
        }
    } else {
        if (var_info.symbol->is_array()) {
            return fmt::format("({}.thread_data + {})", global_struct_instance(), i_var);
        } else {
            return fmt::format("{}.thread_data[{}]", global_struct_instance(), i_var);
        }
    }
}


std::string CodegenNeuronCppVisitor::global_variable_name(const SymbolType& symbol,
                                                          bool use_instance) const {
    if (use_instance) {
        return fmt::format("inst.{}->{}", naming::INST_GLOBAL_MEMBER, symbol->get_name());
    } else {
        return fmt::format("{}.{}", global_struct_instance(), symbol->get_name());
    }
}


std::string CodegenNeuronCppVisitor::get_variable_name(const std::string& name,
                                                       bool use_instance) const {
    const std::string& varname = update_if_ion_variable_name(name);

    auto name_comparator = [&varname](const auto& sym) { return varname == get_name(sym); };

    if (name == naming::POINT_PROCESS_VARIABLE) {
        if (printing_net_receive) {
            // In net_receive blocks, the point process is passed in as an
            // argument called:
            return "_pnt";
        }
        // The "integer variable" branch will pick up the correct `_ppvar` when
        // not printing a NET_RECEIVE block.
    }

    if (stringutils::starts_with(name, "_ptable_")) {
        return fmt::format("{}.{}", global_struct_instance(), name);
    }

    // float variable
    auto f = std::find_if(codegen_float_variables.begin(),
                          codegen_float_variables.end(),
                          name_comparator);
    if (f != codegen_float_variables.end()) {
        return float_variable_name(*f, use_instance);
    }

    // integer variable
    auto i =
        std::find_if(codegen_int_variables.begin(), codegen_int_variables.end(), name_comparator);
    if (i != codegen_int_variables.end()) {
        return int_variable_name(*i, varname, use_instance);
    }

    // thread variable
    auto t = std::find_if(codegen_thread_variables.begin(),
                          codegen_thread_variables.end(),
                          name_comparator);
    if (t != codegen_thread_variables.end()) {
        return thread_variable_name(*t, use_instance);
    }

    // global variable
    auto g = std::find_if(codegen_global_variables.begin(),
                          codegen_global_variables.end(),
                          name_comparator);
    if (g != codegen_global_variables.end()) {
        return global_variable_name(*g, use_instance);
    }

    if (varname == naming::NTHREAD_DT_VARIABLE) {
        return std::string("nt->_") + naming::NTHREAD_DT_VARIABLE;
    }

    if (varname == naming::NTHREAD_T_VARIABLE) {
        return std::string("nt->_") + naming::NTHREAD_T_VARIABLE;
    }

    // external variable
    auto e = std::find_if(info.external_variables.begin(),
                          info.external_variables.end(),
                          name_comparator);
    if (e != info.external_variables.end()) {
        return fmt::format("{}()", varname);
    }

    auto const iter =
        std::find_if(info.neuron_global_variables.begin(),
                     info.neuron_global_variables.end(),
                     [&varname](auto const& entry) { return entry.first->get_name() == varname; });
    if (iter != info.neuron_global_variables.end()) {
        std::string ret;
        if (use_instance) {
            ret = "*(inst.";
        }
        ret.append(varname);
        if (use_instance) {
            ret.append(")");
        }
        return ret;
    }

    // otherwise return original name
    return varname;
}


/****************************************************************************************/
/*                      Main printing routines for code generation                      */
/****************************************************************************************/


void CodegenNeuronCppVisitor::print_standard_includes() {
    printer->add_newline();
    printer->add_multi_line(R"CODE(
        #include <Eigen/Dense>
        #include <Eigen/LU>
        #include <math.h>
        #include <stdio.h>
        #include <stdlib.h>
        #include <vector>
    )CODE");
    if (info.eigen_newton_solver_exist) {
        printer->add_multi_line(nmodl::solvers::newton_hpp);
    }
}


void CodegenNeuronCppVisitor::print_neuron_includes() {
    printer->add_newline();
    printer->add_multi_line(R"CODE(
        #include "mech_api.h"
        #include "neuron/cache/mechanism_range.hpp"
        #include "nrniv_mf.h"
        #include "section_fwd.hpp"
    )CODE");
}


void CodegenNeuronCppVisitor::print_sdlists_init([[maybe_unused]] bool print_initializers) {
    /// _initlists() should only be called once by the mechanism registration function
    /// (_<mod_file>_reg())
    printer->add_newline(2);
    printer->push_block("static void _initlists()");
    for (auto i = 0; i < info.prime_variables_by_order.size(); ++i) {
        const auto& prime_var = info.prime_variables_by_order[i];
        /// TODO: Something similar needs to happen for slist/dlist2 but I don't know their usage at
        // the moment
        /// TODO: We have to do checks and add errors similar to nocmodl in the
        // SemanticAnalysisVisitor
        if (prime_var->is_array()) {
            /// TODO: Needs a for loop here. Look at
            // https://github.com/neuronsimulator/nrn/blob/df001a436bcb4e23d698afe66c2a513819a6bfe8/src/nmodl/deriv.cpp#L524
            /// TODO: Also needs a test
            printer->fmt_push_block("for (int _i = 0; _i < {}; ++_i)", prime_var->get_length());
            printer->fmt_line("/* {}[{}] */", prime_var->get_name(), prime_var->get_length());
            printer->fmt_line("_slist1[{}+_i] = {{{}, _i}};",
                              i,
                              position_of_float_var(prime_var->get_name()));
            const auto prime_var_deriv_name = "D" + prime_var->get_name();
            printer->fmt_line("/* {}[{}] */", prime_var_deriv_name, prime_var->get_length());
            printer->fmt_line("_dlist1[{}+_i] = {{{}, _i}};",
                              i,
                              position_of_float_var(prime_var_deriv_name));
            printer->pop_block();
        } else {
            printer->fmt_line("/* {} */", prime_var->get_name());
            printer->fmt_line("_slist1[{}] = {{{}, 0}};",
                              i,
                              position_of_float_var(prime_var->get_name()));
            const auto prime_var_deriv_name = "D" + prime_var->get_name();
            printer->fmt_line("/* {} */", prime_var_deriv_name);
            printer->fmt_line("_dlist1[{}] = {{{}, 0}};",
                              i,
                              position_of_float_var(prime_var_deriv_name));
        }
    }
    printer->pop_block();
}

CodegenCppVisitor::ParamVector CodegenNeuronCppVisitor::functor_params() {
    auto params = internal_method_parameters();
    params.push_back({"", "double", "", "v"});

    return params;
}

void CodegenNeuronCppVisitor::print_mechanism_global_var_structure(bool print_initializers) {
    const auto value_initialize = print_initializers ? "{}" : "";

    /// TODO: Print only global variables printed in NEURON
    printer->add_newline(2);
    printer->add_line("/* NEURON global variables */");
    if (info.primes_size != 0) {
        printer->fmt_line("static neuron::container::field_index _slist1[{0}], _dlist1[{0}];",
                          info.primes_size);
    }

    for (const auto& ion: info.ions) {
        printer->fmt_line("static Symbol* _{}_sym;", ion.name);
    }

    printer->add_line("static int mech_type;");

    if (info.point_process) {
        printer->add_line("static int _pointtype;");
    } else {
        printer->add_multi_line(R"CODE(
        static Prop* _extcall_prop;
        /* _prop_id kind of shadows _extcall_prop to allow validity checking. */
        static _nrn_non_owning_id_without_container _prop_id{};)CODE");
    }

    printer->add_line("static _nrn_mechanism_std_vector<Datum> _extcall_thread;");

    // Start printing the CNRN-style global variables.
    auto float_type = default_float_data_type();
    printer->add_newline(2);
    printer->add_line("/** all global variables */");
    printer->fmt_push_block("struct {}", global_struct());

    if (!info.ions.empty()) {
        // TODO implement these when needed.
    }

    if (!info.thread_variables.empty()) {
        size_t prefix_sum = 0;
        for (size_t i = 0; i < info.thread_variables.size(); ++i) {
            const auto& var = info.thread_variables[i];
            codegen_thread_variables.push_back({var, i, prefix_sum});

            prefix_sum += var->get_length();
        }
    }


    for (const auto& var: info.global_variables) {
        codegen_global_variables.push_back(var);
    }

    if (info.vectorize && !info.top_local_variables.empty()) {
        size_t prefix_sum = info.thread_var_data_size;
        size_t n_thread_vars = codegen_thread_variables.size();
        for (size_t i = 0; i < info.top_local_variables.size(); ++i) {
            const auto& var = info.top_local_variables[i];
            codegen_thread_variables.push_back({var, n_thread_vars + i, prefix_sum});

            prefix_sum += var->get_length();
        }
    }

    if (!info.vectorize && !info.top_local_variables.empty()) {
        for (size_t i = 0; i < info.top_local_variables.size(); ++i) {
            const auto& var = info.top_local_variables[i];
            codegen_global_variables.push_back(var);
        }
    }


    if (!codegen_thread_variables.empty()) {
        if (!info.vectorize) {
            // MOD files that aren't "VECTORIZED" don't have thread data.
            throw std::runtime_error("Found thread variables with `vectorize == false`.");
        }

        codegen_global_variables.push_back(make_symbol("thread_data_in_use"));

        auto symbol = make_symbol("thread_data");
        auto thread_data_size = info.thread_var_data_size + info.top_local_thread_size;
        symbol->set_as_array(thread_data_size);
        codegen_global_variables.push_back(symbol);
    }

    for (const auto& var: info.state_vars) {
        auto name = var->get_name() + "0";
        auto symbol = program_symtab->lookup(name);
        if (symbol == nullptr) {
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    for (const auto& var: info.constant_variables) {
        codegen_global_variables.push_back(var);
    }

    for (const auto& var: codegen_global_variables) {
        auto name = var->get_name();
        auto length = var->get_length();
        if (var->is_array()) {
            printer->fmt_line("{} {}[{}] /* TODO init const-array */;", float_type, name, length);
        } else {
            double value{};
            if (auto const& value_ptr = var->get_value()) {
                value = *value_ptr;
            }
            printer->fmt_line("{} {}{};",
                              float_type,
                              name,
                              print_initializers ? fmt::format("{{{:g}}}", value) : std::string{});
        }
    }

    if (info.table_count > 0) {
        // basically the same code as coreNEURON uses
        printer->fmt_line("double usetable{};", print_initializers ? "{1}" : "");
        codegen_global_variables.push_back(make_symbol(naming::USE_TABLE_VARIABLE));

        for (const auto& block: info.functions_with_table) {
            const auto& name = block->get_node_name();
            printer->fmt_line("{} tmin_{}{};", float_type, name, value_initialize);
            printer->fmt_line("{} mfac_{}{};", float_type, name, value_initialize);
            codegen_global_variables.push_back(make_symbol("tmin_" + name));
            codegen_global_variables.push_back(make_symbol("mfac_" + name));
        }

        for (const auto& variable: info.table_statement_variables) {
            auto const name = "t_" + variable->get_name();
            auto const num_values = variable->get_num_values();
            if (variable->is_array()) {
                int array_len = variable->get_length();
                printer->fmt_line(
                    "{} {}[{}][{}]{};", float_type, name, array_len, num_values, value_initialize);
            } else {
                printer->fmt_line("{} {}[{}]{};", float_type, name, num_values, value_initialize);
            }
            codegen_global_variables.push_back(make_symbol(name));
        }
    }

    print_global_struct_function_table_ptrs();

    if (info.vectorize && info.thread_data_index) {
        // TODO compare CoreNEURON something extcall stuff.
        // throw std::runtime_error("Not implemented, global vectorize something else.");
    }

    if (info.diam_used) {
        printer->fmt_line("Symbol* _morphology_sym;");
    }

    printer->pop_block(";");

    print_global_var_struct_assertions();
    print_global_var_struct_decl();
    print_global_var_external_access();

    print_global_param_default_values();
}

void CodegenNeuronCppVisitor::print_global_var_external_access() {
    for (const auto& var: codegen_global_variables) {
        auto var_name = get_name(var);
        auto var_expr = get_variable_name(var_name, false);

        printer->fmt_push_block("auto {}() -> std::decay<decltype({})>::type ",
                                method_name(var_name),
                                var_expr);
        printer->fmt_line("return {};", var_expr);
        printer->pop_block();
    }
    if (!codegen_global_variables.empty()) {
        printer->add_newline();
    }

    for (const auto& var: info.external_variables) {
        auto var_name = get_name(var);
        printer->fmt_line("double {}();", var_name);
    }
    if (!info.external_variables.empty()) {
        printer->add_newline();
    }
}

void CodegenNeuronCppVisitor::print_global_param_default_values() {
    printer->push_block("static std::vector<double> _parameter_defaults =");

    std::vector<std::string> defaults;
    for (const auto& p: info.range_parameter_vars) {
        double value = p->get_value() == nullptr ? 0.0 : *p->get_value();
        defaults.push_back(fmt::format("{:g} /* {} */", value, p->get_name()));
    }

    printer->add_multi_line(fmt::format("{}", fmt::join(defaults, ",\n")));
    printer->pop_block(";");
}

void CodegenNeuronCppVisitor::print_global_variables_for_hoc() {
    auto variable_printer = [&](const std::vector<SymbolType>& variables, bool if_array) {
        for (const auto& variable: variables) {
            if (variable->is_array() == if_array) {
                // false => do not use the instance struct, which is not
                // defined in the global declaration that we are printing
                auto name = get_variable_name(variable->get_name(), false);
                auto ename = add_escape_quote(variable->get_name() + "_" + info.mod_suffix);
                if (if_array) {
                    auto length = variable->get_length();
                    printer->fmt_line("{{{}, {}, {}}},", ename, name, length);
                } else {
                    printer->fmt_line("{{{}, &{}}},", ename, name);
                }
            }
        }
    };

    auto globals = info.global_variables;
    auto thread_vars = info.thread_variables;

    if (info.table_count > 0) {
        globals.push_back(make_symbol(naming::USE_TABLE_VARIABLE));
    }

    printer->add_newline(2);
    printer->add_line("/** connect global (scalar) variables to hoc -- */");
    printer->add_line("static DoubScal hoc_scalar_double[] = {");
    printer->increase_indent();
    variable_printer(globals, false);
    variable_printer(thread_vars, false);
    printer->add_line("{nullptr, nullptr}");
    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(2);
    printer->add_line("/** connect global (array) variables to hoc -- */");
    printer->add_line("static DoubVec hoc_vector_double[] = {");
    printer->increase_indent();
    variable_printer(globals, true);
    variable_printer(thread_vars, true);
    printer->add_line("{nullptr, nullptr, 0}");
    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(2);
    printer->add_line("/* declaration of user functions */");

    auto print_entrypoint_decl = [this](const auto& callables, auto get_name) {
        for (const auto& node: callables) {
            const auto name = get_name(node);
            printer->fmt_line("{};", hoc_function_signature(name));

            if (!info.point_process) {
                printer->fmt_line("{};", py_function_signature(name));
            }
        }
    };

    auto get_name = [](const auto& node) { return node->get_node_name(); };
    print_entrypoint_decl(info.functions, get_name);
    print_entrypoint_decl(info.procedures, get_name);
    print_entrypoint_decl(info.function_tables, get_name);
    print_entrypoint_decl(info.function_tables, [](const auto& node) {
        auto node_name = node->get_node_name();
        return "table_" + node_name;
    });

    printer->add_newline(2);
    printer->add_line("/* connect user functions to hoc names */");
    printer->add_line("static VoidFunc hoc_intfunc[] = {");
    printer->increase_indent();
    if (info.point_process) {
        printer->add_line("{0, 0}");
        printer->decrease_indent();
        printer->add_line("};");
        printer->add_line("static Member_func _member_func[] = {");
        printer->increase_indent();
        printer->add_multi_line(R"CODE(
        {"loc", _hoc_loc_pnt},
        {"has_loc", _hoc_has_loc},
        {"get_loc", _hoc_get_loc_pnt},)CODE");
    } else {
        printer->fmt_line("{{\"setdata_{}\", _hoc_setdata}},", info.mod_suffix);
    }

    auto print_callable_reg = [this](const auto& callables, auto get_name) {
        for (const auto& node: callables) {
            const auto name = get_name(node);
            printer->fmt_line("{{\"{}{}\", {}}},", name, info.rsuffix, hoc_function_name(name));
        }
    };

    print_callable_reg(info.procedures, get_name);
    print_callable_reg(info.functions, get_name);
    print_callable_reg(info.function_tables, get_name);
    print_callable_reg(info.function_tables, [](const auto& node) {
        auto node_name = node->get_node_name();
        return "table_" + node_name;
    });

    printer->add_line("{nullptr, nullptr}");
    printer->decrease_indent();
    printer->add_line("};");
    if (!info.point_process) {
        printer->push_block("static NPyDirectMechFunc npy_direct_func_proc[] =");
        for (const auto& procedure: info.procedures) {
            const auto proc_name = procedure->get_node_name();
            printer->fmt_line("{{\"{}\", {}}},", proc_name, py_function_name(proc_name));
        }
        for (const auto& function: info.functions) {
            const auto func_name = function->get_node_name();
            printer->fmt_line("{{\"{}\", {}}},", func_name, py_function_name(func_name));
        }
        printer->add_line("{nullptr, nullptr}");
        printer->pop_block(";");
    }
}

void CodegenNeuronCppVisitor::print_mechanism_register() {
    printer->add_newline(2);
    printer->add_line("/** register channel with the simulator */");
    printer->fmt_push_block("extern \"C\" void _{}_reg()", info.mod_file);
    printer->add_line("_initlists();");
    printer->add_newline();

    for (const auto& ion: info.ions) {
        double valence = ion.valence.value_or(-10000.0);
        printer->fmt_line("ion_reg(\"{}\", {});", ion.name, valence);
    }
    if (!info.ions.empty()) {
        printer->add_newline();
    }

    for (const auto& ion: info.ions) {
        printer->fmt_line("_{0}_sym = hoc_lookup(\"{0}_ion\");", ion.name);
    }
    if (!info.ions.empty()) {
        printer->add_newline();
    }

    const auto compute_functions_parameters =
        breakpoint_exist()
            ? fmt::format("{}, {}, {}",
                          nrn_cur_required() ? method_name(naming::NRN_CUR_METHOD) : "nullptr",
                          method_name(naming::NRN_JACOB_METHOD),
                          nrn_state_required() ? method_name(naming::NRN_STATE_METHOD) : "nullptr")
            : "nullptr, nullptr, nullptr";


    const auto register_mech_args = fmt::format("{}, {}, {}, {}, {}, {}",
                                                get_channel_info_var_name(),
                                                method_name(naming::NRN_ALLOC_METHOD),
                                                compute_functions_parameters,
                                                method_name(naming::NRN_INIT_METHOD),
                                                info.first_pointer_var_index,
                                                1 + info.thread_data_index);
    if (info.point_process) {
        printer->fmt_line(
            "_pointtype = point_register_mech({}, _hoc_create_pnt, _hoc_destroy_pnt, "
            "_member_func);",
            register_mech_args);

        if (info.destructor_node) {
            printer->fmt_line("register_destructor({});",
                              method_name(naming::NRN_DESTRUCTOR_METHOD));
        }
    } else {
        printer->fmt_line("register_mech({});", register_mech_args);
    }


    if (info.thread_callback_register) {
        printer->fmt_line("_extcall_thread.resize({});", info.thread_data_index + 1);
        printer->fmt_line("thread_mem_init(_extcall_thread.data());");
        printer->fmt_line("{} = 0;", get_variable_name("thread_data_in_use", false));
    }


    /// type related information
    printer->add_newline();
    printer->fmt_line("mech_type = nrn_get_mechtype({}[1]);", get_channel_info_var_name());

    printer->add_line("hoc_register_parm_default(mech_type, &_parameter_defaults);");

    // register the table-checking function
    if (info.table_count > 0) {
        printer->fmt_line("_nrn_thread_table_reg(mech_type, {});", table_thread_function_name());
    }

    printer->add_line("_nrn_mechanism_register_data_fields(mech_type,");
    printer->increase_indent();

    const auto codegen_float_variables_size = codegen_float_variables.size();
    std::vector<std::string> mech_register_args;

    for (int i = 0; i < codegen_float_variables_size; ++i) {
        const auto& float_var = codegen_float_variables[i];
        if (float_var->is_array()) {
            mech_register_args.push_back(
                fmt::format("_nrn_mechanism_field<double>{{\"{}\", {}}} /* {} */",
                            float_var->get_name(),
                            float_var->get_length(),
                            i));
        } else {
            mech_register_args.push_back(fmt::format(
                "_nrn_mechanism_field<double>{{\"{}\"}} /* {} */", float_var->get_name(), i));
        }
    }

    const auto codegen_int_variables_size = codegen_int_variables.size();
    for (int i = 0; i < codegen_int_variables_size; ++i) {
        const auto& int_var = codegen_int_variables[i];
        const auto& name = int_var.symbol->get_name();
        if (i != info.semantics[i].index) {
            throw std::runtime_error("Broken logic.");
        }
        const auto& semantic = info.semantics[i].name;

        auto type = "double*";
        if (name == naming::POINT_PROCESS_VARIABLE) {
            type = "Point_process*";
        } else if (name == naming::TQITEM_VARIABLE) {
            type = "void*";
        } else if (stringutils::starts_with(name, "style_") &&
                   stringutils::starts_with(semantic, "#") &&
                   stringutils::ends_with(semantic, "_ion")) {
            type = "int*";
        } else if (semantic == naming::FOR_NETCON_SEMANTIC) {
            type = "void*";
        }

        mech_register_args.push_back(
            fmt::format("_nrn_mechanism_field<{}>{{\"{}\", \"{}\"}} /* {} */",
                        type,
                        name,
                        info.semantics[i].name,
                        i));
    }

    printer->add_multi_line(fmt::format("{}", fmt::join(mech_register_args, ",\n")));

    printer->decrease_indent();
    printer->add_line(");");
    printer->add_newline();


    printer->fmt_line("hoc_register_prop_size(mech_type, {}, {});",
                      float_variables_size(),
                      int_variables_size());

    for (int i = 0; i < codegen_int_variables_size; ++i) {
        if (i != info.semantics[i].index) {
            throw std::runtime_error("Broken logic.");
        }

        printer->fmt_line("hoc_register_dparam_semantics(mech_type, {}, \"{}\");",
                          i,
                          info.semantics[i].name);
    }

    if (info.write_concentration) {
        printer->fmt_line("nrn_writes_conc(mech_type, 0);");
    }

    if (info.artificial_cell) {
        printer->fmt_line("add_nrn_artcell(mech_type, {});", info.tqitem_index);
    }

    if (info.net_event_used) {
        printer->add_line("add_nrn_has_net_event(mech_type);");
    }

    if (info.for_netcon_used) {
        auto dparam_it =
            std::find_if(info.semantics.begin(), info.semantics.end(), [](const IndexSemantics& a) {
                return a.name == naming::FOR_NETCON_SEMANTIC;
            });
        if (dparam_it == info.semantics.end()) {
            throw std::runtime_error("Couldn't find `fornetcon` variable.");
        }

        int dparam_index = dparam_it->index;
        printer->fmt_line("add_nrn_fornetcons(mech_type, {});", dparam_index);
    }

    printer->add_line("hoc_register_var(hoc_scalar_double, hoc_vector_double, hoc_intfunc);");
    if (!info.point_process) {
        printer->add_line("hoc_register_npy_direct(mech_type, npy_direct_func_proc);");
    }
    if (info.net_receive_node) {
        printer->fmt_line("pnt_receive[mech_type] = nrn_net_receive_{};", info.mod_suffix);
        printer->fmt_line("pnt_receive_size[mech_type] = {};", info.num_net_receive_parameters);
    }

    if (info.net_receive_initial_node) {
        printer->fmt_line("pnt_receive_init[mech_type] = net_init;");
    }

    if (info.thread_callback_register) {
        printer->add_line("_nrn_thread_reg(mech_type, 1, thread_mem_init);");
        printer->add_line("_nrn_thread_reg(mech_type, 0, thread_mem_cleanup);");
    }

    if (info.diam_used) {
        printer->fmt_line("{}._morphology_sym = hoc_lookup(\"morphology\");",
                          global_struct_instance());
    }

    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_thread_memory_callbacks() {
    if (!info.thread_callback_register) {
        return;
    }

    auto static_thread_data = get_variable_name("thread_data", false);
    auto inuse = get_variable_name("thread_data_in_use", false);
    auto thread_data_index = info.thread_var_thread_id;
    printer->push_block("static void thread_mem_init(Datum* _thread) ");
    printer->push_block(fmt::format("if({})", inuse));
    printer->fmt_line("_thread[{}] = {{neuron::container::do_not_search, new double[{}]{{}}}};",
                      thread_data_index,
                      info.thread_var_data_size + info.top_local_thread_size);
    printer->pop_block();
    printer->push_block("else");
    printer->fmt_line("_thread[{}] = {{neuron::container::do_not_search, {}}};",
                      thread_data_index,
                      static_thread_data);
    printer->fmt_line("{} = 1;", inuse);
    printer->pop_block();
    printer->pop_block();

    printer->push_block("static void thread_mem_cleanup(Datum* _thread) ");
    printer->fmt_line("double * _thread_data_ptr = _thread[{}].get<double*>();", thread_data_index);
    printer->push_block(fmt::format("if(_thread_data_ptr == {})", static_thread_data));
    printer->fmt_line("{} = 0;", inuse);
    printer->pop_block();
    printer->push_block("else");
    printer->add_line("delete[] _thread_data_ptr;");
    printer->pop_block();
    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_neuron_global_variable_declarations() {
    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        printer->fmt_line("extern {} {};", type, name);
    }
}

void CodegenNeuronCppVisitor::print_mechanism_range_var_structure(bool print_initializers) {
    auto const value_initialize = print_initializers ? "{}" : "";
    printer->add_newline(2);
    printer->add_line("/** all mechanism instance variables and global variables */");
    printer->fmt_push_block("struct {} ", instance_struct());

    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        printer->fmt_line("{}* {}{};",
                          type,
                          name,
                          print_initializers ? fmt::format("{{&::{}}}", name) : std::string{});
    }
    for (auto& var: codegen_float_variables) {
        const auto& name = var->get_name();
        printer->fmt_line("double* {}{};", name, value_initialize);
    }
    for (auto& var: codegen_int_variables) {
        const auto& name = var.symbol->get_name();
        if (name == naming::POINT_PROCESS_VARIABLE) {
            continue;
        } else if (var.is_index || var.is_integer) {
            // In NEURON we don't create caches for `int*`. Hence, do nothing.
        } else {
            auto qualifier = var.is_constant ? "const " : "";
            auto type = var.is_vdata ? "void*" : default_float_data_type();
            printer->fmt_line("{}{}* const* {}{};", qualifier, type, name, value_initialize);
        }
    }

    printer->fmt_line("{}* {}{};",
                      global_struct(),
                      naming::INST_GLOBAL_MEMBER,
                      print_initializers ? fmt::format("{{&{}}}", global_struct_instance())
                                         : std::string{});
    printer->pop_block(";");
}

void CodegenNeuronCppVisitor::print_make_instance() const {
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_instance_{}(_nrn_mechanism_cache_range& _lmc)",
                            instance_struct(),
                            info.mod_suffix);
    printer->fmt_push_block("return {}", instance_struct());

    std::vector<std::string> make_instance_args;


    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        make_instance_args.push_back(fmt::format("&::{}", name));
    }


    const auto codegen_float_variables_size = codegen_float_variables.size();
    for (int i = 0; i < codegen_float_variables_size; ++i) {
        const auto& float_var = codegen_float_variables[i];
        if (float_var->is_array()) {
            make_instance_args.push_back(
                fmt::format("_lmc.template data_array_ptr<{}, {}>()", i, float_var->get_length()));
        } else {
            make_instance_args.push_back(fmt::format("_lmc.template fpfield_ptr<{}>()", i));
        }
    }

    const auto codegen_int_variables_size = codegen_int_variables.size();
    for (size_t i = 0; i < codegen_int_variables_size; ++i) {
        const auto& var = codegen_int_variables[i];
        auto name = var.symbol->get_name();
        auto const variable = [&var, i]() -> std::string {
            if (var.is_index || var.is_integer) {
                return "";
            } else if (var.is_vdata) {
                return "";
            } else {
                return fmt::format("_lmc.template dptr_field_ptr<{}>()", i);
            }
        }();
        if (variable != "") {
            make_instance_args.push_back(variable);
        }
    }

    printer->add_multi_line(fmt::format("{}", fmt::join(make_instance_args, ",\n")));

    printer->pop_block(";");
    printer->pop_block();
}

void CodegenNeuronCppVisitor::print_node_data_structure(bool print_initializers) {
    printer->add_newline(2);
    printer->fmt_push_block("struct {} ", node_data_struct());

    // Pointers to node variables
    printer->add_line("int const * nodeindices;");
    printer->add_line("double const * node_voltages;");
    printer->add_line("double * node_diagonal;");
    printer->add_line("double * node_rhs;");
    printer->add_line("int nodecount;");

    printer->pop_block(";");
}

void CodegenNeuronCppVisitor::print_make_node_data() const {
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_node_data_{}(NrnThread& nt, Memb_list& _ml_arg)",
                            node_data_struct(),
                            info.mod_suffix);

    std::vector<std::string> make_node_data_args = {"_ml_arg.nodeindices",
                                                    "nt.node_voltage_storage()",
                                                    "nt.node_d_storage()",
                                                    "nt.node_rhs_storage()",
                                                    "_ml_arg.nodecount"};

    printer->fmt_push_block("return {}", node_data_struct());
    printer->add_multi_line(fmt::format("{}", fmt::join(make_node_data_args, ",\n")));

    printer->pop_block(";");
    printer->pop_block();


    printer->fmt_push_block("static {} make_node_data_{}(Prop * _prop)",
                            node_data_struct(),
                            info.mod_suffix);
    printer->add_line("static std::vector<int> node_index{0};");
    printer->add_line("Node* _node = _nrn_mechanism_access_node(_prop);");

    make_node_data_args = {"node_index.data()",
                           "&_nrn_mechanism_access_voltage(_node)",
                           "&_nrn_mechanism_access_d(_node)",
                           "&_nrn_mechanism_access_rhs(_node)",
                           "1"};

    printer->fmt_push_block("return {}", node_data_struct());
    printer->add_multi_line(fmt::format("{}", fmt::join(make_node_data_args, ",\n")));

    printer->pop_block(";");
    printer->pop_block();
    printer->add_newline();
}

void CodegenNeuronCppVisitor::print_thread_variables_structure(bool print_initializers) {
    if (codegen_thread_variables.empty()) {
        return;
    }

    printer->add_newline(2);
    printer->fmt_push_block("struct {} ", thread_variables_struct());
    printer->add_line("double * thread_data;");
    printer->add_newline();

    std::string simd_width = "1";


    for (const auto& var_info: codegen_thread_variables) {
        printer->fmt_push_block("double * {}_ptr(size_t id)", var_info.symbol->get_name());
        printer->fmt_line("return thread_data + {} + (id % {});", var_info.offset, simd_width);
        printer->pop_block();

        printer->fmt_push_block("double & {}(size_t id)", var_info.symbol->get_name());
        printer->fmt_line("return thread_data[{} + (id % {})];", var_info.offset, simd_width);
        printer->pop_block();
    }
    printer->add_newline();

    printer->push_block(fmt::format("{}(double * const thread_data)", thread_variables_struct()));
    printer->fmt_line("this->thread_data = thread_data;");
    printer->pop_block();

    printer->pop_block(";");
}


void CodegenNeuronCppVisitor::print_initial_block(const InitialBlock* node) {
    // read ion statements
    auto read_statements = ion_read_statements(BlockType::Initial);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    // initial block
    if (node != nullptr) {
        const auto& block = node->get_statement_block();
        print_statement_block(*block, false, false);
    }

    // write ion statements
    auto write_statements = ion_write_statements(BlockType::Initial);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Initial);
        printer->add_line(text);
    }
}


void CodegenNeuronCppVisitor::print_global_function_common_code(BlockType type,
                                                                const std::string& function_name) {
    std::string method = function_name.empty() ? compute_method_name(type) : function_name;
    ParamVector args = {{"", "const _nrn_model_sorted_token&", "", "_sorted_token"},
                        {"", "NrnThread*", "", "nt"},
                        {"", "Memb_list*", "", "_ml_arg"},
                        {"", "int", "", "_type"}};
    printer->fmt_push_block("void {}({})", method, get_parameter_str(args));

    printer->add_line("_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _type};");
    printer->fmt_line("auto inst = make_instance_{}(_lmc);", info.mod_suffix);
    printer->fmt_line("auto node_data = make_node_data_{}(*nt, *_ml_arg);", info.mod_suffix);

    printer->add_line("auto nodecount = _ml_arg->nodecount;");
    printer->add_line("auto* _thread = _ml_arg->_thread;");
    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}(_thread[{}].get<double*>());",
                          thread_variables_struct(),
                          info.thread_var_thread_id);
    }
}


void CodegenNeuronCppVisitor::print_nrn_init(bool skip_init_check) {
    printer->add_newline(2);

    print_global_function_common_code(BlockType::Initial);

    printer->push_block("for (int id = 0; id < nodecount; id++)");

    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    if (!info.artificial_cell) {
        printer->add_line("int node_id = node_data.nodeindices[id];");
        printer->add_line("auto v = node_data.node_voltages[node_id];");
    }

    print_rename_state_vars();

    if (!info.changed_dt.empty()) {
        printer->fmt_line("double _save_prev_dt = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE));
        printer->fmt_line("{} = {};",
                          get_variable_name(naming::NTHREAD_DT_VARIABLE),
                          info.changed_dt);
    }

    print_initial_block(info.initial_node);

    if (!info.changed_dt.empty()) {
        printer->fmt_line("{} = _save_prev_dt;", get_variable_name(naming::NTHREAD_DT_VARIABLE));
    }

    printer->pop_block();
    printer->pop_block();
}

void CodegenNeuronCppVisitor::print_nrn_jacob() {
    printer->add_newline(2);

    ParamVector args = {{"", "const _nrn_model_sorted_token&", "", "_sorted_token"},
                        {"", "NrnThread*", "", "nt"},
                        {"", "Memb_list*", "", "_ml_arg"},
                        {"", "int", "", "_type"}};

    printer->fmt_push_block("static void {}({})",
                            method_name(naming::NRN_JACOB_METHOD),
                            get_parameter_str(args));  // begin function


    printer->add_multi_line(
        "_nrn_mechanism_cache_range _lmc{_sorted_token, *nt, *_ml_arg, _type};");

    printer->fmt_line("auto inst = make_instance_{}(_lmc);", info.mod_suffix);
    printer->fmt_line("auto node_data = make_node_data_{}(*nt, *_ml_arg);", info.mod_suffix);
    printer->fmt_line("auto nodecount = _ml_arg->nodecount;");
    printer->push_block("for (int id = 0; id < nodecount; id++)");  // begin for

    if (breakpoint_exist()) {
        printer->add_line("int node_id = node_data.nodeindices[id];");
        printer->fmt_line("node_data.node_diagonal[node_id] {} inst.{}[id];",
                          operator_for_d(),
                          info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                         : naming::CONDUCTANCE_VARIABLE);
    }

    printer->pop_block();  // end for
    printer->pop_block();  // end function
}


void CodegenNeuronCppVisitor::print_callable_preamble_from_prop() {
    printer->add_line("Datum* _ppvar = _nrn_mechanism_access_dparam(prop);");
    printer->add_line("_nrn_mechanism_cache_instance _lmc{prop};");
    printer->add_line("const size_t id = 0;");

    printer->fmt_line("auto inst = make_instance_{}(_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(prop);", info.mod_suffix);
    }

    if (!codegen_thread_variables.empty()) {
        printer->fmt_line("auto _thread_vars = {}({}_global.thread_data);",
                          thread_variables_struct(),
                          info.mod_suffix);
    }

    printer->add_newline();
}

void CodegenNeuronCppVisitor::print_nrn_constructor_declaration() {
    if (info.constructor_node) {
        printer->fmt_line("void {}(Prop* prop);", method_name(naming::NRN_CONSTRUCTOR_METHOD));
    }
}

void CodegenNeuronCppVisitor::print_nrn_constructor() {
    if (info.constructor_node) {
        printer->fmt_push_block("void {}(Prop* prop)", method_name(naming::NRN_CONSTRUCTOR_METHOD));

        print_callable_preamble_from_prop();

        auto block = info.constructor_node->get_statement_block();
        print_statement_block(*block, false, false);

        printer->pop_block();
    }
}


void CodegenNeuronCppVisitor::print_nrn_destructor_declaration() {
    printer->fmt_line("void {}(Prop* prop);", method_name(naming::NRN_DESTRUCTOR_METHOD));
}

void CodegenNeuronCppVisitor::print_nrn_destructor() {
    printer->fmt_push_block("void {}(Prop* prop)", method_name(naming::NRN_DESTRUCTOR_METHOD));
    print_callable_preamble_from_prop();

    for (const auto& rv: info.random_variables) {
        printer->fmt_line("nrnran123_deletestream((nrnran123_State*) {});",
                          get_variable_name(get_name(rv), false));
    }


    if (info.destructor_node) {
        auto block = info.destructor_node->get_statement_block();
        print_statement_block(*block, false, false);
    }

    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_nrn_alloc() {
    printer->add_newline(2);

    auto method = method_name(naming::NRN_ALLOC_METHOD);
    printer->fmt_push_block("static void {}(Prop* _prop)", method);
    printer->add_line("Datum *_ppvar = nullptr;");

    if (info.point_process) {
        printer->push_block("if (nrn_point_prop_)");
        printer->add_multi_line(R"CODE(
                _nrn_mechanism_access_alloc_seq(_prop) = _nrn_mechanism_access_alloc_seq(nrn_point_prop_);
                _ppvar = _nrn_mechanism_access_dparam(nrn_point_prop_);
        )CODE");
        printer->chain_block("else");
    }
    if (info.semantic_variable_count) {
        printer->fmt_line("_ppvar = nrn_prop_datum_alloc(mech_type, {}, _prop);",
                          info.semantic_variable_count);
        printer->add_line("_nrn_mechanism_access_dparam(_prop) = _ppvar;");
    }
    printer->add_multi_line(R"CODE(
        _nrn_mechanism_cache_instance _lmc{_prop};
        size_t const _iml = 0;
    )CODE");
    printer->fmt_line("assert(_nrn_mechanism_get_num_vars(_prop) == {});",
                      codegen_float_variables.size());
    if (float_variables_size()) {
        printer->add_line("/*initialize range parameters*/");
        for (size_t i_param = 0; i_param < info.range_parameter_vars.size(); ++i_param) {
            const auto var = info.range_parameter_vars[i_param];
            if (var->is_array()) {
                continue;
            }
            const auto& var_name = var->get_name();
            auto var_pos = position_of_float_var(var_name);

            printer->fmt_line("_lmc.template fpfield<{}>(_iml) = {}; /* {} */",
                              var_pos,
                              fmt::format("_parameter_defaults[{}]", i_param),
                              var_name);
        }
    }
    if (info.point_process) {
        printer->pop_block();
    }

    if (info.semantic_variable_count) {
        printer->add_line("_nrn_mechanism_access_dparam(_prop) = _ppvar;");
    }

    const auto codegen_int_variables_size = codegen_int_variables.size();

    if (info.diam_used || info.area_used) {
        for (size_t i = 0; i < codegen_int_variables.size(); ++i) {
            auto var_info = codegen_int_variables[i];
            if (var_info.symbol->get_name() == naming::DIAM_VARIABLE) {
                printer->fmt_line("Prop * morphology_prop = need_memb({}._morphology_sym);",
                                  global_struct_instance());
                printer->fmt_line(
                    "_ppvar[{}] = _nrn_mechanism_get_param_handle(morphology_prop, 0);", i);
            }
            if (var_info.symbol->get_name() == naming::AREA_VARIABLE) {
                printer->fmt_line("_ppvar[{}] = _nrn_mechanism_get_area_handle(nrn_alloc_node_);",
                                  i);
            }
        }
    }

    for (const auto& ion: info.ions) {
        printer->fmt_line("Symbol * {}_sym = hoc_lookup(\"{}_ion\");", ion.name, ion.name);
        printer->fmt_line("Prop * {}_prop = need_memb({}_sym);", ion.name, ion.name);

        if (ion.is_exterior_conc_written()) {
            printer->fmt_line("nrn_check_conc_write(_prop, {}_prop, 0);", ion.name);
        }

        if (ion.is_interior_conc_written()) {
            printer->fmt_line("nrn_check_conc_write(_prop, {}_prop, 1);", ion.name);
        }

        int conc = ion.is_conc_written() ? 3 : int(ion.is_conc_read());
        int rev = ion.is_rev_written() ? 3 : int(ion.is_rev_read());

        printer->fmt_line("nrn_promote({}_prop, {}, {});", ion.name, conc, rev);

        for (size_t i = 0; i < codegen_int_variables_size; ++i) {
            const auto& var = codegen_int_variables[i];

            const std::string& var_name = var.symbol->get_name();

            if (stringutils::starts_with(var_name, "ion_")) {
                std::string ion_var_name = std::string(var_name.begin() + 4, var_name.end());
                if (ion.is_ionic_variable(ion_var_name) ||
                    ion.is_current_derivative(ion_var_name) || ion.is_rev_potential(ion_var_name)) {
                    printer->fmt_line("_ppvar[{}] = _nrn_mechanism_get_param_handle({}_prop, {});",
                                      i,
                                      ion.name,
                                      ion.variable_index(ion_var_name));
                }
            } else {
                if (ion.is_style(var_name)) {
                    printer->fmt_line(
                        "_ppvar[{}] = {{neuron::container::do_not_search, "
                        "&(_nrn_mechanism_access_dparam({}_prop)[0].literal_value<int>())}};",
                        i,
                        ion.name);
                }
            }
        }
    }

    if (!info.random_variables.empty()) {
        for (const auto& rv: info.random_variables) {
            printer->fmt_line("{} = nrnran123_newstream();",
                              get_variable_name(get_name(rv), false));
        }
        printer->fmt_line("nrn_mech_inst_destruct[mech_type] = neuron::{};",
                          method_name(naming::NRN_DESTRUCTOR_METHOD));
    }

    if (info.point_process || info.artificial_cell) {
        printer->fmt_push_block("if(!nrn_point_prop_)");

        if (info.constructor_node) {
            printer->fmt_line("{}(_prop);", method_name(naming::NRN_CONSTRUCTOR_METHOD));
        }
        printer->pop_block();
    }

    printer->pop_block();
}


/****************************************************************************************/
/*                                 Print nrn_state routine                              */
/****************************************************************************************/

void CodegenNeuronCppVisitor::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }

    printer->add_newline(2);
    print_global_function_common_code(BlockType::State);

    printer->push_block("for (int id = 0; id < nodecount; id++)");
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    printer->add_line("auto v = node_data.node_voltages[node_id];");

    /**
     * \todo Eigen solver node also emits IonCurVar variable in the functor
     * but that shouldn't update ions in derivative block
     */
    if (ion_variable_struct_required()) {
        throw std::runtime_error("Not implemented.");
    }

    auto read_statements = ion_read_statements(BlockType::State);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.nrn_state_block) {
        info.nrn_state_block->visit_children(*this);
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(*block, false, false);
    }

    const auto& write_statements = ion_write_statements(BlockType::State);
    for (auto& statement: write_statements) {
        const auto& text = process_shadow_update_statement(statement, BlockType::State);
        printer->add_line(text);
    }

    printer->pop_block();
    printer->pop_block();
}


/****************************************************************************************/
/*                              Print nrn_cur related routines                          */
/****************************************************************************************/

std::string CodegenNeuronCppVisitor::nrn_current_arguments() {
    return get_arg_str(nrn_current_parameters());
}


CodegenNeuronCppVisitor::ParamVector CodegenNeuronCppVisitor::nrn_current_parameters() {
    if (ion_variable_struct_required()) {
        throw std::runtime_error("Not implemented.");
    }

    ParamVector params = {{"", "_nrn_mechanism_cache_range&", "", "_lmc"},
                          {"", "NrnThread*", "", "nt"},
                          {"", "Datum*", "", "_ppvar"},
                          {"", "Datum*", "", "_thread"}};

    if (info.thread_callback_register) {
        auto type_name = fmt::format("{}&", thread_variables_struct());
        params.emplace_back("", type_name, "", "_thread_vars");
    }
    params.emplace_back("", "size_t", "", "id");
    params.emplace_back("", fmt::format("{}&", instance_struct()), "", "inst");
    params.emplace_back("", fmt::format("{}&", node_data_struct()), "", "node_data");
    params.emplace_back("", "double", "", "v");
    return params;
}


void CodegenNeuronCppVisitor::print_nrn_current(const BreakpointBlock& node) {
    const auto& args = nrn_current_parameters();
    const auto& block = node.get_statement_block();
    printer->add_newline(2);
    printer->fmt_push_block("inline double nrn_current_{}({})",
                            info.mod_suffix,
                            get_parameter_str(args));
    printer->add_line("double current = 0.0;");
    print_statement_block(*block, false, false);
    for (auto& current: info.currents) {
        const auto& name = get_variable_name(current);
        printer->fmt_line("current += {};", name);
    }
    printer->add_line("return current;");
    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_nrn_cur_conductance_kernel(const BreakpointBlock& node) {
    const auto& block = node.get_statement_block();
    print_statement_block(*block, false, false);
    if (!info.currents.empty()) {
        std::string sum;
        for (const auto& current: info.currents) {
            auto var = breakpoint_current(current);
            sum += get_variable_name(var);
            if (&current != &info.currents.back()) {
                sum += "+";
            }
        }
        printer->fmt_line("double rhs = {};", sum);
    }

    std::string sum;
    for (const auto& conductance: info.conductances) {
        auto var = breakpoint_current(conductance.variable);
        sum += get_variable_name(var);
        if (&conductance != &info.conductances.back()) {
            sum += "+";
        }
    }
    printer->fmt_line("double g = {};", sum);

    for (const auto& conductance: info.conductances) {
        if (!conductance.ion.empty()) {
            const auto& lhs = std::string(naming::ION_VARNAME_PREFIX) + "di" + conductance.ion +
                              "dv";
            const auto& rhs = get_variable_name(conductance.variable);
            const ShadowUseStatement statement{lhs, "+=", rhs};
            const auto& text = process_shadow_update_statement(statement, BlockType::Equation);
            printer->add_line(text);
        }
    }
}


void CodegenNeuronCppVisitor::print_nrn_cur_non_conductance_kernel() {
    printer->fmt_line("double I1 = nrn_current_{}({}+0.001);",
                      info.mod_suffix,
                      nrn_current_arguments());
    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            if (ion.is_ionic_current(var)) {
                const auto& name = get_variable_name(var);
                printer->fmt_line("double di{} = {};", ion.name, name);
            }
        }
    }
    printer->fmt_line("double I0 = nrn_current_{}({});", info.mod_suffix, nrn_current_arguments());
    printer->add_line("double rhs = I0;");

    printer->add_line("double g = (I1-I0)/0.001;");
    for (auto& ion: info.ions) {
        for (auto& var: ion.writes) {
            if (ion.is_ionic_current(var)) {
                const auto& lhs = std::string(naming::ION_VARNAME_PREFIX) + "di" + ion.name + "dv";
                auto rhs = fmt::format("(di{}-{})/0.001", ion.name, get_variable_name(var));
                if (info.point_process) {
                    auto area = get_variable_name(naming::NODE_AREA_VARIABLE);
                    rhs += fmt::format("*1.e2/{}", area);
                }
                const ShadowUseStatement statement{lhs, "+=", rhs};
                const auto& text = process_shadow_update_statement(statement, BlockType::Equation);
                printer->add_line(text);
            }
        }
    }
}


void CodegenNeuronCppVisitor::print_nrn_cur_kernel(const BreakpointBlock& node) {
    printer->add_line("int node_id = node_data.nodeindices[id];");
    printer->add_line("double v = node_data.node_voltages[node_id];");
    printer->add_line("auto* _ppvar = _ml_arg->pdata[id];");
    const auto& read_statements = ion_read_statements(BlockType::Equation);
    for (auto& statement: read_statements) {
        printer->add_line(statement);
    }

    if (info.conductances.empty()) {
        print_nrn_cur_non_conductance_kernel();
    } else {
        print_nrn_cur_conductance_kernel(node);
    }

    const auto& write_statements = ion_write_statements(BlockType::Equation);
    for (auto& statement: write_statements) {
        auto text = process_shadow_update_statement(statement, BlockType::Equation);
        printer->add_line(text);
    }

    if (info.point_process) {
        const auto& area = get_variable_name(naming::NODE_AREA_VARIABLE);
        printer->fmt_line("double mfactor = 1.e2/{};", area);
        printer->add_line("g = g*mfactor;");
        printer->add_line("rhs = rhs*mfactor;");
    }

    // print_g_unused();
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_fast_imem_calculation() {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_cur() {
    if (!nrn_cur_required()) {
        return;
    }

    if (info.conductances.empty()) {
        print_nrn_current(*info.breakpoint_node);
    }

    printer->add_newline(2);
    printer->add_line("/** update current */");
    print_global_function_common_code(BlockType::Equation);
    // print_channel_iteration_block_parallel_hint(BlockType::Equation, info.breakpoint_node);
    printer->push_block("for (int id = 0; id < nodecount; id++)");
    print_nrn_cur_kernel(*info.breakpoint_node);
    // print_nrn_cur_matrix_shadow_update();
    // if (!nrn_cur_reduction_loop_required()) {
    //     print_fast_imem_calculation();
    // }


    printer->fmt_line("node_data.node_rhs[node_id] {} rhs;", operator_for_rhs());

    if (breakpoint_exist()) {
        printer->fmt_line("inst.{}[id] = g;",
                          info.vectorize ? naming::CONDUCTANCE_UNUSED_VARIABLE
                                         : naming::CONDUCTANCE_VARIABLE);
    }
    printer->pop_block();

    // if (nrn_cur_reduction_loop_required()) {
    //     printer->push_block("for (int id = 0; id < nodecount; id++)");
    //     print_nrn_cur_matrix_shadow_reduction();
    //     printer->pop_block();
    //     print_fast_imem_calculation();
    // }

    // print_kernel_data_present_annotation_block_end();
    printer->pop_block();
}


/****************************************************************************************/
/*                            Main code printing entry points                           */
/****************************************************************************************/

void CodegenNeuronCppVisitor::print_headers_include() {
    print_standard_includes();
    print_neuron_includes();

    if (info.thread_callback_register) {
        printer->add_line("extern void _nrn_thread_reg(int, int, void(*)(Datum*));");
    }
}


void CodegenNeuronCppVisitor::print_macro_definitions() {
    print_global_macros();
    print_mechanism_variables_macros();

    printer->add_line("extern Node* nrn_alloc_node_;");
}


void CodegenNeuronCppVisitor::print_global_macros() {
    printer->add_newline();
    printer->add_line("/* NEURON global macro definitions */");
    if (info.vectorize) {
        printer->add_multi_line(R"CODE(
            /* VECTORIZED */
            #define NRN_VECTORIZED 1
        )CODE");
    } else {
        printer->add_multi_line(R"CODE(
            /* NOT VECTORIZED */
            #define NRN_VECTORIZED 0
        )CODE");
    }
}


void CodegenNeuronCppVisitor::print_mechanism_variables_macros() {
    printer->add_newline();
    printer->add_line("static constexpr auto number_of_datum_variables = ",
                      std::to_string(int_variables_size()),
                      ";");
    printer->add_line("static constexpr auto number_of_floating_point_variables = ",
                      std::to_string(codegen_float_variables.size()),
                      ";");
    printer->add_newline();
    printer->add_multi_line(R"CODE(
    namespace {
    template <typename T>
    using _nrn_mechanism_std_vector = std::vector<T>;
    using _nrn_model_sorted_token = neuron::model_sorted_token;
    using _nrn_mechanism_cache_range = neuron::cache::MechanismRange<number_of_floating_point_variables, number_of_datum_variables>;
    using _nrn_mechanism_cache_instance = neuron::cache::MechanismInstance<number_of_floating_point_variables, number_of_datum_variables>;
    using _nrn_non_owning_id_without_container = neuron::container::non_owning_identifier_without_container;
    template <typename T>
    using _nrn_mechanism_field = neuron::mechanism::field<T>;
    template <typename... Args>
    void _nrn_mechanism_register_data_fields(Args&&... args) {
        neuron::mechanism::register_data_fields(std::forward<Args>(args)...);
    }
    }  // namespace
    )CODE");

    if (info.point_process) {
        printer->add_line("extern Prop* nrn_point_prop_;");
    } else {
        printer->add_line("Prop* hoc_getdata_range(int type);");
    }
    // for registration of tables
    if (info.table_count > 0) {
        printer->add_line("void _nrn_thread_table_reg(int, nrn_thread_table_check_t);");
    }
    if (info.for_netcon_used) {
        printer->add_line("int _nrn_netcon_args(void*, double***);");
    }
}


void CodegenNeuronCppVisitor::print_data_structures(bool print_initializers) {
    print_mechanism_global_var_structure(print_initializers);
    print_mechanism_range_var_structure(print_initializers);
    print_node_data_structure(print_initializers);
    print_thread_variables_structure(print_initializers);
    print_make_instance();
    print_make_node_data();
}


void CodegenNeuronCppVisitor::print_v_unused() const {
    if (!info.vectorize) {
        return;
    }
    printer->add_multi_line(R"CODE(
        #if NRN_PRCELLSTATE
        inst->v_unused[id] = v;
        #endif
    )CODE");
}


void CodegenNeuronCppVisitor::print_g_unused() const {
    printer->add_multi_line(R"CODE(
        #if NRN_PRCELLSTATE
        inst->g_unused[id] = g;
        #endif
    )CODE");
}


void CodegenNeuronCppVisitor::print_compute_functions() {
    print_hoc_py_wrapper_function_definitions();
    for (const auto& procedure: info.procedures) {
        print_procedure(*procedure);
    }
    for (const auto& function: info.functions) {
        print_function(*function);
    }
    for (const auto& function_table: info.function_tables) {
        print_function_tables(*function_table);
    }

    print_nrn_init();
    print_nrn_cur();
    print_nrn_state();
    print_nrn_jacob();
    print_net_receive();
    print_net_init();
}


void CodegenNeuronCppVisitor::print_codegen_routines() {
    print_backend_info();
    print_headers_include();
    print_macro_definitions();
    print_neuron_global_variable_declarations();
    print_namespace_start();
    print_nmodl_constants();
    print_prcellstate_macros();
    print_mechanism_info();
    print_data_structures(true);
    print_nrn_constructor_declaration();
    print_nrn_destructor_declaration();
    print_nrn_alloc();
    print_function_prototypes();
    print_point_process_function_definitions();
    print_setdata_functions();
    print_check_table_entrypoint();
    print_functors_definitions();
    print_global_variables_for_hoc();
    print_thread_memory_callbacks();
    print_compute_functions();  // only nrn_cur and nrn_state
    print_nrn_constructor();
    print_nrn_destructor();
    print_sdlists_init(true);
    print_mechanism_register();
    print_namespace_stop();
}

void CodegenNeuronCppVisitor::print_ion_variable() {
    throw std::runtime_error("Not implemented.");
}


void CodegenNeuronCppVisitor::print_net_send_call(const ast::FunctionCall& node) {
    auto const& arguments = node.get_arguments();

    if (printing_net_init) {
        throw std::runtime_error("Not implemented. [jfiwoei]");
    }

    std::string weight_pointer = "nullptr";
    auto point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE,
                                           /* use_instance */ false);
    if (!printing_net_receive) {
        point_process += ".get<Point_process*>()";
    }
    const auto& tqitem = get_variable_name("tqitem", /* use_instance */ false);

    printer->fmt_text("{}(/* tqitem */ &{}, {}, {}, {} + ",
                      info.artificial_cell ? "artcell_net_send" : "net_send",
                      tqitem,
                      weight_pointer,
                      point_process,
                      get_variable_name("t"));
    print_vector_elements(arguments, ", ");
    printer->add_text(')');
}

void CodegenNeuronCppVisitor::print_net_move_call(const ast::FunctionCall& node) {
    const auto& point_process = get_variable_name("point_process", /* use_instance */ false);
    const auto& tqitem = get_variable_name("tqitem", /* use_instance */ false);

    printer->fmt_text("{}(/* tqitem */ &{}, {}, ",
                      info.artificial_cell ? "artcell_net_move" : "net_move",
                      tqitem,
                      point_process);

    print_vector_elements(node.get_arguments(), ", ");
    printer->add_text(')');
}

void CodegenNeuronCppVisitor::print_net_event_call(const ast::FunctionCall& /* node */) {
    const auto& point_process = get_variable_name(naming::POINT_PROCESS_VARIABLE,
                                                  /* use_instance */ false);
    printer->fmt_text("net_event({}, t)", point_process);
}

void CodegenNeuronCppVisitor::print_function_table_call(const FunctionCall& node) {
    auto name = node.get_node_name();
    const auto& arguments = node.get_arguments();
    printer->add_text(method_name(name), "(");
    print_vector_elements(arguments, ", ");
    printer->add_text(')');
}

/**
 * Rename arguments to NET_RECEIVE block with corresponding pointer variable
 *
 * \code{.mod}
 *      NET_RECEIVE (weight, R){
 *            x = R
 *      }
 * \endcode
 *
 * then generated code should be:
 *
 * \code{.cpp}
 *      x[id] = _args[1];
 * \endcode
 *
 * So, the `R` in AST needs to be renamed with `_args[1]`.
 */
static void rename_net_receive_arguments(const ast::NetReceiveBlock& net_receive_node,
                                         const ast::Node& node) {
    const auto& parameters = net_receive_node.get_parameters();

    auto n_parameters = parameters.size();
    for (size_t i = 0; i < n_parameters; ++i) {
        const auto& name = parameters[i]->get_node_name();
        auto var_used = VarUsageVisitor().variable_used(node, name);
        if (var_used) {
            RenameVisitor vr(name, fmt::format("_args[{}]", i));
            node.get_statement_block()->visit_children(vr);
        }
    }
}

CodegenNeuronCppVisitor::ParamVector CodegenNeuronCppVisitor::net_receive_args() {
    return {{"", "Point_process*", "", "_pnt"},
            {"", "double*", "", "_args"},
            {"", "double", "", "flag"}};
}


void CodegenNeuronCppVisitor::print_net_receive_common_code() {
    printer->add_line("_nrn_mechanism_cache_instance _lmc{_pnt->prop};");
    printer->add_line("auto * nt = static_cast<NrnThread*>(_pnt->_vnt);");
    printer->add_line("auto * _ppvar = _nrn_mechanism_access_dparam(_pnt->prop);");

    printer->fmt_line("auto inst = make_instance_{}(_lmc);", info.mod_suffix);
    if (!info.artificial_cell) {
        printer->fmt_line("auto node_data = make_node_data_{}(_pnt->prop);", info.mod_suffix);
    }
    printer->fmt_line("// nocmodl has a nullptr dereference for thread variables.");
    printer->fmt_line("// NMODL will fail to compile at a later point, because of");
    printer->fmt_line("// missing '_thread_vars'.");
    printer->fmt_line("Datum * _thread = nullptr;");

    printer->add_line("size_t id = 0;");
    printer->add_line("double t = nt->_t;");
}

void CodegenNeuronCppVisitor::print_net_receive() {
    printing_net_receive = true;
    auto node = info.net_receive_node;
    if (!node) {
        return;
    }

    printer->fmt_push_block("static void nrn_net_receive_{}({})",
                            info.mod_suffix,
                            get_parameter_str(net_receive_args()));

    rename_net_receive_arguments(*node, *node);
    print_net_receive_common_code();


    print_statement_block(*node->get_statement_block(), false, false);

    printer->add_newline();
    printer->pop_block();
    printing_net_receive = false;
}

void CodegenNeuronCppVisitor::print_net_init() {
    const auto node = info.net_receive_initial_node;
    if (node == nullptr) {
        return;
    }

    // rename net_receive arguments used in the initial block of net_receive
    rename_net_receive_arguments(*info.net_receive_node, *node);

    printing_net_init = true;
    printer->add_newline(2);
    printer->fmt_push_block("static void net_init({})", get_parameter_str(net_receive_args()));

    auto block = node->get_statement_block().get();
    if (!block->get_statements().empty()) {
        print_net_receive_common_code();
        print_statement_block(*block, false, false);
    }
    printer->pop_block();
    printing_net_init = false;
}


/****************************************************************************************/
/*                            Overloaded visitor routines                               */
/****************************************************************************************/

/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::visit_watch_statement(const ast::WatchStatement& /* node */) {
    return;
}

void CodegenNeuronCppVisitor::visit_for_netcon(const ast::ForNetcon& node) {
    // The setup for enabling this loop is:
    //   double ** _fornetcon_data = ...;
    //   for(size_t i = 0; i < n_netcons; ++i) {
    //      double * _netcon_data = _fornetcon_data[i];
    //
    //      // loop body.
    //   }
    //
    // Where `_fornetcon_data` is an array of pointers to the arguments, one
    // for each netcon; and `_netcon_data` points to the arguments for the
    // current netcon.
    //
    // Similar to the CoreNEURON solution, we replace all arguments with the
    // C++ string that implements them, i.e. `_netcon_data[{}]`. The arguments
    // are positional and thus simply numbered through.
    const auto& args = node.get_parameters();
    RenameVisitor v;
    const auto& statement_block = node.get_statement_block();
    for (size_t i_arg = 0; i_arg < args.size(); ++i_arg) {
        auto old_name = args[i_arg]->get_node_name();
        auto new_name = fmt::format("_netcon_data[{}]", i_arg);
        v.set(old_name, new_name);
        statement_block->accept(v);
    }

    auto dparam_it =
        std::find_if(info.semantics.begin(), info.semantics.end(), [](const IndexSemantics& a) {
            return a.name == naming::FOR_NETCON_SEMANTIC;
        });
    if (dparam_it == info.semantics.end()) {
        throw std::runtime_error("Couldn't find `fornetcon` variable.");
    }

    int dparam_index = dparam_it->index;
    auto netcon_var = get_name(codegen_int_variables[dparam_index]);

    // This is called from `print_statement_block` which pre-indents the
    // current line. Hence `add_text` only.
    printer->add_text("double ** _fornetcon_data;");
    printer->add_newline();

    printer->fmt_line("int _n_netcons = _nrn_netcon_args({}, &_fornetcon_data);",
                      get_variable_name(netcon_var, false));

    printer->push_block("for (size_t _i = 0; _i < _n_netcons; ++_i)");
    printer->add_line("double * _netcon_data = _fornetcon_data[_i];");
    print_statement_block(*statement_block, false, false);
    printer->pop_block();
}


}  // namespace codegen
}  // namespace nmodl
