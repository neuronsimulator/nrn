#ifndef _NMODL_SYMTAB_HPP_
#define _NMODL_SYMTAB_HPP_

#include <map>

#include "symtab/symbol.hpp"

namespace symtab {

    using namespace details;
    using namespace ast;

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
      public:
        /// map of symbol name and associated symbol for faster lookup
        std::map<std::string, std::shared_ptr<Symbol>> symbols;

        /// insert new symbol into table
        void insert(std::shared_ptr<Symbol> sym);

        /// check if symbol with given name exist
        std::shared_ptr<Symbol> lookup(std::string name);

        /// pretty print
        void print(std::stringstream& stream, std::string title, int indent);
    };

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
        /// name of the block
        std::string symtab_name;

        /// table holding all symbols in the current block
        Table table;

        /// pointer to ast node for which current block symbol created
        AST* node = nullptr;

        /// true if current symbol table is global. blocks like neuron,
        /// parameter defines global variables and hence they go into
        /// single global symbol table
        bool global = false;

        /// pointer to the symbol table of parent block in the mod file
        std::shared_ptr<SymbolTable> parent = nullptr;

        /// symbol table for each enclosing block in the current nmodl block
        /// construct. for example, for every block statement (like if, while,
        /// for) within function we append new symbol table. note that this is
        /// also required for nested blocks like INITIAL in NET_RECEIVE.
        std::map<std::string, std::shared_ptr<SymbolTable>> children;

        /// pretty print table
        void print_table(std::stringstream& ss, int indent);

      public:
        SymbolTable(std::string name, AST* node, bool global = false)
            : symtab_name(name), node(node), global(global) {
        }

        SymbolTable(const SymbolTable& table);

        std::string name() const {
            return symtab_name;
        }

        std::string type() const {
            return node->getTypeName();
        }

        std::string title();

        /// todo: set token for every block from parser
        std::string position() const {
            auto token = node->getToken();
            if (token)
                return token->position();
            else
                return ModToken().position();
        }

        bool global_scope() const {
            return global;
        }

        std::shared_ptr<SymbolTable> get_parent_table() const {
            return parent;
        }

        std::string get_parent_table_name() {
            return parent ? parent->name() : "None";
        }

        std::shared_ptr<Symbol> lookup(std::string name) {
            return table.lookup(name);
        }

        void insert(std::shared_ptr<Symbol> symbol) {
            table.insert(symbol);
        }

        void set_parent_table(std::shared_ptr<SymbolTable> block) {
            parent = block;
        }

        int symbol_count() const {
            return table.symbols.size();
        }

        SymbolTable* clone() {
            return new SymbolTable(*this);
        }

        std::shared_ptr<Symbol> lookup_in_scope(std::string name);

        std::vector<std::shared_ptr<Symbol>> get_global_variables();

        bool under_global_scope();

        void insert_table(std::string name, std::shared_ptr<SymbolTable> table);

        void print(std::stringstream& ss, int indent);
    };

    /**
     * \class ModelSymbolTable
     * \brief Represent symbol table for nmodl block
     *
     * SymbolTable is sufficinet to hold information about all symbols in the
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

        /// symbol table for parent block (used during symbol table construction)
        std::shared_ptr<SymbolTable> parent_symtab = nullptr;

        /// Return unique name by appending some counter value
        std::string get_unique_name(std::string name, AST* node);

        /// name of top level global symbol table
        const std::string GLOBAL_SYMTAB_BANE = "NMODL_GLOBAL";

      public:
        /// entering into new nmodl block
        std::shared_ptr<SymbolTable> enter_scope(std::string name, AST* node, bool global);

        /// leaving current nmodl block
        void leave_scope();

        void insert(std::shared_ptr<Symbol> symbol);

        std::shared_ptr<Symbol> lookup(std::string name);

        /// pretty print
        void print(std::stringstream& ss);
    };

}  // namespace symtab

#endif