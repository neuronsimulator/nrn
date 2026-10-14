/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::StateDiscontinuityVisitor
 */

#include "symtab/decl.hpp"
#include "visitors/ast_visitor.hpp"
#include <string>
#include <unordered_set>

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class StateDiscontinuityVisitor
 * \brief Visitor used for replacing literal calls to `state_discontinuity` in a NET_RECEIVE block
 */
class StateDiscontinuityVisitor: public AstVisitor {
  private:
    /// true if we are visiting a NET_RECEIVE block
    bool in_net_receive_block = false;

  public:
    void visit_net_receive_block(ast::NetReceiveBlock& node) override;
    void visit_statement_block(ast::StatementBlock& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
