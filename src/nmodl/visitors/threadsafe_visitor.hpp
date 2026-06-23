/*
 * Copyright 2025 EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::ThreadsafeVisitor
 */

#include <string>
#include <unordered_set>
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class ThreadsafeVisitor
 * \brief Visitor used for making a mod file threadsafe
 */
class ThreadsafeVisitor: public AstVisitor {
  private:
    bool in_net_receive_block = false;
    bool in_initial_block = false;
    bool in_parameter_block = false;
    bool in_assigned_block = false;
    bool can_be_made_threadsafe = true;
    std::unordered_set<std::string> global_vars;
    bool convert_globals = false;
    bool convert_verbatim = false;

  public:
    void visit_program(ast::Program& node) override;
    void visit_assigned_block(ast::AssignedBlock&) override;
    void visit_function_call(ast::FunctionCall&) override;
    void visit_initial_block(ast::InitialBlock&) override;
    void visit_net_receive_block(ast::NetReceiveBlock&) override;
    void visit_param_block(ast::ParamBlock&) override;
    void visit_binary_expression(ast::BinaryExpression&) override;
    explicit ThreadsafeVisitor(bool convert_globals = false, bool convert_verbatim = false)
        : convert_globals(convert_globals)
        , convert_verbatim(convert_verbatim){};
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
