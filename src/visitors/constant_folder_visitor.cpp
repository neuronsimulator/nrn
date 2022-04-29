/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/constant_folder_visitor.hpp"

#include "ast/all.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace visitor {

/// check if given expression is a number
/// note that the DEFINE node is already expanded to integer
static inline bool is_number(const std::shared_ptr<ast::Expression>& node) {
    return node->is_integer() || node->is_double() || node->is_float();
}

/// get value of a number node
/// TODO : eval method can be added to virtual base class
static double get_value(const std::shared_ptr<ast::Expression>& node) {
    if (node->is_integer()) {
        return std::dynamic_pointer_cast<ast::Integer>(node)->eval();
    } else if (node->is_float()) {
        return std::dynamic_pointer_cast<ast::Float>(node)->to_double();
    } else if (node->is_double()) {
        return std::dynamic_pointer_cast<ast::Double>(node)->to_double();
    }
    throw std::runtime_error("Invalid type passed to is_number()");
}

/// operators that currently implemented
static inline bool supported_operator(ast::BinaryOp op) {
    return op == ast::BOP_ADDITION || op == ast::BOP_SUBTRACTION || op == ast::BOP_MULTIPLICATION ||
           op == ast::BOP_DIVISION;
}

/// Evaluate binary operation
/// TODO : add support for other binary operators like ^ (pow)
static double compute(double lhs, ast::BinaryOp op, double rhs) {
    switch (op) {
    case ast::BOP_ADDITION:
        return lhs + rhs;

    case ast::BOP_SUBTRACTION:
        return lhs - rhs;

    case ast::BOP_MULTIPLICATION:
        return lhs * rhs;

    case ast::BOP_DIVISION:
        return lhs / rhs;

    default:
        throw std::logic_error("Invalid binary operator in constant folding");
    }
}

/**
 * Visit parenthesis expression and simplify it
 * @param node AST node representing an expression with parenthesis
 *
 * AST could have expression like (1+2). In this case, it has following
 * form in the AST :
 *
 *  parenthesis_exp => wrapped_expr => binary_expression => ...
 *
 * To make constant folding simple, we can remove intermediate wrapped_expr
 * and directly replace binary_expression inside parenthesis_exp :
 *
 *  parenthesis_exp => binary_expression => ...
 */
void ConstantFolderVisitor::visit_paren_expression(ast::ParenExpression& node) {
    node.visit_children(*this);
    auto expr = node.get_expression();
    if (expr->is_wrapped_expression()) {
        auto e = std::dynamic_pointer_cast<ast::WrappedExpression>(expr);
        node.set_expression(e->get_expression());
    }
}

/**
 * Visit wrapped node type and perform constant folding
 * @param node AST node that wrap other node types
 *
 * MOD file has expressions like
 *
 * a = 1 + 2
 * DEFINE NN 10
 * FROM i=0 TO NN-2 {
 *
 * }
 *
 * which need to be turned into
 *
 * a = 1 + 2
 * DEFINE NN 10
 * FROM i=0 TO 8 {
 *
 * }
 */
void ConstantFolderVisitor::visit_wrapped_expression(ast::WrappedExpression& node) {
    node.visit_children(*this);
    node.visit_children(*this);

    /// first expression which is wrapped
    auto expr = node.get_expression();

    /// if wrapped expression is parentheses
    bool is_parentheses = false;

    /// opposite to visit_paren_expression, we might have
    /// a = (2+1)
    /// in this case we can pick inner expression.
    if (expr->is_paren_expression()) {
        auto e = std::dynamic_pointer_cast<ast::ParenExpression>(expr);
        expr = e->get_expression();
        is_parentheses = true;
    }

    /// we want to simplify binary expressions only
    if (!expr->is_binary_expression()) {
        /// wrapped expression might be parenthesis expression like (2)
        /// which we can simplify to 2 to help next evaluations
        if (is_parentheses) {
            node.set_expression(std::move(expr));
        }
        return;
    }

    auto binary_expr = std::dynamic_pointer_cast<ast::BinaryExpression>(expr);
    auto lhs = binary_expr->get_lhs();
    auto rhs = binary_expr->get_rhs();
    auto op = binary_expr->get_op().get_value();

    /// in case of expression like
    /// a = 2 + ((1) + (3))
    /// we are in the innermost expression i.e. ((1) + (3))
    /// where (1) and (3) are wrapped expression themself. we can
    /// remove these extra wrapped expressions

    if (lhs->is_wrapped_expression()) {
        auto e = std::dynamic_pointer_cast<ast::WrappedExpression>(lhs);
        lhs = e->get_expression();
    }

    if (rhs->is_wrapped_expression()) {
        auto e = std::dynamic_pointer_cast<ast::WrappedExpression>(rhs);
        rhs = e->get_expression();
    }

    /// once we simplify, lhs and rhs must be numbers for constant folding
    if (!is_number(lhs) || !is_number(rhs) || !supported_operator(op)) {
        return;
    }

    const std::string& nmodl_before = to_nmodl(binary_expr);

    /// compute the value of expression
    auto value = compute(get_value(lhs), op, get_value(rhs));

    /// if both operands are not integers or floats, result is double
    if (lhs->is_integer() && rhs->is_integer()) {
        node.set_expression(std::make_shared<ast::Integer>(static_cast<int>(value), nullptr));
    } else if (lhs->is_double() || rhs->is_double()) {
        node.set_expression(std::make_shared<ast::Double>(stringutils::to_string(value)));
    } else {
        node.set_expression(std::make_shared<ast::Float>(stringutils::to_string(value)));
    }

    const std::string& nmodl_after = to_nmodl(node.get_expression());
    logger->debug("ConstantFolderVisitor : expression {} folded to {}", nmodl_before, nmodl_after);
}

}  // namespace visitor
}  // namespace nmodl
