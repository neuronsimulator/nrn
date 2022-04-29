/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <utility>

#include "ast/ast.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/logger.hpp"
#include "utils/table_data.hpp"

namespace nmodl {
namespace symtab {

using namespace ast;
using syminfo::NmodlType;
using syminfo::Status;


int SymbolTable::Table::counter = 0;

/**
 *  Insert symbol into current symbol table. There are certain
 *  cases where we were getting re-insertion errors.
 */
void SymbolTable::Table::insert(const std::shared_ptr<Symbol>& symbol) {
    std::string name = symbol->get_name();
    if (lookup(name) != nullptr) {
        throw std::runtime_error("Trying to re-insert symbol " + name);
    }
    symbol->set_id(counter++);
    symbols.push_back(symbol);
}


std::shared_ptr<Symbol> SymbolTable::Table::lookup(const std::string& name) const {
    for (const auto& symbol: symbols) {
        if (symbol->get_name() == name) {
            return symbol;
        }
    }
    return nullptr;
}


SymbolTable::SymbolTable(const SymbolTable& table) {
    symtab_name = table.name();
    global = table.global_scope();
    node = nullptr;
    parent = nullptr;
}


bool SymbolTable::is_method_defined(const std::string& name) const {
    auto symbol = lookup_in_scope(name);
    if (symbol != nullptr) {
        auto node = symbol->get_node();
        if (node != nullptr) {
            if (node->is_procedure_block() || node->is_function_block()) {
                return true;
            }
        }
    }
    return false;
}


std::string SymbolTable::position() const {
    auto token = node->get_token();
    std::string position;
    if (token != nullptr) {
        position = token->position();
    } else {
        position = ModToken().position();
    }
    return position;
}


void SymbolTable::insert_table(const std::string& name, std::shared_ptr<SymbolTable> table) {
    if (children.find(name) != children.end()) {
        throw std::runtime_error("Trying to re-insert SymbolTable " + name);
    }
    children[name] = std::move(table);
}


/// return all symbol having any of the provided properties
std::vector<std::shared_ptr<Symbol>> SymbolTable::get_variables_with_properties(
    NmodlType properties,
    bool all) const {
    std::vector<std::shared_ptr<Symbol>> variables;
    for (auto& symbol: table.symbols) {
        if (all) {
            if (symbol->has_all_properties(properties)) {
                variables.push_back(symbol);
            }
        } else {
            if (symbol->has_any_property(properties)) {
                variables.push_back(symbol);
            }
        }
    }
    return variables;
}

/// return all symbol which has all "with" properties and none of the "without" properties
std::vector<std::shared_ptr<Symbol>> SymbolTable::get_variables(NmodlType with, NmodlType without) {
    auto variables = get_variables_with_properties(with, true);
    decltype(variables) result;
    for (auto& variable: variables) {
        if (!variable->has_any_property(without)) {
            result.push_back(variable);
        }
    }
    return result;
}


std::vector<std::shared_ptr<Symbol>> SymbolTable::get_variables_with_status(Status status,
                                                                            bool all) const {
    std::vector<std::shared_ptr<Symbol>> variables;
    for (auto& symbol: table.symbols) {
        if (all) {
            if (symbol->has_all_status(status)) {
                variables.push_back(symbol);
            }
        } else {
            if (symbol->has_any_status(status)) {
                variables.push_back(symbol);
            }
        }
    }
    return variables;
}


/**
 *  Check if current symbol table is in global scope
 *
 *  We create program scope at the top level and it has global scope.
 *  It contains neuron variables like t, dt, celsius etc. Then each
 *  nmodl block defining variables are added under this program's symbol
 *  table. Hence there are multiple levels of global scopes. In this
 *  helper function we make sure current block as well as it's parent are
 *  under global scopes.
 */
bool SymbolTable::under_global_scope() {
    bool global_scope = global;
    auto parent_table = parent;

    // traverse all parent blocks to make sure everyone is global
    while (global_scope && (parent_table != nullptr)) {
        parent_table = parent_table->parent;
        if (parent_table != nullptr) {
            global_scope = parent_table->global_scope();
        }
    }
    return global_scope;
}


/// lookup for symbol in current scope as well as all parents
std::shared_ptr<Symbol> SymbolTable::lookup_in_scope(const std::string& name) const {
    auto symbol = table.lookup(name);
    if (!symbol && (parent != nullptr)) {
        symbol = parent->lookup_in_scope(name);
    }
    return symbol;
}

/// lookup in current sytab as well as all parent symbol tables
std::shared_ptr<Symbol> ModelSymbolTable::lookup(const std::string& name) {
    if (current_symtab == nullptr) {
        throw std::logic_error("Lookup with previous symtab = nullptr ");
    }

    auto symbol = current_symtab->lookup(name);
    if (symbol) {
        return symbol;
    }

    // check into all parent symbol tables
    auto parent = current_symtab->get_parent_table();
    while (parent != nullptr) {
        symbol = parent->lookup(name);
        if (symbol) {
            break;
        }
        parent = parent->get_parent_table();
    }
    return symbol;
}


/**
 *  Emit warning message for shadowing definition or throw an exception
 *  if variable is being redefined in the same block.
 */
void ModelSymbolTable::emit_message(const std::shared_ptr<Symbol>& first,
                                    const std::shared_ptr<Symbol>& second,
                                    bool redefinition) {
    auto node = first->get_node();
    std::string name = first->get_name();
    auto properties = to_string(second->get_properties());
    std::string type = "UNKNOWN";
    if (node != nullptr) {
        type = node->get_node_type_name();
    }

    if (redefinition) {
        std::string msg = "Re-declaration of " + name + " [" + type + "]";
        msg += "<" + properties + "> in " + current_symtab->name();
        msg += " with one in " + second->get_scope();
        throw std::runtime_error(msg);
    }
    std::string msg = "SYMTAB :: " + name + " [" + type + "] in ";
    msg += current_symtab->name() + " shadows <" + properties;
    msg += "> definition in " + second->get_scope();
    logger->warn(msg);
}


/**
 * Insert symbol in the update mode i.e. symbol table is previously created
 * and we are adding new symbol table.
 *
 * We set status as "created" because missing symbol means the variable is
 * added by some intermediate passes.
 *
 * Consider inlining pass which creates a block like:
 *
 *  DERIVATIVE states() {
 *      LOCAL xx
 *      {
 *          LOCAL xx
 *      }
 *  }
 *
 *  Second xx is not added into symbol table (and hence not visible
 *  to other passes like local renamer). In this case we have to do
 *  local lookup in the current symtab and insert if doesn't exist.
 */
std::shared_ptr<Symbol> ModelSymbolTable::update_mode_insert(
    const std::shared_ptr<Symbol>& symbol) {
    symbol->set_scope(current_symtab->name());
    symbol->mark_created();

    std::string name = symbol->get_name();
    auto search_symbol = lookup(name);

    /// if no symbol found then safe to insert
    if (search_symbol == nullptr) {
        current_symtab->insert(symbol);
        return symbol;
    }

    /// for global scope just combine properties
    if (current_symtab->global_scope()) {
        search_symbol->add_properties(symbol->get_properties());
        return search_symbol;
    }

    /// insert into current block's symbol table
    if (current_symtab->lookup(name) == nullptr) {
        current_symtab->insert(symbol);
    }
    return symbol;
}

void ModelSymbolTable::update_order(const std::shared_ptr<Symbol>& present_symbol,
                                    const std::shared_ptr<Symbol>& new_symbol) {
    auto symbol = (present_symbol != nullptr) ? present_symbol : new_symbol;

    bool is_parameter = new_symbol->has_any_property(NmodlType::param_assign);
    bool is_assigned_definition = new_symbol->has_any_property(NmodlType::assigned_definition);

    if (symbol->get_definition_order() == -1) {
        if (is_parameter || is_assigned_definition) {
            symbol->set_definition_order(definition_order++);
        }
    }
}

std::shared_ptr<Symbol> ModelSymbolTable::insert(const std::shared_ptr<Symbol>& symbol) {
    if (current_symtab == nullptr) {
        throw std::logic_error("Can not insert symbol without entering scope");
    }

    auto search_symbol = lookup(symbol->get_name());
    update_order(search_symbol, symbol);

    /// handle update mode insertion
    if (update_table) {
        return update_mode_insert(symbol);
    }

    symbol->set_scope(current_symtab->name());

    // if no symbol found then safe to insert
    if (search_symbol == nullptr) {
        current_symtab->insert(symbol);
        return symbol;
    }

    /**
     *  For global symbol tables, same variable can appear in multiple
     *  nmodl "global" blocks. It's an error if it appears multiple times
     *  in the same nmodl block. To check this we compare symbol properties
     *  which are bit flags. If they are not same that means, we have to
     *  add new properties to existing symbol. If the properties are same
     *  that means symbol are duplicate.
     */
    if (current_symtab->global_scope()) {
        if (search_symbol->has_any_property(symbol->get_properties())) {
            emit_message(symbol, search_symbol, true);
        } else {
            search_symbol->add_properties(symbol->get_properties());
        }
        return search_symbol;
    }

    /**
     *  For non-global scopes, check if symbol that we found has same
     *  scope name as symbol table. This means variable is being re-declared
     *  within the same scope. Otherwise, there is variable with same name
     *  in parent scopes and it will shadow the definition. In this case just
     *  emit the warning and insert the symbol.
     *
     *  Note:
     *  We suppress the warning for the voltage since in most cases it is extern
     *  as well as argument and it is fine like that.
     */
    if (search_symbol->get_scope() == current_symtab->name()) {
        emit_message(symbol, search_symbol, true);
    } else {
        /**
         * Suppress warning for voltage since it is often extern and argument.
         */
        if (symbol->get_name() != "v") {
            emit_message(symbol, search_symbol, false);
        }
        current_symtab->insert(symbol);
    }
    return symbol;
}


/**
 *  Some blocks can appear multiple times in the nmodl file. In order to distinguish
 *  them we simply append counter.
 *  \todo We should add position information to make name unique
 */
std::string ModelSymbolTable::get_unique_name(const std::string& name, Ast* node, bool is_global) {
    static int block_counter = 0;
    std::string new_name(name);
    if (is_global) {
        new_name = GLOBAL_SYMTAB_NAME;
    } else if (node->is_statement_block() || node->is_solve_block() || node->is_before_block() ||
               node->is_after_block()) {
        new_name += std::to_string(block_counter++);
    }
    return new_name;
}


/**
 *  Function callback at the entry of every block in nmodl file
 *  Every block starts a new scope and hence new symbol table is created.
 *  The same symbol table is returned so that visitor can store pointer to
 *  symbol table within a node.
 */
SymbolTable* ModelSymbolTable::enter_scope(const std::string& name,
                                           Ast* node,
                                           bool global,
                                           SymbolTable* node_symtab) {
    if (node == nullptr) {
        throw std::runtime_error("Can't enter with empty node");
    }

    /**
     *  All global blocks in mod file have same global symbol table. If there
     *  is already symbol table setup in global scope, return the same.
     */
    if (symtab && global) {
        return symtab.get();
    }

    /// statement block within global scope is part of global block itself
    if (symtab && node->is_statement_block() && current_symtab->under_global_scope()) {
        return symtab.get();
    }

    if (node_symtab == nullptr || !update_table) {
        auto new_name = get_unique_name(name, node, global);
        auto new_symtab = std::make_shared<SymbolTable>(new_name, node, global);
        new_symtab->set_parent_table(current_symtab);
        if (symtab == nullptr) {
            symtab = new_symtab;
        }
        if (current_symtab != nullptr) {
            current_symtab->insert_table(new_name, new_symtab);
        }
        node_symtab = new_symtab.get();
    }
    current_symtab = node_symtab;
    return current_symtab;
}


/**
 *  Callback at the exit of every block in nmodl file. When we reach
 *  program node (top most level), there is no parent block and hence
 *  use top level symbol table. Otherwise traverse back to parent.
 */
void ModelSymbolTable::leave_scope() {
    if (current_symtab == nullptr) {
        throw std::logic_error("Trying leave scope without entering");
    }
    if (current_symtab != nullptr) {
        current_symtab = current_symtab->get_parent_table();
    }
    if (current_symtab == nullptr) {
        current_symtab = symtab.get();
    }
}


/**
 * Update mode is true if we want to re-use exisiting symbol table.
 * If there is no symbol table constructed then we toggle mode.
 */
void ModelSymbolTable::set_mode(bool update_mode) {
    if (update_mode && symtab == nullptr) {
        update_mode = false;
    }
    update_table = update_mode;
    if (!update_table) {
        symtab = nullptr;
        current_symtab = nullptr;
    }
    definition_order = 0;
}


//=============================================================================
// Symbol table pretty-printing functions
//=============================================================================


void SymbolTable::Table::print(std::ostream& stream, std::string title, int indent) const {
    using stringutils::text_alignment;
    using utils::TableData;
    if (!symbols.empty()) {
        TableData table;
        table.title = std::move(title);
        table.headers = {
            "NAME", "PROPERTIES", "STATUS", "LOCATION", "VALUE", "# READS", "# WRITES"};
        table.alignments = {text_alignment::left,
                            text_alignment::left,
                            text_alignment::right,
                            text_alignment::right,
                            text_alignment::right};

        for (const auto& symbol: symbols) {
            auto is_external = symbol->is_external_variable();
            auto read_count = symbol->get_read_count();
            auto write_count = symbol->get_write_count();

            // do not print external symbols which are not used in the current model
            if (is_external && read_count == 0 && write_count == 0) {
                continue;
            }

            auto name = symbol->get_name();
            if (symbol->is_array()) {
                name += "[" + std::to_string(symbol->get_length()) + "]";
            }
            auto position = symbol->get_token().position();
            auto properties = syminfo::to_string(symbol->get_properties());
            auto status = syminfo::to_string(symbol->get_status());
            auto reads = std::to_string(symbol->get_read_count());
            std::string value;
            auto sym_value = symbol->get_value();
            if (sym_value) {
                value = std::to_string(*sym_value);
            }
            auto writes = std::to_string(symbol->get_write_count());
            table.rows.push_back({name, properties, status, position, value, reads, writes});
        }
        table.print(stream, indent);
    }
}


/// construct title for symbol table
std::string SymbolTable::title() const {
    auto node_type = node->get_node_type_name();
    auto name = symtab_name + " [" + node_type + " IN " + get_parent_table_name() + "] ";
    auto location = "POSITION : " + position();
    auto scope = global ? "GLOBAL" : "LOCAL";
    return name + location + " SCOPE : " + scope;
}


void SymbolTable::print(std::ostream& ss, int level) const {
    table.print(ss, title(), level);

    /// when current symbol table is empty, the children
    /// can be printed from the same indentation level
    /// (this is to avoid unnecessary empty indentations)
    auto next_level = level;
    if (table.symbols.empty()) {
        next_level--;
    }

    /// recursively print all children tables
    for (const auto& item: children) {
        if (item.second->symbol_count() >= 0) {
            (item.second)->print(ss, ++next_level);
            next_level--;
        }
    }
}

}  // namespace symtab
}  // namespace nmodl
