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
#include <regex>

#include "ast/all.hpp"
#include "codegen/codegen_utils.hpp"
#include "config/config.h"
#include "utils/string_utils.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace codegen {

using namespace ast;

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


/****************************************************************************************/
/*                     Common helper routines accross codegen functions                 */
/****************************************************************************************/


int CodegenNeuronCppVisitor::position_of_float_var(const std::string& name) const {
    const auto has_name = [&name](const SymbolType& symbol) { return symbol->get_name() == name; };
    const auto var_iter =
        std::find_if(codegen_float_variables.begin(), codegen_float_variables.end(), has_name);
    if (var_iter != codegen_float_variables.end()) {
        return var_iter - codegen_float_variables.begin();
    } else {
        throw std::logic_error(name + " variable not found");
    }
}


int CodegenNeuronCppVisitor::position_of_int_var(const std::string& name) const {
    const auto has_name = [&name](const IndexVariableInfo& index_var_symbol) {
        return index_var_symbol.symbol->get_name() == name;
    };
    const auto var_iter =
        std::find_if(codegen_int_variables.begin(), codegen_int_variables.end(), has_name);
    if (var_iter != codegen_int_variables.end()) {
        return var_iter - codegen_int_variables.begin();
    } else {
        throw std::logic_error(name + " variable not found");
    }
}


/****************************************************************************************/
/*                                Backend specific routines                             */
/****************************************************************************************/


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_atomic_reduction_pragma() {
    return;
}


