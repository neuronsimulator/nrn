/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::IndexedNameVisitor
 */

#include <string>
#include <unordered_set>

#include "ast/diff_eq_expression.hpp"
#include "ast/indexed_name.hpp"
#include "ast/program.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class IndexedNameVisitor
 * \brief Get node name with indexed for the IndexedName node and
 * the dependencies of DiffEqExpression node
 */
class IndexedNameVisitor: public AstVisitor {
  private:
    std::string indexed_name;
    std::pair<std::string, std::unordered_set<std::string>> dependencies;

  public:
    /// \name Ctor & dtor
    /// \{

    /// Default constructor
    IndexedNameVisitor() = default;

    /// \}

    /// Get node name with index for the IndexedName node
    void visit_indexed_name(ast::IndexedName& node) override;

    /// Get dependencies for the DiffEqExpression node
    void visit_diff_eq_expression(ast::DiffEqExpression& node) override;

    void visit_program(ast::Program& node) override;

    /// get the attribute indexed_name
    std::string get_indexed_name();

    /// get the attribute dependencies
    std::pair<std::string, std::unordered_set<std::string>> get_dependencies();
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
