/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::AfterCVodeToCnexpVisitor
 */

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class AfterCVodeToCnexpVisitor
 * \brief Visitor to change usage of after_cvode solver to cnexp
 *
 *  `CVODE` is not supported in CoreNEURON. If MOD file has `after_cvode` solver then
 *   we can treat that as `cnexp`. In order to re-use existing passes, in this visitor we
 *  replace `after_cvode` with `cnexp`.
 */

class AfterCVodeToCnexpVisitor: public AstVisitor {
  public:
    /// \name Ctor & dtor
    /// \{

    /// Default constructor
    AfterCVodeToCnexpVisitor() = default;

    /// \}
    void visit_solve_block(ast::SolveBlock& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