/****************************************************************************************/
/*                         Printing routines for code generation                        */
/****************************************************************************************/


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_function_prototypes() {
    if (info.functions.empty() && info.procedures.empty()) {
        return;
    }
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_function_or_procedure(const ast::Block& node,
                                                          const std::string& name) {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_function_procedure_helper(const ast::Block& node) {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_procedure(const ast::ProcedureBlock& node) {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_function(const ast::FunctionBlock& node) {
    return;
}


/****************************************************************************************/
/*                           Code-specific helper routines                              */
/****************************************************************************************/


/// TODO: Edit for NEURON
std::string CodegenNeuronCppVisitor::internal_method_arguments() {
    return {};
}


/// TODO: Edit for NEURON
CodegenNeuronCppVisitor::ParamVector CodegenNeuronCppVisitor::internal_method_parameters() {
    return {};
}


/// TODO: Edit for NEURON
const char* CodegenNeuronCppVisitor::external_method_arguments() noexcept {
    return {};
}


/// TODO: Edit for NEURON
const char* CodegenNeuronCppVisitor::external_method_parameters(bool table) noexcept {
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


/// TODO: Write for NEURON
std::string CodegenNeuronCppVisitor::process_verbatim_text(std::string const& text) {
    return {};
}


/// TODO: Write for NEURON
std::string CodegenNeuronCppVisitor::register_mechanism_arguments() const {
    return {};
};


/****************************************************************************************/
/*               Code-specific printing routines for code generation                    */
/****************************************************************************************/


void CodegenNeuronCppVisitor::print_namespace_start() {
    printer->add_newline(2);
    printer->push_block("namespace neuron");
}


void CodegenNeuronCppVisitor::print_namespace_stop() {
    printer->pop_block();
}


/****************************************************************************************/
/*                         Routines for returning variable name                         */
/****************************************************************************************/


/// TODO: Edit for NEURON
std::string CodegenNeuronCppVisitor::float_variable_name(const SymbolType& symbol,
                                                         bool use_instance) const {
    auto name = symbol->get_name();
    auto dimension = symbol->get_length();
    // auto position = position_of_float_var(name);
    if (symbol->is_array()) {
        if (use_instance) {
            return fmt::format("(inst.{}+id*{})", name, dimension);
        }
        throw std::runtime_error("Printing non-instance variables is not implemented.");
        // return fmt::format("(data + {}*pnodecount + id*{})", position, dimension);
    }
    if (use_instance) {
        return fmt::format("inst.{}[id]", name);
    }
    throw std::runtime_error("Not implemented.");
    // return fmt::format("data[{}*pnodecount + id]", position);
}


/// TODO: Edit for NEURON
std::string CodegenNeuronCppVisitor::int_variable_name(const IndexVariableInfo& symbol,
                                                       const std::string& name,
                                                       bool use_instance) const {
    return name;
}


/// TODO: Edit for NEURON
std::string CodegenNeuronCppVisitor::global_variable_name(const SymbolType& symbol,
                                                          bool use_instance) const {
    return symbol->get_name();
}


std::string CodegenNeuronCppVisitor::get_variable_name(const std::string& name,
                                                       bool use_instance) const {
    // const std::string& varname = update_if_ion_variable_name(name);
    const std::string& varname = name;

    auto symbol_comparator = [&varname](const SymbolType& sym) {
        return varname == sym->get_name();
    };

    auto index_comparator = [&varname](const IndexVariableInfo& var) {
        return varname == var.symbol->get_name();
    };

    // float variable
    auto f = std::find_if(codegen_float_variables.begin(),
                          codegen_float_variables.end(),
                          symbol_comparator);
    if (f != codegen_float_variables.end()) {
        return float_variable_name(*f, use_instance);
    }

    // integer variable
    auto i =
        std::find_if(codegen_int_variables.begin(), codegen_int_variables.end(), index_comparator);
    if (i != codegen_int_variables.end()) {
        return int_variable_name(*i, varname, use_instance);
    }

    // global variable
    auto g = std::find_if(codegen_global_variables.begin(),
                          codegen_global_variables.end(),
                          symbol_comparator);
    if (g != codegen_global_variables.end()) {
        return global_variable_name(*g, use_instance);
    }

    if (varname == naming::NTHREAD_DT_VARIABLE) {
        return std::string("_nt->_") + naming::NTHREAD_DT_VARIABLE;
    }

    // t in net_receive method is an argument to function and hence it should
    // be used instead of nt->_t which is current time of thread
    if (varname == naming::NTHREAD_T_VARIABLE && !printing_net_receive) {
        return std::string("_nt->_") + naming::NTHREAD_T_VARIABLE;
    }

    auto const iter =
        std::find_if(info.neuron_global_variables.begin(),
                     info.neuron_global_variables.end(),
                     [&varname](auto const& entry) { return entry.first->get_name() == varname; });
    if (iter != info.neuron_global_variables.end()) {
        std::string ret;
        if (use_instance) {
            ret = "*(inst->";
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


void CodegenNeuronCppVisitor::print_backend_info() {
    time_t current_time{};
    time(&current_time);
    std::string data_time_str{std::ctime(&current_time)};
    auto version = nmodl::Version::NMODL_VERSION + " [" + nmodl::Version::GIT_REVISION + "]";

    printer->add_line("/*********************************************************");
    printer->add_line("Model Name      : ", info.mod_suffix);
    printer->add_line("Filename        : ", info.mod_file, ".mod");
    printer->add_line("NMODL Version   : ", nmodl_version());
    printer->fmt_line("Vectorized      : {}", info.vectorize);
    printer->fmt_line("Threadsafe      : {}", info.thread_safe);
    printer->add_line("Created         : ", stringutils::trim(data_time_str));
    printer->add_line("Simulator       : ", simulator_name());
    printer->add_line("Backend         : ", backend_name());
    printer->add_line("NMODL Compiler  : ", version);
    printer->add_line("*********************************************************/");
}


void CodegenNeuronCppVisitor::print_standard_includes() {
    printer->add_newline();
    printer->add_multi_line(R"CODE(
        #include <math.h>
        #include <stdio.h>
        #include <stdlib.h>
    )CODE");
    if (!info.vectorize) {
        printer->add_line("#include <vector>");
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


void CodegenNeuronCppVisitor::print_mechanism_global_var_structure(bool print_initializers) {
    /// TODO: Print only global variables printed in NEURON
    printer->add_newline(2);
    printer->add_line("/* NEURON global variables */");
    if (info.primes_size != 0) {
        printer->fmt_line("static neuron::container::field_index _slist1[{0}], _dlist1[{0}];",
                          info.primes_size);
    }
    printer->add_line("static int mech_type;");

    printer->fmt_line("static int {} = {};",
                      naming::NRN_POINTERINDEX,
                      info.pointer_variables.size() > 0
                          ? static_cast<int>(info.pointer_variables.size())
                          : -1);
}


/// TODO: Same as CoreNEURON?
void CodegenNeuronCppVisitor::print_global_variables_for_hoc() {
    /// TODO: Write HocParmLimits and other HOC global variables (delta_t)
    // Probably needs more changes
    auto variable_printer =
        [&](const std::vector<SymbolType>& variables, bool if_array, bool if_vector) {
            for (const auto& variable: variables) {
                if (variable->is_array() == if_array) {
                    // false => do not use the instance struct, which is not
                    // defined in the global declaration that we are printing
                    auto name = get_variable_name(variable->get_name(), false);
                    auto ename = add_escape_quote(variable->get_name() + "_" + info.mod_suffix);
                    auto length = variable->get_length();
                    if (if_vector) {
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
    variable_printer(globals, false, false);
    variable_printer(thread_vars, false, false);
    printer->add_line("{nullptr, nullptr}");
    printer->decrease_indent();
    printer->add_line("};");

    printer->add_newline(2);
    printer->add_line("/** connect global (array) variables to hoc -- */");
    printer->add_line("static DoubVec hoc_vector_double[] = {");
    printer->increase_indent();
    variable_printer(globals, true, true);
    variable_printer(thread_vars, true, true);
    printer->add_line("{nullptr, nullptr, 0}");
    printer->decrease_indent();
    printer->add_line("};");
}

void CodegenNeuronCppVisitor::print_make_instance() const {
    printer->add_newline(2);
    printer->fmt_push_block("static {} make_instance_{}(_nrn_mechanism_cache_range& _ml)",
                            instance_struct(),
                            info.mod_suffix);
    printer->fmt_push_block("return {}", instance_struct());

    const auto codegen_float_variables_size = codegen_float_variables.size();
    for (int i = 0; i < codegen_float_variables_size; ++i) {
        const auto& float_var = codegen_float_variables[i];
        if (float_var->is_array()) {
            printer->fmt_line("_ml.template data_array<{}, {}>(0){}",
                              i,
                              float_var->get_length(),
                              i < codegen_float_variables_size - 1 ? "," : "");
        } else {
            printer->fmt_line("&_ml.template fpfield<{}>(0){}",
                              i,
                              i < codegen_float_variables_size - 1 ? "," : "");
        }
    }
    printer->pop_block(";");
    printer->pop_block();
}

void CodegenNeuronCppVisitor::print_mechanism_register() {
    /// TODO: Write this according to NEURON
    printer->add_newline(2);
    printer->add_line("/** register channel with the simulator */");
    printer->fmt_push_block("extern \"C\" void _{}_reg()", info.mod_file);
    printer->add_line("_initlists();");
    printer->add_newline();

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
                                                naming::NRN_POINTERINDEX,
                                                1 + info.thread_data_index);
    printer->fmt_line("register_mech({});", register_mech_args);

    // type related information
    printer->add_newline();
    printer->fmt_line("mech_type = nrn_get_mechtype({}[1]);", get_channel_info_var_name());

    // More things to add here
    printer->add_line("_nrn_mechanism_register_data_fields(mech_type,");
    printer->increase_indent();
    const auto codegen_float_variables_size = codegen_float_variables.size();
    for (int i = 0; i < codegen_float_variables_size; ++i) {
        const auto& float_var = codegen_float_variables[i];
        const auto print_comma = i < codegen_float_variables_size - 1 || info.emit_cvode;
        if (float_var->is_array()) {
            printer->fmt_line("_nrn_mechanism_field<double>{{\"{}\", {}}} /* {} */{}",
                              float_var->get_name(),
                              float_var->get_length(),
                              i,
                              print_comma ? "," : "");
        } else {
            printer->fmt_line("_nrn_mechanism_field<double>{{\"{}\"}} /* {} */{}",
                              float_var->get_name(),
                              i,
                              print_comma ? "," : "");
        }
    }
    if (info.emit_cvode) {
        printer->add_line("_nrn_mechanism_field<int>{\"_cvode_ieq\", \"cvodeieq\"} /* 0 */");
    }
    printer->decrease_indent();
    printer->add_line(");");
    printer->add_newline();
    printer->pop_block();
}


void CodegenNeuronCppVisitor::print_mechanism_range_var_structure(bool print_initializers) {
    auto const value_initialize = print_initializers ? "{}" : "";
    auto int_type = default_int_data_type();
    printer->add_newline(2);
    printer->add_line("/** all mechanism instance variables and global variables */");
    printer->fmt_push_block("struct {} ", instance_struct());

    for (auto const& [var, type]: info.neuron_global_variables) {
        auto const name = var->get_name();
        printer->fmt_line("{}* {}{};",
                          type,
                          name,
                          print_initializers ? fmt::format("{{&coreneuron::{}}}", name)
                                             : std::string{});
    }
    for (auto& var: codegen_float_variables) {
        const auto& name = var->get_name();
        printer->fmt_line("double* {}{};", name, value_initialize);
    }
    for (auto& var: codegen_int_variables) {
        const auto& name = var.symbol->get_name();
        if (var.is_index || var.is_integer) {
            auto qualifier = var.is_constant ? "const " : "";
            printer->fmt_line("{}{}* {}{};", qualifier, int_type, name, value_initialize);
        } else {
            auto qualifier = var.is_constant ? "const " : "";
            auto type = var.is_vdata ? "void*" : default_float_data_type();
            printer->fmt_line("{}{}* {}{};", qualifier, type, name, value_initialize);
        }
    }

    // printer->fmt_line("{}* {}{};",
    //                   global_struct(),
    //                   naming::INST_GLOBAL_MEMBER,
    //                   print_initializers ? fmt::format("{{&{}}}", global_struct_instance())
    //                                      : std::string{});
    printer->pop_block(";");
}


void CodegenNeuronCppVisitor::print_initial_block(const InitialBlock* node) {
    // initial block
    if (node != nullptr) {
        const auto& block = node->get_statement_block();
        print_statement_block(*block, false, false);
    }
}


void CodegenNeuronCppVisitor::print_global_function_common_code(BlockType type,
                                                                const std::string& function_name) {
    std::string method = function_name.empty() ? compute_method_name(type) : function_name;
    std::string args =
        "_nrn_model_sorted_token const& _sorted_token, NrnThread* _nt, Memb_list* _ml_arg, int "
        "_type";
    printer->fmt_push_block("void {}({})", method, args);

    printer->add_line("_nrn_mechanism_cache_range _lmr{_sorted_token, *_nt, *_ml_arg, _type};");
    printer->fmt_line("auto inst = make_instance_{}(_lmr);", info.mod_suffix);
    printer->add_line("auto nodecount = _ml_arg->nodecount;");
}


void CodegenNeuronCppVisitor::print_nrn_init(bool skip_init_check) {
    printer->add_newline(2);

    print_global_function_common_code(BlockType::Initial);

    printer->push_block("for (int id = 0; id < nodecount; id++)");
    print_initial_block(info.initial_node);
    printer->pop_block();

    printer->pop_block();
}

void CodegenNeuronCppVisitor::print_nrn_jacob() {
    printer->add_newline(2);
    printer->add_line("/** nrn_jacob function */");

    printer->fmt_line(
        "static void {}(_nrn_model_sorted_token const& _sorted_token, NrnThread* "
        "_nt, Memb_list* _ml_arg, int _type) {{}}",
        method_name(naming::NRN_JACOB_METHOD));
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_constructor() {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_destructor() {
    return;
}


/// TODO: Print the equivalent of `nrn_alloc_<mech_name>`
void CodegenNeuronCppVisitor::print_nrn_alloc() {
    printer->add_newline(2);
    auto method = method_name(naming::NRN_ALLOC_METHOD);
    printer->fmt_push_block("static void {}(Prop* _prop)", method);
    printer->add_line("// do nothing");
    printer->pop_block();
}


/****************************************************************************************/
/*                                 Print nrn_state routine                              */
/****************************************************************************************/


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_state() {
    if (!nrn_state_required()) {
        return;
    }

    printer->add_newline(2);
    print_global_function_common_code(BlockType::State);

    printer->push_block("for (int id = 0; id < nodecount; id++)");

    if (info.nrn_state_block) {
        info.nrn_state_block->visit_children(*this);
    }

    if (info.currents.empty() && info.breakpoint_node != nullptr) {
        auto block = info.breakpoint_node->get_statement_block();
        print_statement_block(*block, false, false);
    }

    printer->pop_block();
    printer->pop_block();
}


/****************************************************************************************/
/*                              Print nrn_cur related routines                          */
/****************************************************************************************/


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_current(const BreakpointBlock& node) {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_cur_conductance_kernel(const BreakpointBlock& node) {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_cur_non_conductance_kernel() {
    return;
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_nrn_cur_kernel(const BreakpointBlock& node) {
    return;
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

    printer->add_newline(2);

    printer->fmt_line(
        "void {}(_nrn_model_sorted_token const& _sorted_token, NrnThread* _nt, Memb_list* _ml_arg, "
        "int _type) {{}}",
        method_name(naming::NRN_CUR_METHOD));

    /// TODO: Fill in
}


/****************************************************************************************/
/*                            Main code printing entry points                           */
/****************************************************************************************/

void CodegenNeuronCppVisitor::print_headers_include() {
    print_standard_includes();
    print_neuron_includes();
}


void CodegenNeuronCppVisitor::print_macro_definitions() {
    print_global_macros();
    print_mechanism_variables_macros();
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
                      std::to_string(float_variables_size()),
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
    /// TODO: More prints here?
}


void CodegenNeuronCppVisitor::print_namespace_begin() {
    print_namespace_start();
}


void CodegenNeuronCppVisitor::print_namespace_end() {
    print_namespace_stop();
}


void CodegenNeuronCppVisitor::print_data_structures(bool print_initializers) {
    print_mechanism_global_var_structure(print_initializers);
    print_mechanism_range_var_structure(print_initializers);
    print_make_instance();
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


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_compute_functions() {
    print_nrn_init();
    print_nrn_cur();
    print_nrn_state();
    print_nrn_jacob();
}


/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::print_codegen_routines() {
    print_backend_info();
    print_headers_include();
    print_macro_definitions();
    print_namespace_begin();
    print_nmodl_constants();
    print_prcellstate_macros();
    print_mechanism_info();
    print_data_structures(true);
    print_nrn_alloc();
    print_global_variables_for_hoc();
    print_compute_functions();  // only nrn_cur and nrn_state
    print_sdlists_init(true);
    print_mechanism_register();
    print_namespace_end();
}

void CodegenNeuronCppVisitor::print_net_send_call(const ast::FunctionCall& node) {
    throw std::runtime_error("Not implemented.");
}

void CodegenNeuronCppVisitor::print_net_move_call(const ast::FunctionCall& node) {
    throw std::runtime_error("Not implemented.");
}

void CodegenNeuronCppVisitor::print_net_event_call(const ast::FunctionCall& node) {
    throw std::runtime_error("Not implemented.");
}


/****************************************************************************************/
/*                            Overloaded visitor routines                               */
/****************************************************************************************/

/// TODO: Edit for NEURON
void CodegenNeuronCppVisitor::visit_watch_statement(const ast::WatchStatement& /* node */) {
    return;
}


}  // namespace codegen
}  // namespace nmodl
