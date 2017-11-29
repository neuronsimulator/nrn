#include "symtab/symbol_table.hpp"
#include "lexer/token_mapping.hpp"

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

    std::shared_ptr<Symbol> Table::lookup(std::string sname) {
        if (symbols.find(sname) == symbols.end()) {
            return nullptr;
        }
        return symbols[sname];
    }

    SymbolTable::SymbolTable(const SymbolTable& table) {
        symtab_name = table.name();
        global = table.global_scope();
        node = nullptr;
        parent = nullptr;
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
            auto node = symbol->get_node();
            if ((node->isRangeVar() || node->isDependentDef() || node->isParamAssign())) {
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
    /// \todo not sure why we are searching in parent symtab only. Also we could
    ///       also use lookup_in_scope partly. revisit this usage.
    std::shared_ptr<Symbol> ModelSymbolTable::lookup(std::string name) {
        if (!parent_symtab) {
            throw std::logic_error("Lookup wit previous symtab = nullptr ");
        }

        auto symbol = parent_symtab->lookup(name);
        if (symbol) {
            return symbol;
        }

        // check into all parents
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

    void SymbolTable::insert_table(std::string name, std::shared_ptr<SymbolTable> table) {
        if (childrens.find(name) != childrens.end()) {
            throw std::runtime_error("Trying to re-insert SymbolTable " + name);
        }
        childrens[name] = table;
    }

    void ModelSymbolTable::insert(std::shared_ptr<Symbol> symbol) {
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
        auto type = symbol->get_node()->getTypeName();

        /** For global symbol tables, same variable can appear in multiple
         *  nmodl "global" blocks. It's an error if it appears multiple times
         *  in the same nmodl block. To check this we compare symbol properties
         *  which are bit flags. If they are not same that means, we have to
         *  add new properties to existing symbol. If the properties are same
         *  that means symbol are duplicate.
         */
        if (parent_symtab->global_scope()) {
            if (search_symbol->has_common_properties(symbol->get_properties())) {
                std::string msg = "SYMTAB ERROR: _NMODL_GLOBAL_ has re-declaration of ";
                msg += name + " <" + type + "> " + properties;
                std::cout << msg << "\n";
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
                std::string msg = "SYMTAB ERROR: Re-declaration of " + name;
                msg += " [" + type + "]" + " <" + properties + "> in ";
                msg += parent_symtab->name() + " with one in " + search_symbol->get_scope();
                std::cout << msg << "\n";
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

    /// \todo : Remove this: used from setupBlock which could also use enter_scope
    std::shared_ptr<SymbolTable> ModelSymbolTable::initialize_scope(std::string scope,
                                                                    AST* node,
                                                                    bool global) {
        if (global) {
            scope = "_NMODL_GLOBAL_";
        }
        return enter_scope(scope, node, global);
    }

    /** Callback at the entry of every block in nmodl file
     *  Every block starts a new scope and hence new symbol table is created.
     *  The same symbol table is returned so that visitor can store pointer to
     *  symbol table within a node.
     */
    std::shared_ptr<SymbolTable> ModelSymbolTable::enter_scope(std::string name,
                                                               AST* node,
                                                               bool global) {
        /// all global blocks in mod file have same symbol table
        if (symtab && name == "_NMODL_GLOBAL_") {
            parent_symtab = symtab;
            return parent_symtab;
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
        if (parent_symtab) {
            parent_symtab = parent_symtab->get_parent_table();
        }

        if (parent_symtab == nullptr) {
            parent_symtab = symtab;
        }
    }

    //=============================================================================
    // Revisit implementation of all below symbol table printing functions
    //=============================================================================

    enum class alignment { left, right, center };

    /// Left/Right/Center-aligns string within a field of width "width"
    std::string format_text(std::string text, int width, alignment type) {
        std::string left, right;
        // count excess room to pad
        auto padding = width - text.size();
        if (type == alignment::left) {
            right = std::string(padding, ' ');
        } else if (type == alignment::right) {
            left = std::string(padding, ' ');
        } else {
            left = std::string(padding / 2, ' ');
            right = std::string(padding / 2, ' ');
            // if odd #, add one more space
            if (padding > 0 && padding % 2 != 0) {
                right += " ";
            }
        }
        return left + text + right;
    }

    void Table::print(std::stringstream& ss, std::string title, int n) {
        if (symbols.size()) {
            std::vector<int> width{15, 15, 15, 15, 15};
            std::vector<std::vector<std::string>> rows;

            for (const auto& syminfo : symbols) {
                auto symbol = syminfo.second;

                // do not print external symbols which are not used in the current model
                if (symbol->is_extern_token_only() && symbol->get_read_count() == 0 &&
                    symbol->get_write_count() == 0) {
                    continue;
                }

                auto name = syminfo.first;
                auto position = symbol->get_token().position();
                auto properties = to_string(symbol->get_properties());
                auto reads = std::to_string(symbol->get_read_count());
                auto writes = std::to_string(symbol->get_write_count());

                std::vector<std::string> row{name, properties, position, reads, writes};
                rows.push_back(row);

                for (int i = 0; i < row.size(); i++) {
                    if (width[i] < (row[i].length() + 3))
                        width[i] = row[i].length() + 3;
                }
            }

            std::stringstream header;
            header << "| ";
            header << format_text("NAME", width[0], alignment::center) << " | ";
            header << format_text("PROPERTIES", width[1], alignment::center) << " | ";
            header << format_text("LOCATION", width[2], alignment::center) << " | ";
            header << format_text("# READS", width[3], alignment::center) << " | ";
            header << format_text("# WRITES", width[4], alignment::center) << "  |";

            auto header_len = header.str().length();
            auto spaces = std::string(n * 4, ' ');
            auto separator_line = std::string(header_len, '-');

            ss << "\n" << spaces << separator_line;
            ss << "\n" << spaces;
            ss << "|" << format_text(title, header_len - 2, alignment::center) << "|";
            ss << "\n" << spaces << separator_line;
            ss << "\n" << spaces << header.str();
            ss << "\n" << spaces << separator_line;

            for (const auto& row : rows) {
                ss << "\n" << spaces << "| ";
                ss << format_text(row[0], width[0], alignment::left) << " | ";
                ss << format_text(row[1], width[1], alignment::left) << " | ";
                ss << format_text(row[2], width[2], alignment::right) << " | ";
                ss << format_text(row[3], width[3], alignment::right) << " | ";
                ss << format_text(row[4], width[4], alignment::right) << "  |";
            }
            ss << "\n" << spaces << separator_line << "\n";
        }
    }

    void SymbolTable::print(std::stringstream& ss, int level) {
        auto title = symtab_name + " [" + type() + " IN " + get_parent_table_name() + "] ";
        title += "POSITION : " + position();
        title += global ? " SCOPE : GLOBAL" : " SCOPE : LOCAL";

        table.print(ss, title, level);

        auto next_level = level;
        if (table.symbols.size() == 0) {
            next_level--;
        }

        for (const auto& item : childrens) {
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