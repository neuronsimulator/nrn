/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief Variour types to store code generation specific information
 */

#include <string>
#include <unordered_set>

#include "ast/ast.hpp"
#include "symtab/symbol_table.hpp"

namespace nmodl {
namespace codegen {

/**
 * @addtogroup codegen_details
 * @{
 */

/**
 * \class Conductance
 * \brief Represent conductance statements used in mod file
 */
struct Conductance {
    /// name of the ion
    std::string ion;

    /// ion variable like intra/extra concentration
    std::string variable;
};


/**
 * \class Ion
 * \brief Represent ions used in mod file
 */
struct Ion {
    /// name of the ion
    std::string name;

    /// ion variables that are being read
    std::vector<std::string> reads;

    /// ion variables that are being written
    std::vector<std::string> writes;

    /// if style semantic needed
    bool need_style = false;

    Ion() = delete;

    Ion(std::string name)
        : name(std::move(name)) {}

    /**
     * Check if variable name is a ionic current
     *
     * This is equivalent of IONCUR flag in mod2c.
     * If it is read variable then also get NRNCURIN flag.
     * If it is write variables then also get NRNCUROUT flag.
     */
    bool is_ionic_current(const std::string& text) const {
        return text == ("i" + name);
    }

    /**
     * Check if variable name is internal cell concentration
     *
     * This is equivalent of IONIN flag in mod2c.
     */
    bool is_intra_cell_conc(const std::string& text) const {
        return text == (name + "i");
    }

    /**
     * Check if variable name is external cell concentration
     *
     * This is equivalent of IONOUT flag in mod2c.
     */
    bool is_extra_cell_conc(const std::string& text) const {
        return text == (name + "o");
    }

    /**
     * Check if variable name is reveral potential
     *
     * This is equivalent of IONEREV flag in mod2c.
     */
    bool is_rev_potential(const std::string& text) const {
        return text == ("e" + name);
    }

    /// check if it is either internal or external concentration
    bool is_ionic_conc(const std::string& text) const {
        return is_intra_cell_conc(text) || is_extra_cell_conc(text);
    }

    /// for a given ion, return different variable names/properties
    /// like internal/external concentration, reversial potential,
    /// ionic current etc.
    static std::vector<std::string> get_possible_variables(const std::string& ion_name) {
        return {"i" + ion_name, ion_name + "i", ion_name + "o", "e" + ion_name};
    }
};


/**
 * \class IndexSemantics
 * \brief Represent semantic information for index variable
 */
struct IndexSemantics {
    /// start position in the int array
    int index;

    /// name/type of the variable (i.e. semantics)
    std::string name;

    /// number of elements (typically one)
    int size;

    IndexSemantics() = delete;
    IndexSemantics(int index, std::string name, int size)
        : index(index)
        , name(name)
        , size(size) {}
};


/**
 * \class CodegenInfo
 * \brief Represent information collected from AST for code generation
 *
 * Code generation passes require different information from AST. This
 * information is gathered in this single class.
 *
 * \todo Need to store all Define i.e. macro definitions?
 */
struct CodegenInfo {
    /// name of mod file
    std::string mod_file;

    /// name of the suffix
    std::string mod_suffix;

    /// true if mod file is vectorizable (which should be always true for coreneuron)
    /// But there are some blocks like LINEAR are not thread safe in neuron or mod2c
    /// context. In this case vectorize is used to determine number of float variable
    /// in the data array (e.g. v). For such non thread methods or blocks vectorize is
    /// false.
    bool vectorize = true;

    /// if mod file is thread safe (always true for coreneuron)
    bool thread_safe = true;

    /// if mod file is point process
    bool point_process = false;

    /// if mod file is artificial cell
    bool artificial_cell = false;

    /// if electrode current specified
    bool electrode_current = false;

    /// if thread thread call back routines need to register
    bool thread_callback_register = false;

    /// if bbcore pointer is used
    bool bbcore_pointer_used = false;

    /// if write concentration call required in initial block
    bool write_concentration = false;

    /// if net_send function is used
    bool net_send_used = false;

    /// if net_even function is used
    bool net_event_used = false;

    /// if diam is used
    bool diam_used = false;

    /// if area is used
    bool area_used = false;

    /// if for_netcon is used
    bool for_netcon_used = false;

    /// number of watch expressions
    int watch_count = 0;

    // number of table statements
    int table_count = 0;

    /**
     * thread_data_index indicates number of threads being allocated.
     * For example, if there is derivimplicit method used, then two thread
     * structures are created. When we print global variables then
     * thread_data_index is used to indicate whats next thread id to use.
     */
    int thread_data_index = 0;

    /**
     * Top local variables are those local variables that appear in global scope.
     * Thread structure is created for top local variables and doesn't thread id
     * 0. For example, if derivimplicit method is used then thread id 0 is assigned
     * to those structures first. And then next thread id is assigned for top local
     * variables. The idea of thread is assignement is to have same order for variables
     * between neuron and coreneuron.
     */

    /// thread id for top local variables
    int top_local_thread_id = 0;

    /// total length of all top local variables
    int top_local_thread_size = 0;

    /// thread id for thread promoted variables
    int thread_var_thread_id = 0;

    /// sum of length of thread promoted variables
    int thread_var_data_size = 0;

    /// thread id for derivimplicit variables
    int derivimplicit_var_thread_id = -1;

    /// slist/dlist id for derivimplicit block
    int derivimplicit_list_num = -1;

    /// number of solve blocks in mod file
    int num_solve_blocks = 0;

