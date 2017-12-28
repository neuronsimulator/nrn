#include "lexer/token_mapping.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/table_data.hpp"
#include "utils/string_utils.hpp"

namespace symtab {

    /** Insert symbol into current symbol table. There are certain
     *  cases where we were getting re-insertion errors.
     *
     * \todo Revisit the error handling
     */
    void Table::insert(std::shared_ptr<Symbol> symbol) {
        std::string name = symbol->get_name();
        if (symbols.find(name) != symbols.end()) {
            throw std::runtime_error("Trying to re-insert symbol " + name);
        }
        symbols[name] = symbol;
    }

    std::shared_ptr<Symbol> Table::lookup(std::string name) {
        if (symbols.find(name) == symbols.end()) {
            return nullptr;
        }
        return symbols[name];
    }

    SymbolTable::SymbolTable(const SymbolTable& table) {
        symtab_name = table.name();
        global = table.global_scope();
        node = nullptr;
        parent = nullptr;
    }

    /// insert new symbol table of one of the children block
    void SymbolTable::insert_table(std::string name, std::shared_ptr<SymbolTable> table) {
        if (children.find(name) != children.end()) {
            throw std::runtime_error("Trying to re-insert SymbolTable " + name);
        }
        children[name] = table;
    }

    /** Get all symbols which can be used in global scope. Note that
     *  this is different from GLOBAL variable type in the mod file. Here
     *  global meaning all variables that can be used in entire mod file.
     *  This is used from optimization passes.
     *
     *  \todo Voltage v can be global variable as well as external. In order
     *        to avoid optimizations, we need to handle this case properly
     *
     *  \todo Instead of ast node, use symbol properties to check variable type
     */
    std::vector<std::shared_ptr<Symbol>> SymbolTable::get_global_variables() {
        std::vector<std::shared_ptr<Symbol>> variables;
        for (auto& syminfo : table.symbols) {
            auto symbol = syminfo.second;
            SymbolInfo property = NmodlInfo::range_var;
            property |= NmodlInfo::dependent_def | NmodlInfo::param_assign;
            if (symbol->has_properties(property)) {
                variables.push_back(symbol);
            }
        }
        return variables;
    }

    /** Check if current symbol table is in global scope
     *  We create program scope at the top level and it has global scope.
     *  It contains neuron variables like t, dt, celsius etc. Then each
     *  nmodl block defining variables are added under this program's symbol
     *  table. Hence there are multiple levels of global scopes. In this
     *  helper function we make sure current block as well as it's parent are
     *  under global scopes.
     *
     *  \todo It seems not necessary to traverse all parents if current block
     *        itself is with global scope.
     */
    bool SymbolTable::under_global_scope() {
        bool global_scope = global;
        auto parent_table = parent;

        // traverse all parent blocks to make sure everyone is global
        while (global_scope && parent_table) {
            parent_table = parent_table->parent;
            if (parent_table) {
                global_scope = parent_table->global_scope();
            }
        }
        return global_scope;
    }

    /// lookup for symbol in current scope as well as all parents
    std::shared_ptr<Symbol> SymbolTable::lookup_in_scope(std::string name) {
        auto symbol = table.lookup(name);
        if (!symbol && parent) {
            symbol = parent->lookup_in_scope(name);
        }
        return symbol;
    }

    /// lookup in all parents symbol table
    /// \todo parent_symtab is somewhat misleading. it's actually
    ///       current symbol table that is under process. we should
    ///       rename it. note that we still need parent symbol table
    ///       filed to traverse the parents. revisit this usage.
    std::shared_ptr<Symbol> ModelSymbolTable::lookup(std::string name) {
        /// parent symbol is not set means symbol table is
        /// is not used with visitor at all. it would be ok
        // to just return nullptr?
        if (!parent_symtab) {
            throw std::logic_error("Lookup wit previous symtab = nullptr ");
        }

        auto symbol = parent_symtab->lookup(name);
        if (symbol) {
            return symbol;
        }

        // check into all parent symbol tables
        auto parent = parent_symtab->get_parent_table();
        while (parent) {
            symbol = parent->lookup(name);
            if (symbol) {
                break;
            }
            parent = parent->get_parent_table();
        }
        return symbol;
    }

    void ModelSymbolTable::insert(std::shared_ptr<Symbol> symbol) {
        if (parent_symtab == nullptr) {
            throw std::logic_error("Can not insert symbol without entering scope");
        }

        symbol->set_scope(parent_symtab->name());
        std::string name = symbol->get_name();
        auto search_symbol = lookup(name);

        // if no symbol found then safe to insert
        if (search_symbol == nullptr) {
            parent_symtab->insert(symbol);
            return;
        }

        // properties and type information for error reporting
        auto properties = to_string(search_symbol->get_properties());
        auto node = symbol->get_node();
        std::string type = "UNKNOWN";
        if (node) {
            type = node->getTypeName();
        }

        /** For global symbol tables, same variable can appear in multiple
         *  nmodl "global" blocks. It's an error if it appears multiple times
         *  in the same nmodl block. To check this we compare symbol properties
         *  which are bit flags. If they are not same that means, we have to
         *  add new properties to existing symbol. If the properties are same
         *  that means symbol are duplicate.
         *
         *  \todo Error handling should go to logger instead of exception
         */
        if (parent_symtab->global_scope()) {
            if (search_symbol->has_properties(symbol->get_properties())) {
                std::string msg = GLOBAL_SYMTAB_BANE + " has re-declaration of ";
                msg += name + " <" + type + "> " + properties;
                throw std::runtime_error(msg);
            } else {
                search_symbol->combine_properties(symbol->get_properties());
            }
        } else {
            /** For non-global scopes, check if symbol that we found has same
             *  scope name as symbol table. This means variable is being re-declared
             *  within the same scope. Otherwise, there is variable with same name
             *  in parent scopes and it will shadow the definition. In this case just
             *  emit the warning and insert the symbol.
             */
            if (search_symbol->get_scope() == parent_symtab->name()) {
                std::string msg = "Re-declaration of " + name + " [" + type + "]";
                msg += "<" + properties + "> in " + parent_symtab->name();
                msg += " with one in " + search_symbol->get_scope();
                throw std::runtime_error(msg);
            } else {
                std::string msg = "SYMTAB WARNING: " + name + " [" + type + "] in ";
                msg += parent_symtab->name() + " shadows <" + properties;
                msg += "> definition in " + search_symbol->get_scope();
                std::cout << msg << "\n";
                parent_symtab->insert(symbol);
            }
        }
    }

