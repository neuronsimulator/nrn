/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once


#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "ast/ast.hpp"
#include "lexer/modtoken.hpp"
#include "symtab/symbol_table.hpp"


using namespace nmodl;
using namespace ast;

struct PyAST: public AST {
  public:
    using AST::AST;

    void visit_children(Visitor* v) override {
        PYBIND11_OVERLOAD_PURE(void,            // Return type
                               AST,             // Parent class
                               visit_children,  // Name of function in C++ (must match Python name)
                               v                // Argument(s)
        );
    }

    void accept(Visitor* v) override { PYBIND11_OVERLOAD_PURE(void, AST, accept, v); }

    AstNodeType get_node_type() override {
        PYBIND11_OVERLOAD_PURE(AstNodeType,    // Return type
                               AST,            // Parent class
                               get_node_type,  // Name of function in C++ (must match Python name)
                                               // No argument (trailing ,)
        );
    }

    std::string get_node_type_name() override {
        PYBIND11_OVERLOAD_PURE(std::string, AST, get_node_type_name, );
    }

    std::string get_node_name() override { PYBIND11_OVERLOAD(std::string, AST, get_node_name, ); }

    AST* clone() override { PYBIND11_OVERLOAD(AST*, AST, clone, ); }

    std::shared_ptr<AST> get_shared_ptr() override { PYBIND11_OVERLOAD(std::shared_ptr<AST>, AST, get_shared_ptr, ); }

    ModToken* get_token() override { PYBIND11_OVERLOAD(ModToken*, AST, get_token, ); }

    void set_symbol_table(symtab::SymbolTable* newsymtab) override {
        PYBIND11_OVERLOAD(void, AST, set_symbol_table, newsymtab);
    }

    symtab::SymbolTable* get_symbol_table() override {
        PYBIND11_OVERLOAD(symtab::SymbolTable*, AST, get_symbol_table, );
    }
    std::shared_ptr<StatementBlock> get_statement_block() override {
        PYBIND11_OVERLOAD(std::shared_ptr<StatementBlock>, AST, get_statement_block, );
    }

    void negate() override { PYBIND11_OVERLOAD(void, AST, negate, ); }

    void set_name(std::string name) override { PYBIND11_OVERLOAD(void, AST, set_name, name); }

    bool is_ast() override { PYBIND11_OVERLOAD(bool, AST, is_ast, ); }

    {% for node in nodes %}

    bool is_{{node.class_name | snake_case}}() override {
        PYBIND11_OVERLOAD(bool, AST, is_{{node.class_name | snake_case}}, );
    }

    {% endfor %}
};
