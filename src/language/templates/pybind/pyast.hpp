/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#pragma once

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include "ast/all.hpp"
#include "lexer/modtoken.hpp"
#include "symtab/symbol_table.hpp"


using namespace nmodl;
using namespace ast;

/**
 * \file
 * \brief Base AST class for Python bindings
 */

/**
 * \defgroup nmodl_python Python Interface
 * \brief Python Bindings Implementation
 */

/**
 *
 * \defgroup ast_python AST Python Interface
 * \ingroup nmodl_python
 * \brief Ast classes for Python bindings
 * \{
 */

/**
 * \brief Class mirroring nmodl::ast::Ast for Python bindings
 *
 * \details \copydetails nmodl::ast::Ast
 *
 * This class is used to interface nmodl::Ast with the Python
 * world using `pybind11`.
 */
struct PyAst: public Ast {

    void visit_children(visitor::Visitor& v) override {
        PYBIND11_OVERLOAD_PURE(void,            /// Return type
                               Ast,             /// Parent class
                               visit_children,  /// Name of function in C++ (must match Python name)
                               v                /// Argument(s)
        );
    }

    void visit_children(visitor::ConstVisitor& v) const override {
        PYBIND11_OVERLOAD_PURE(void,            /// Return type
                               Ast,             /// Parent class
                               visit_children,  /// Name of function in C++ (must match Python name)
                               v                /// Argument(s)
        );
    }

    void accept(visitor::Visitor& v) override {
        PYBIND11_OVERLOAD_PURE(void, Ast, accept, v);
    }

    void accept(visitor::ConstVisitor& v) const override {
        PYBIND11_OVERLOAD_PURE(void, Ast, accept, v);
    }

    Ast* clone() const override {
        PYBIND11_OVERLOAD(Ast*, Ast, clone, );
    }

    AstNodeType get_node_type() const override {
        PYBIND11_OVERLOAD_PURE(AstNodeType,    // Return type
                               Ast,            // Parent class
                               get_node_type,  // Name of function in C++ (must match Python name)
                                               // No argument (trailing ,)
        );
    }

    std::string get_node_type_name() const override {
        PYBIND11_OVERLOAD_PURE(std::string, Ast, get_node_type_name, );
    }

    std::string get_node_name() const override {
        PYBIND11_OVERLOAD(std::string, Ast, get_node_name, );
    }

    std::string get_nmodl_name() const override {
        PYBIND11_OVERLOAD(std::string, Ast, get_nmodl_name, );
    }

    std::shared_ptr<Ast> get_shared_ptr() override {
        PYBIND11_OVERLOAD(std::shared_ptr<Ast>, Ast, get_shared_ptr, );
    }

    std::shared_ptr<const Ast> get_shared_ptr() const override {
        PYBIND11_OVERLOAD(std::shared_ptr<const Ast>, Ast, get_shared_ptr, );
    }

    const ModToken* get_token() const override {
        PYBIND11_OVERLOAD(const ModToken*, Ast, get_token, );
    }

    symtab::SymbolTable* get_symbol_table() const override {
        PYBIND11_OVERLOAD(symtab::SymbolTable*, Ast, get_symbol_table, );
    }

    const std::shared_ptr<StatementBlock>& get_statement_block() const override {
        PYBIND11_OVERLOAD(const std::shared_ptr<StatementBlock>&, Ast, get_statement_block, );
    }

    void set_symbol_table(symtab::SymbolTable* newsymtab) override {
        PYBIND11_OVERLOAD(void, Ast, set_symbol_table, newsymtab);
    }

    void set_name(const std::string& name) override {
        PYBIND11_OVERLOAD(void, Ast, set_name, name);
    }

    void negate() override {
        PYBIND11_OVERLOAD(void, Ast, negate, );
    }

    bool is_ast() const noexcept override {
        PYBIND11_OVERLOAD(bool, Ast, is_ast, );
    }

    {% for node in nodes %}

    bool is_{{node.class_name | snake_case}}() const noexcept override {
        PYBIND11_OVERLOAD(bool, Ast, is_{{node.class_name | snake_case}}, );
    }

    {% endfor %}

    Ast* get_parent() const override {
        PYBIND11_OVERLOAD(Ast*, Ast, get_parent, );
    }

    void set_parent(Ast* p) override {
        PYBIND11_OVERLOAD(void, Ast, set_parent, p);
    }
};

/** \} */  // end of ast_python
