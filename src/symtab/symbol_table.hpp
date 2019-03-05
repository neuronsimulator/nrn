/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <memory>
#include <vector>

#include "symtab/symbol.hpp"


namespace nmodl {
namespace symtab {

/**
 * \class SymbolTable
 * \brief Represent symbol table for nmodl block
 *
 * Symbol Table is used to track information about every block construct
 * encountered in the nmodl. In NMODL, block constructs are NEURON,
 * PARAMETER NET_RECEIVE etc. Each block is considered a new scope.
 *
 * NMODL supports nested block definitions (i.e. nested blocks). One
 * specific example of this is INITIAL block in NET_RECEIVE block. In this
 * case we need multiple scopes for single top level block of NMODL. In
 * the future if we enable block level scopes, we will need symbol tables
 * per block. Hence we are implementing BlockSymbolTable which stores
 * all symbol table information for specific NMODL block. Note that each
 * BlockSymbolTable implementation is recursive because while symbol
 * lookup we have to search first into local/current block. If lookup is
 * unsuccessfull then we have to traverse parent blocks until the end.
 *
 * \todo Revisit when clone method is used and implementation of copy
 *       constructor
 * \todo Name may not require as we have added AST node
 */
class SymbolTable {
  private:
    /**
     * \class Table
     * \brief Helper class for implementing symbol table
     *
     * Table is used to store information about every block construct
     * encountered in the nmodl file. Each symbol has name but for fast lookup,
     * we create map with the associated name.
     *
     * \todo Re-implement pretty printing
     */
    class Table {
        static int counter;

      public:
        /// map of symbol name and associated symbol for faster lookup
        std::vector<std::shared_ptr<Symbol>> symbols;

        /// insert new symbol into table
        void insert(const std::shared_ptr<Symbol>& symbol);

        /// check if symbol with given name exist
        std::shared_ptr<Symbol> lookup(const std::string& name) const;

        /// pretty print
        void print(std::stringstream& stream, std::string title, int indent);
    };
    /// name of the block
    std::string symtab_name;

    /// table holding all symbols in the current block
    Table table;

    /// pointer to ast node for which current block symbol created
    ast::AST* node = nullptr;

    /// true if current symbol table is global. blocks like neuron,
    /// parameter defines global variables and hence they go into
    /// single global symbol table
    bool global = false;

    /// pointer to the symbol table of parent block in the mod file
    SymbolTable* parent = nullptr;

    /// symbol table for each enclosing block in the current nmodl block
    /// construct. for example, for every block statement (like if, while,
    /// for) within function we append new symbol table. note that this is
    /// also required for nested blocks like INITIAL in NET_RECEIVE.
    std::map<std::string, std::shared_ptr<SymbolTable>> children;

  public:
    SymbolTable(std::string name, ast::AST* node, bool global = false)
        : symtab_name(name)
        , node(node)
        , global(global) {}

    SymbolTable(const SymbolTable& table);

    std::string name() const {
        return symtab_name;
    }

    std::string type() const;

    std::string title();

    bool is_method_defined(const std::string& name) const;

    /// todo: set token for every block from parser
    std::string position() const;

    bool global_scope() const {
        return global;
    }

    SymbolTable* get_parent_table() const {
        return parent;
    }

    std::string get_parent_table_name() {
        return parent ? parent->name() : "None";
    }

    std::shared_ptr<Symbol> lookup(const std::string& name) {
        return table.lookup(name);
    }

    void insert(std::shared_ptr<Symbol> symbol) {
        table.insert(symbol);
    }

    void set_parent_table(SymbolTable* block) {
        parent = block;
    }

    int symbol_count() const {
        return table.symbols.size();
    }

    /// \todo: revisit the usage as tokens will be pointing to old nodes
    SymbolTable* clone() {
        return new SymbolTable(*this);
    }

    std::shared_ptr<Symbol> lookup_in_scope(const std::string& name) const;

    std::vector<std::shared_ptr<Symbol>> get_variables(syminfo::NmodlType with,
                                                       syminfo::NmodlType without);

    std::vector<std::shared_ptr<Symbol>> get_variables_with_properties(
        syminfo::NmodlType properties,
        bool all = false);

    std::vector<std::shared_ptr<Symbol>> get_variables_with_status(syminfo::Status status,
                                                                   bool all = false);

    bool under_global_scope();

    void insert_table(const std::string& name, std::shared_ptr<SymbolTable> table);

    void print(std::stringstream& ss, int level);

    std::string to_string() {
        std::stringstream s;
        print(s, 0);
        return s.str();
    }
};

/**
 * \class ModelSymbolTable
 * \brief Represent symbol table for nmodl block
 *
 * SymbolTable is sufficient to hold information about all symbols in the
 * mod file. It might be sufficient to keep track of global symbol tables
 * and local symbol tables. But we construct symbol table using visitor
 * pass. In this case we visit ast and recursively create symbol table for
 * each block scope. In this case, ModelSymbolTable is provide interface
 * to build symbol table with visitor.
 *
 * \note For included mod file it's not clear yet whether we need to maintain
 *       separate ModelSymbolTable.
 * \note See command project in compiler teaching course for details
 *
 * \todo Unique name should be based on location. Use ModToken to get position.
 */
class ModelSymbolTable {
  private:
    /// symbol table for mod file (always top level symbol table)
    std::shared_ptr<SymbolTable> symtab = nullptr;

    /// current symbol table being constructed
    SymbolTable* current_symtab = nullptr;

    /// return unique name by appending some counter value
    std::string get_unique_name(const std::string& name, ast::AST* node, bool is_global);

    /// name of top level global symbol table
    const std::string GLOBAL_SYMTAB_NAME = "NMODL_GLOBAL";

    /// default mode of symbol table: if update is true then we update exisiting
    /// symbols otherwise we throw away old table and construct new one
    bool update_table = false;

    /// current order of variable being defined
    int definition_order = 0;

    /// if breakpoint block exist in the model
    bool breakpoint_exist = false;

    std::string model_suffix = "";

    /// insert symbol table in update mode
    std::shared_ptr<Symbol> update_mode_insert(const std::shared_ptr<Symbol>& symbol);

    void emit_message(const std::shared_ptr<Symbol>& first,
                      const std::shared_ptr<Symbol>& second,
                      bool redefinition);

    void update_order(const std::shared_ptr<Symbol>& present_symbol,
                      const std::shared_ptr<Symbol>& new_symbol);

  public:
    /// entering into new nmodl block
    SymbolTable* enter_scope(const std::string& name,
                             ast::AST* node,
                             bool global,
                             SymbolTable* node_symtab);

    /// leaving current nmodl block
    void leave_scope();

    /// insert new symbol into current table
    std::shared_ptr<Symbol> insert(const std::shared_ptr<Symbol>& symbol);

    /// lookup for symbol into current as well as all parent tables
    std::shared_ptr<Symbol> lookup(const std::string& name);

    /// pretty print
    void print(std::stringstream& ss);

    /// re-initialize members to throw away old symbol tables
    /// this is required as symtab visitor pass runs multiple time
    void set_mode(bool update_mode);

    bool has_breakpoint() const {
        return breakpoint_exist;
    }

    std::string model_name() const {
        return model_suffix;
    }
};

}  // namespace symtab
}  // namespace nmodl
