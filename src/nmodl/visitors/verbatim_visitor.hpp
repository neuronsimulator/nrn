/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::VerbatimVisitor
 */

#include <vector>

#include "ast/ast.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * @addtogroup visitor_classes
 * @{
 */

/**
 * \class VerbatimVisitor
 * \brief %Visitor for verbatim blocks of AST
 *
 * This is simple example of visitor that uses base AstVisitor
 * interface. We override visitVerbatim method and store all
 * verbatim blocks that we encounter. This could be used for
 * generating report of all verbatim blocks from all mod files
 * in ModelDB.
 */
class VerbatimVisitor: public ConstAstVisitor {
  private:
    /// flag to enable/disable printing blocks as we visit them
    bool verbose = false;

    /// vector containing all verbatim blocks
    std::vector<std::string> blocks;

  public:
    VerbatimVisitor() = default;

    explicit VerbatimVisitor(bool verbose) {
        this->verbose = verbose;
    }

    void visit_verbatim(const ast::Verbatim& node) override;

    const std::vector<std::string>& verbatim_blocks() const noexcept {
        return blocks;
    }
};

/** @} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
