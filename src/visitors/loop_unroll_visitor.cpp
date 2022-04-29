/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitors/loop_unroll_visitor.hpp"

#include "ast/all.hpp"
#include "parser/c11_driver.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace visitor {

/**
 * \class IndexRemover
 * \brief Helper visitor to replace index of array variable with integer
 *
 * When loop is unrolled, the index variable like `i` :
 *
 *   ca[i] <-> ca[i+1]
 *
 * has type `Name` in the AST. This needs to be replaced with `Integer`
 * for optimizations like constant folding. This pass look at name and
 * binary expressions under index variables.
 */
class IndexRemover: public AstVisitor {
  private:
    /// index variable name
    std::string index;

    /// integer value of index variable
    int value;

    /// true if we are visiting index variable
    bool under_indexed_name = false;

  public:
    IndexRemover(std::string index, int value)
        : index(std::move(index))
        , value(value) {}

    /// if expression we are visiting is `Name` then return new `Integer` node
    std::shared_ptr<ast::Expression> replace_for_name(
        const std::shared_ptr<ast::Expression>& node) const {
        if (node->is_name()) {
            auto name = std::dynamic_pointer_cast<ast::Name>(node);
            if (name->get_node_name() == index) {
                return std::make_shared<ast::Integer>(value, nullptr);
            }
        }
        return node;
    }

    void visit_binary_expression(ast::BinaryExpression& node) override {
        node.visit_children(*this);
        if (under_indexed_name) {
            /// first recursively replaces children
            /// replace lhs & rhs if they have matching index variable
            const auto& lhs = replace_for_name(node.get_lhs());
            const auto& rhs = replace_for_name(node.get_rhs());
            node.set_lhs(std::move(lhs));
            node.set_rhs(std::move(rhs));
        }
    }

    virtual void visit_indexed_name(ast::IndexedName& node) override {
        under_indexed_name = true;
        node.visit_children(*this);
        /// once all children are replaced, do the same for index
        const auto& length = replace_for_name(node.get_length());
        node.set_length(std::move(length));
        under_indexed_name = false;
    }
};


/// return underlying expression wrapped by WrappedExpression
static std::shared_ptr<ast::Expression> unwrap(const std::shared_ptr<ast::Expression>& expr) {
    if (expr && expr->is_wrapped_expression()) {
        const auto& e = std::dynamic_pointer_cast<ast::WrappedExpression>(expr);
        return e->get_expression();
    }
    return expr;
}


/**
 * Unroll given for loop
 *
 * \param node From loop node in the AST
 * \return expression statement representing unrolled loop if successful otherwise \a nullptr
 */
static std::shared_ptr<ast::ExpressionStatement> unroll_for_loop(
    const std::shared_ptr<ast::FromStatement>& node) {
    /// loop can be in the form of `FROM i=(0) TO (10)`
    /// so first unwrap all elements of the loop
    const auto& from = unwrap(node->get_from());
    const auto& to = unwrap(node->get_to());
    const auto& increment = unwrap(node->get_increment());

    /// we can unroll if iteration space of the loop is known
    /// after constant folding start, end and increment must be known
    if (!from->is_integer() || !to->is_integer() ||
        (increment != nullptr && !increment->is_integer())) {
        return nullptr;
    }

    int start = std::dynamic_pointer_cast<ast::Integer>(from)->eval();
    int end = std::dynamic_pointer_cast<ast::Integer>(to)->eval();
    int step = 1;
    if (increment != nullptr) {
        step = std::dynamic_pointer_cast<ast::Integer>(increment)->eval();
    }

    ast::StatementVector statements;
    std::string index_var = node->get_node_name();
    for (int i = start; i <= end; i += step) {
        /// duplicate loop body and copy all statements to new vector
        const auto new_block = std::unique_ptr<ast::StatementBlock>(
            node->get_statement_block()->clone());
        IndexRemover(index_var, i).visit_statement_block(*new_block);
        statements.insert(statements.end(),
                          new_block->get_statements().begin(),
                          new_block->get_statements().end());
    }

    /// create new statement representing unrolled loop
    auto block = new ast::StatementBlock(std::move(statements));
    return std::make_shared<ast::ExpressionStatement>(block);
}


/**
 * Parse verbatim blocks and rename variable if it is used.
 */
void LoopUnrollVisitor::visit_statement_block(ast::StatementBlock& node) {
    node.visit_children(*this);

    const auto& statements = node.get_statements();

    for (auto iter = statements.begin(); iter != statements.end(); ++iter) {
        if ((*iter)->is_from_statement()) {
            const auto& statement = std::dynamic_pointer_cast<ast::FromStatement>((*iter));

            /// check if any verbatim block exists
            const auto& verbatim_blocks = collect_nodes(*statement, {ast::AstNodeType::VERBATIM});
            if (!verbatim_blocks.empty()) {
                logger->debug("LoopUnrollVisitor : can not unroll because of verbatim block");
                continue;
            }

            /// unroll loop, replace current statement on successfull unroll
            const auto& new_statement = unroll_for_loop(statement);
            if (new_statement != nullptr) {
                node.reset_statement(iter, new_statement);

                const auto& before = to_nmodl(statement);
                const auto& after = to_nmodl(new_statement);
                logger->debug("LoopUnrollVisitor : \n {} \n unrolled to \n {}", before, after);
            }
        }
    }
}

}  // namespace visitor
}  // namespace nmodl
