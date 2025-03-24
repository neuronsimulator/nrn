/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::ImplicitArgumentVisitor
 */

#include "visitors/ast_visitor.hpp"

#include <string>


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
    explicit ImplicitArgumentVisitor(const std::string& simulator = "coreneuron")
        : simulator(simulator) {}

    void visit_function_call(ast::FunctionCall& node) override;

  private:
    std::string simulator;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