    /** Some blocks can appear multiple times in the nmodl file. In order to distinguish
     *  them we simple append counter.
     *  \todo We should add position information to make name unique
     */
    std::string ModelSymbolTable::get_unique_name(std::string name, AST* node) {
        static int block_counter = 0;
        if (node->isStatementBlock() || node->isSolveBlock() || node->isBeforeBlock() ||
            node->isAfterBlock()) {
            name += std::to_string(block_counter++);
        }
        return name;
    }

    /** Function callback at the entry of every block in nmodl file
     *  Every block starts a new scope and hence new symbol table is created.
     *  The same symbol table is returned so that visitor can store pointer to
     *  symbol table within a node.
     */
    std::shared_ptr<SymbolTable> ModelSymbolTable::enter_scope(std::string name,
                                                               AST* node,
                                                               bool global) {
        if (node == nullptr) {
            throw std::runtime_error("Can't enter with empty node");
        }

        /// all global blocks in mod file have same symbol table
        if (symtab && global) {
            parent_symtab = symtab;
            return parent_symtab;
        }

        /// change name for global symbol table
        if (global) {
            name = GLOBAL_SYMTAB_BANE;
        }

        /// statement block within global scope is part of global block itself
        if (symtab && node->isStatementBlock() && parent_symtab->under_global_scope()) {
            parent_symtab = symtab;
            return parent_symtab;
        }

        // new symbol table for current block with unique name
        name = get_unique_name(name, node);
        auto new_symtab = std::make_shared<SymbolTable>(name, node, global);

        // symtab is nullptr when we are entering into very fitst block
        // otherwise we have to inset new symbol table, set parent symbol
        // table and then new symbol table becomes parent for future blocks
        if (symtab == nullptr) {
            symtab = new_symtab;
            parent_symtab = new_symtab;
        } else {
            parent_symtab->insert_table(name, new_symtab);
            new_symtab->set_parent_table(parent_symtab);
            parent_symtab = new_symtab;
        }
        return new_symtab;
    }

    /** Callback at the exit of every block in nmodl file When we reach
     *  program node (top most level), there is no parent block and use
     *  same symbol table. Otherwise traverse back to prent.
     */
    void ModelSymbolTable::leave_scope() {
        if (parent_symtab == nullptr && symtab == nullptr) {
            throw std::logic_error("Trying leave scope without entering");
        }

        /// if has parent scope, setup new parent symbol table
        if (parent_symtab) {
            parent_symtab = parent_symtab->get_parent_table();
        }

        /// \todo : when parent is nullptr, parent should not be reset to
        ///         current symbol table. this is happening for global
        ///         scope symbol table
        if (parent_symtab == nullptr) {
            parent_symtab = symtab;
        }
    }

    //=============================================================================
    // Revisit implementation of symbol table pretty printing functions : should
    // be refactored into separate table printer function.
    //=============================================================================

    void Table::print(std::stringstream& stream, std::string title, int indent) {
        if (symbols.size()) {
            TableData table;
            table.title = title;
            table.headers = {"NAME", "PROPERTIES", "LOCATION", "# READS", "# WRITES"};
            table.alignments = {text_alignment::left, text_alignment::left, text_alignment::right,
                                text_alignment::right, text_alignment::right};

            for (const auto& syminfo : symbols) {
                auto symbol = syminfo.second;
                auto is_external = symbol->is_external_symbol_only();
                auto read_count = symbol->get_read_count();
                auto write_count = symbol->get_write_count();

                // do not print external symbols which are not used in the current model
                if (is_external && read_count == 0 && write_count == 0) {
                    continue;
                }

                auto name = syminfo.first;
                auto position = symbol->get_token().position();
                auto properties = to_string(symbol->get_properties());
                auto reads = std::to_string(symbol->get_read_count());
                auto writes = std::to_string(symbol->get_write_count());
                table.rows.push_back({name, properties, position, reads, writes});
            }
            table.print(stream, indent);
        }
    }

    std::string SymbolTable::title() {
        /// construct title for symbol table
        auto name = symtab_name + " [" + type() + " IN " + get_parent_table_name() + "] ";
        auto location = "POSITION : " + position();
        auto scope = global ? "GLOBAL" : "LOCAL";
        return name + location + " SCOPE : " + scope;
    }

    void SymbolTable::print(std::stringstream& ss, int level) {
        table.print(ss, title(), level);

        /// when current symbol table is empty, the childrens
        /// can be printed from the same indentation level
        /// (this is to avoid unnecessary empty indentations)
        auto next_level = level;
        if (table.symbols.size() == 0) {
            next_level--;
        }

        /// recursively print all children tables
        for (const auto& item : children) {
            if (item.second->symbol_count() >= 0) {
                (item.second)->print(ss, ++next_level);
                next_level--;
            }
        }
    }

    void ModelSymbolTable::print(std::stringstream& ss) {
        symtab->print(ss, 0);
    }
}  // namespace symtab