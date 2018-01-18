#ifndef _NMODL_SYMBOL_HPP_
#define _NMODL_SYMBOL_HPP_

#include <map>

#include "ast/ast.hpp"
#include "symtab/symbol_properties.hpp"

namespace symtab {

    using namespace ast;
    using namespace details;

    /**
     * \class Symbol
     * \brief Represent symbol in symbol table
     *
     * Symbol table generator pass visit the AST and insert symbol for
     * each node into symbol table. Symbol could appear multiple times
     * in a block or different global blocks. SymbolInfo object has all
     * nmodl properties information.
     *
     * \todo Multiple tokens (i.e. location information) for symbol should
     *       be tracked
     * \todo Scope information should be more than just string
     * \todo Perf block should track information about all usage of the symbol
     *       (would be helpful for perf modeling)
     */

    class Symbol {
      private:
        /// name of the symbol
        std::string name;

        /// ast node where symbol encountered first time
        /// node is passed from visitor and hence we have
        /// raw pointer instead of shared_ptr
        AST* node = nullptr;

        /// token for position information
        ModToken token;

        /// properties of symbol from whole mod file
        SymbolInfo properties{flags::empty};

        /// scope of the symbol (block name)
        std::string scope;

        /// number of times symbol is read
        int read_count = 0;

        /// number of times symbol is written
        int write_count = 0;

        /// order of derivative (for prime node)
        int order = 0;

      public:
        Symbol() = delete;

        Symbol(std::string name, ModToken token) : name(name), token(token) {
        }

        Symbol(std::string name, AST* node, ModToken token) : name(name), node(node), token(token) {
        }

        void set_scope(std::string s) {
            scope = s;
        }

        std::string get_name() {
            return name;
        }

        std::string get_scope() {
            return scope;
        }

        SymbolInfo get_properties() {
            return properties;
        }

        void read() {
            read_count++;
        }

        void write() {
            write_count++;
        }

        AST* get_node() {
            return node;
        }

        ModToken get_token() {
            return token;
        }

        int get_read_count() const {
            return read_count;
        }

        int get_write_count() const {
            return write_count;
        }

        bool is_external_symbol_only();

        bool has_properties(SymbolInfo new_properties);

        void combine_properties(SymbolInfo new_properties);

        void add_property(NmodlInfo property);

        void add_property(SymbolInfo property);

        void set_order(int new_order);

        bool is_variable();
    };

}  // namespace symtab

#endif