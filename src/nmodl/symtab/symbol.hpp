/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <map>
#include <memory>

#include "lexer/modtoken.hpp"
#include "symtab/symbol_properties.hpp"


namespace nmodl {

namespace ast {
class AST;
}

namespace symtab {

/**
 * \class Symbol
 * \brief Represent symbol in symbol table
 *
 * Symbol table generator pass visit the AST and insert symbol for
 * each node into symbol table. Symbol could appear multiple times
 * in a block or different global blocks. NmodlType object has all
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

    /// original name in case of renaming
    std::string renamed_from;

    /// unique id or index position when symbol is inserted into specific table
    int id = 0;

    /// ast node where symbol encountered first time
    /// node is passed from visitor and hence we have
    /// raw pointer instead of shared_ptr
    ast::AST* node = nullptr;

    /// token for position information
    ModToken token;

    /// properties of symbol from whole mod file
    syminfo::NmodlType properties{syminfo::NmodlType::empty};

    /// status of symbol during various passes
    syminfo::Status status{syminfo::Status::empty};

    /// scope of the symbol (block name)
    std::string scope;

    /// number of times symbol is read
    int read_count = 0;

    /// number of times symbol is written
    int write_count = 0;

    /// order of derivative (for prime node)
    int order = 0;

    /// order in which symbol arrives
    int definition_order = -1;

    /// value (for parameters, constant etc)
    std::shared_ptr<double> value;

    /// true if array variable
    bool array = false;

    /// number of elements
    int length = 1;

    // number of values in case of table variable
    int num_values = 0;

  public:
    Symbol() = delete;

    Symbol(std::string name, ast::AST* node)
        : name(name)
        , node(node) {}

    Symbol(std::string name, ModToken token)
        : name(name)
        , token(token) {}

    Symbol(std::string name, ast::AST* node, ModToken token)
        : name(name)
        , node(node)
        , token(token) {}

    void set_scope(std::string s) {
        scope = s;
    }

    std::string get_name() {
        return name;
    }

    int get_id() {
        return id;
    }

    void set_id(int i) {
        id = i;
    }

    void set_name(std::string new_name) {
        if (renamed_from.empty()) {
            renamed_from = name;
        }
        name = new_name;
    }

    std::string get_scope() {
        return scope;
    }

    syminfo::NmodlType get_properties() {
        return properties;
    }

    syminfo::Status get_status() {
        return status;
    }

    void read() {
        read_count++;
    }

    void write() {
        write_count++;
    }

    ast::AST* get_node() {
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

    int get_definition_order() const {
        return definition_order;
    }

    bool is_external_symbol_only();

    /// \todo : rename to has_any_property
    bool has_properties(syminfo::NmodlType new_properties);

    bool has_all_properties(syminfo::NmodlType new_properties);

    bool has_any_status(syminfo::Status new_status);

    bool has_all_status(syminfo::Status new_status);

    void add_properties(syminfo::NmodlType new_properties);

    void add_property(syminfo::NmodlType property);

    void inlined() {
        status |= syminfo::Status::inlined;
    }

    void mark_created() {
        status |= syminfo::Status::created;
    }

    void mark_renamed() {
        status |= syminfo::Status::renamed;
    }

    void mark_localized() {
        status |= syminfo::Status::localized;
    }

    void mark_thread_safe() {
        status |= syminfo::Status::thread_safe;
    }

    void created_from_state() {
        mark_created();
        status |= syminfo::Status::from_state;
    }

    void set_order(int new_order);

    void set_definition_order(int order) {
        definition_order = order;
    }

    void set_value(double val) {
        value = std::make_shared<double>(val);
    }

    void set_as_array(int len) {
        array = true;
        length = len;
    }

    bool is_array() {
        return array;
    }

    void set_length(int len) {
        length = len;
    }

    int get_length() {
        return length;
    }

    int get_num_values() {
        return num_values;
    }

    void set_num_values(int n) {
        num_values = n;
    }

    std::string get_original_name() {
        return renamed_from;
    }

    std::shared_ptr<double> get_value() {
        return value;
    }

    void set_original_name(std::string new_name) {
        renamed_from = new_name;
    }

    std::string to_string();

    bool is_variable();
};

}  // namespace symtab
}  // namespace nmodl
