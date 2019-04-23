/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <set>

#include "visitors/json_visitor.hpp"
#include "visitors/ast_visitor.hpp"
#include "symtab/symbol_table.hpp"

namespace nmodl {

/* Concrete visitor for constructing symbol table from AST */
class SymtabVisitor : public AstVisitor {

private:
    symtab::ModelSymbolTable* modsymtab;

    std::unique_ptr<JSONPrinter> printer;
    std::set<std::string> block_to_solve;
    bool update = false;
    bool under_state_block = false;

public:
    explicit SymtabVisitor(bool update = false) : printer(new JSONPrinter()), update(update) {}
    SymtabVisitor(std::ostream &os, bool update = false) : printer(new JSONPrinter(os)), update(update) {}
    SymtabVisitor(std::string filename, bool update = false) : printer(new JSONPrinter(filename)), update(update) {}

    void add_model_symbol_with_property(ast::Node* node, symtab::syminfo::NmodlType property);

    void setup_symbol(ast::Node* node, symtab::syminfo::NmodlType property);

    void setup_symbol_table(ast::AST* node, const std::string& name, bool is_global);

    void setup_symbol_table_for_program_block(ast::Program* node);

    void setup_symbol_table_for_global_block(ast::Node* node);

    void setup_symbol_table_for_scoped_block(ast::Node* node, const std::string& name);

    {% for node in nodes %}
        {% if node.is_symtab_method_required %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
        {% endif %}
    {% endfor %}
};

}  // namespace nmodl
