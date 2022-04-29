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

/**
 * \file
 * \brief \copybrief nmodl::visitor::SymtabVisitor
 */

#include <set>

#include "symtab/symbol_table.hpp"
#include "visitors/ast_visitor.hpp"
#include "visitors/json_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class SymtabVisitor
 * \brief Concrete visitor for constructing symbol table from AST
 */
class SymtabVisitor: public AstVisitor {
  private:
    symtab::ModelSymbolTable* modsymtab = nullptr;

    std::unique_ptr<printer::JSONPrinter> printer;
    std::set<std::string> block_to_solve;
    bool update = false;
    bool under_state_block = false;

  public:
    explicit SymtabVisitor(bool update = false)
        : printer(new printer::JSONPrinter())
        , update(update) {}

    SymtabVisitor(symtab::ModelSymbolTable* _modsymtab, bool update = false)
        : modsymtab(_modsymtab)
        , update(update) {}

    SymtabVisitor(std::ostream& os, bool update = false)
        : printer(new printer::JSONPrinter(os))
        , update(update) {}

    SymtabVisitor(const std::string& filename, bool update = false)
        : printer(new printer::JSONPrinter(filename))
        , update(update) {}

    void add_model_symbol_with_property(ast::Node* node, symtab::syminfo::NmodlType property);

    void setup_symbol(ast::Node* node, symtab::syminfo::NmodlType property);

    void setup_symbol_table(ast::Ast* node, const std::string& name, bool is_global);

    void setup_symbol_table_for_program_block(ast::Program* node);

    void setup_symbol_table_for_global_block(ast::Node* node);

    void setup_symbol_table_for_scoped_block(ast::Node* node, const std::string& name);

    // clang-format off
    {% for node in nodes %}
        {% if node.is_symtab_method_required %}
    void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}& node) override;
        {% endif %}
    {% endfor %}
    // clang-format on
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl

