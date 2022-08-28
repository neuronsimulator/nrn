/*************************************************************************
 * Copyright (C) 2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::ImplicitArgumentVisitor
 */

#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class ImplicitArgumentVisitor
 * \brief %Visitor for adding implicit arguments to [Core]NEURON functions.
 *
 * This visitor is used to add implicit arguments to functions (nrn_ghk, ...)
 * that have historially been used in MOD files with "fewer" arguments and
 * relied on global state, but which now need "more" arguments to be passed so
 * that they can be pure functions.
 */
struct ImplicitArgumentVisitor: public AstVisitor {
    void visit_function_call(ast::FunctionCall& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