    /// number of primes (all state variables not necessary to be prime)
    int num_primes = 0;

    /// sum of length of all prime variables
    int primes_size = 0;

    /// number of equations (i.e. statements) in derivative block
    /// typically equal to number of primes
    int num_equations = 0;

    /// derivative block
    const ast::BreakpointBlock* breakpoint_node = nullptr;

    /// nrn_state block
    const ast::NrnStateBlock* nrn_state_block = nullptr;

    /// net receive block for point process
    const ast::NetReceiveBlock* net_receive_node = nullptr;

    /// number of arguments to net_receive block
    int num_net_receive_parameters = 0;

    /// initial block within net receive block
    const ast::InitialBlock* net_receive_initial_node = nullptr;

    /// initial block
    const ast::InitialBlock* initial_node = nullptr;

    /// constructor block
    const ast::ConstructorBlock* constructor_node = nullptr;

    /// destructor block only for point process
    const ast::DestructorBlock* destructor_node = nullptr;

    /// all procedures defined in the mod file
    std::vector<const ast::ProcedureBlock*> procedures;

    /// derivimplicit callbacks need to be emited
    std::vector<const ast::DerivimplicitCallback*> derivimplicit_callbacks;

    /// all functions defined in the mod file
    std::vector<const ast::FunctionBlock*> functions;

    /// all factors defined in the mod file
    std::vector<const ast::FactorDef*> factor_definitions;

    /// ions used in the mod file
    std::vector<Ion> ions;

    using SymbolType = std::shared_ptr<symtab::Symbol>;

    /// range variables which are parameter as well
    std::vector<SymbolType> range_parameter_vars;

    /// range variables which are assigned variables as well
    std::vector<SymbolType> range_assigned_vars;

    /// reamining assigned variables
    std::vector<SymbolType> assigned_vars;

    /// all state variables
    std::vector<SymbolType> state_vars;

    /// state variables excluding such useion read/write variables
    /// that are not ionic currents. In neuron/mod2c these are stored
    /// in the list "rangestate".
    std::vector<SymbolType> range_state_vars;

    /// local variables in the global scope
    std::vector<SymbolType> top_local_variables;

    /// pointer or bbcore pointer variables
    std::vector<SymbolType> pointer_variables;

    /// index/offset for first pointer variable if exist
    int first_pointer_var_index = -1;

    /// tqitem index in integer variables
    /// note that if tqitem doesn't exist then the default value should be 0
    int tqitem_index = 0;

    // updated dt to use with steadystate solver (in initial block)
    // empty string means no change in dt
    std::string changed_dt;

    /// global variables
    std::vector<SymbolType> global_variables;

    /// constant variables
    std::vector<SymbolType> constant_variables;

    /// thread variables (e.g. global variables promoted to thread)
    std::vector<SymbolType> thread_variables;

    /// new one used in print_ion_types
    std::vector<SymbolType> use_ion_variables;

    /// this is the order in which they appear in derivative block
    /// this is required while printing them in initlist function
    std::vector<SymbolType> prime_variables_by_order;

    /// table variables
    std::vector<SymbolType> table_statement_variables;
    std::vector<SymbolType> table_assigned_variables;

    /// [Core]NEURON global variables used (e.g. celsius) and their types
    std::vector<std::pair<SymbolType, std::string>> neuron_global_variables;

    /// function or procedures with table statement
    std::vector<const ast::Block*> functions_with_table;

    /// represent conductance statements used in mod file
    std::vector<Conductance> conductances;

    /// index variable semantic information
    std::vector<IndexSemantics> semantics;

    /// non specific and ionic currents
    std::vector<std::string> currents;

    /// all top level global blocks
    std::vector<ast::Node*> top_blocks;

    /// all top level verbatim blocks
    std::vector<ast::Node*> top_verbatim_blocks;

    /// all watch statements
    std::vector<const ast::WatchStatement*> watch_statements;

    /// all before after blocks
    std::vector<const ast::Block*> before_after_blocks;

    /// all variables/symbols used in the verbatim block
    std::unordered_set<std::string> variables_in_verbatim;

    /// true if eigen newton solver is used
    bool eigen_newton_solver_exist = false;

    /// true if eigen linear solver is used
    bool eigen_linear_solver_exist = false;

    /// if any ion has write variable
    bool ion_has_write_variable() const;

    /// if given variable is ion write variable
    bool is_ion_write_variable(const std::string& name) const;

    /// if given variable is ion read variable
    bool is_ion_read_variable(const std::string& name) const;

    /// if either read or write variable
    bool is_ion_variable(const std::string& name) const;

    /// if given variable is a current
    bool is_current(const std::string& name) const;

    /// if given variable is a ionic current
    bool is_ionic_current(const std::string& name) const;

    /// if given variable is a ionic concentration
    bool is_ionic_conc(const std::string& name) const;

    /// if watch statements are used
    bool is_watch_used() const noexcept {
        return watch_count > 0;
    }

    bool emit_table_thread() const noexcept {
        return (table_count > 0 && vectorize == true);
    }

    /// if legacy derivimplicit solver from coreneuron to be used
    inline bool derivimplicit_used() const {
        return !derivimplicit_callbacks.empty();
    }

    bool function_uses_table(std::string& name) const;

    /// true if EigenNewtonSolver is used in nrn_state block
    bool nrn_state_has_eigen_solver_block() const;

    /// true if WatchStatement uses voltage v variable
    bool is_voltage_used_by_watch_statements() const;

    /// if we need a call back to wrote_conc in neuron/coreneuron
    bool require_wrote_conc = false;
};

/** @} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
