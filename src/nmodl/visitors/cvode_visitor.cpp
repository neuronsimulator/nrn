/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visitors/cvode_visitor.hpp"

#include "ast/all.hpp"
#include "lexer/token_mapping.hpp"
#include "pybind/pyembed.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"
#include <optional>
#include <utility>

namespace pywrap = nmodl::pybind_wrappers;

namespace nmodl {
namespace visitor {

static int get_index(const ast::IndexedName& node) {
    return std::stoi(to_nmodl(node.get_length()));
}

static void remove_conserve_statements(ast::StatementBlock& node) {
    auto conserve_equations = collect_nodes(node, {ast::AstNodeType::CONSERVE});
    if (!conserve_equations.empty()) {
        std::unordered_set<ast::Statement*> eqs;
        for (const auto& item: conserve_equations) {
            eqs.insert(std::dynamic_pointer_cast<ast::Statement>(item).get());
        }
        node.erase_statement(eqs);
    }
}

static std::pair<std::string, std::optional<int>> parse_independent_var(
    std::shared_ptr<ast::Identifier> node) {
    auto variable = std::make_pair(node->get_node_name(), std::optional<int>());
    if (node->is_indexed_name()) {
        variable.second = std::optional<int>(
            get_index(*std::dynamic_pointer_cast<const ast::IndexedName>(node)));
    }
    return variable;
}

/// set of all indexed variables not equal to ``ignored_name``
static std::unordered_set<std::string> get_indexed_variables(const ast::Expression& node,
                                                             const std::string& ignored_name) {
    std::unordered_set<std::string> indexed_variables;
    // all of the "reserved" vars
    auto reserved_symbols = get_external_functions();
    // all indexed vars
    auto indexed_vars = collect_nodes(node, {ast::AstNodeType::INDEXED_NAME});
    for (const auto& var: indexed_vars) {
        const auto& varname = var->get_node_name();
        // skip if it's a reserved var
        auto varname_not_reserved =
            std::none_of(reserved_symbols.begin(),
                         reserved_symbols.end(),
                         [&varname](const auto item) { return varname == item; });
        if (indexed_variables.count(varname) == 0 && varname != ignored_name &&
            varname_not_reserved) {
            indexed_variables.insert(varname);
        }
    }
    return indexed_variables;
}

static std::string cvode_set_lhs(ast::BinaryExpression& node) {
    const auto& lhs = node.get_lhs();

    auto name = std::dynamic_pointer_cast<ast::VarName>(lhs)->get_name();

    std::string varname;
    if (name->is_prime_name()) {
        varname = "D" + name->get_node_name();
        node.set_lhs(std::make_shared<ast::Name>(new ast::String(varname)));
    } else if (name->is_indexed_name()) {
        auto nodes = collect_nodes(*name, {ast::AstNodeType::PRIME_NAME});
        // make sure the LHS isn't just a plain indexed var
        if (!nodes.empty()) {
            varname = "D" + stringutils::remove_character(to_nmodl(node.get_lhs()), '\'');
            auto statement = fmt::format("{} = {}", varname, varname);
            auto expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(
                create_statement(statement));
            const auto bin_expr = std::dynamic_pointer_cast<const ast::BinaryExpression>(
                expr_statement->get_expression());
            node.set_lhs(std::shared_ptr<ast::Expression>(bin_expr->get_lhs()->clone()));
        }
    }
    return varname;
}


class CvodeHelperVisitor: public AstVisitor {
  protected:
    symtab::SymbolTable* program_symtab = nullptr;
    bool in_differential_equation = false;
  public:
    inline void visit_diff_eq_expression(ast::DiffEqExpression& node) {
        in_differential_equation = true;
        node.visit_children(*this);
        in_differential_equation = false;
    }
};

class NonStiffVisitor: public CvodeHelperVisitor {
  public:
    explicit NonStiffVisitor(symtab::SymbolTable* symtab) {
        program_symtab = symtab;
    }

    inline void visit_binary_expression(ast::BinaryExpression& node) {
        const auto& lhs = node.get_lhs();

        if (!in_differential_equation || !lhs->is_var_name()) {
            return;
        }

        auto name = std::dynamic_pointer_cast<ast::VarName>(lhs)->get_name();
        auto varname = cvode_set_lhs(node);

        if (program_symtab->lookup(varname) == nullptr) {
            auto symbol = std::make_shared<symtab::Symbol>(varname, ModToken());
            symbol->set_original_name(name->get_node_name());
            program_symtab->insert(symbol);
        }
    }
};

class StiffVisitor: public CvodeHelperVisitor {
  public:
    explicit StiffVisitor(symtab::SymbolTable* symtab) {
        program_symtab = symtab;
    }

    inline void visit_binary_expression(ast::BinaryExpression& node) {
        const auto& lhs = node.get_lhs();

        if (!in_differential_equation || !lhs->is_var_name()) {
            return;
        }

        auto name = std::dynamic_pointer_cast<ast::VarName>(lhs)->get_name();
        auto varname = cvode_set_lhs(node);

        if (program_symtab->lookup(varname) == nullptr) {
            auto symbol = std::make_shared<symtab::Symbol>(varname, ModToken());
            symbol->set_original_name(name->get_node_name());
            program_symtab->insert(symbol);
        }

        auto rhs = node.get_rhs();
        // all indexed variables (need special treatment in SymPy)
        auto indexed_variables = get_indexed_variables(*rhs, name->get_node_name());
        auto diff2c = pywrap::EmbeddedPythonLoader::get_instance().api().diff2c;
        auto [jacobian, exception_message] =
            diff2c(to_nmodl(*rhs), parse_independent_var(name), indexed_variables);
        if (!exception_message.empty()) {
            logger->warn("CvodeVisitor :: python exception: {}", exception_message);
        }
        // NOTE: LHS can be anything here, the equality is to keep `create_statement` from
        // complaining, we discard the LHS later
        auto statement = fmt::format("{} = {} / (1 - dt * ({}))", varname, varname, jacobian);
        logger->debug("CvodeVisitor :: replacing statement {} with {}", to_nmodl(node), statement);
        auto expr_statement = std::dynamic_pointer_cast<ast::ExpressionStatement>(
            create_statement(statement));
        const auto bin_expr = std::dynamic_pointer_cast<const ast::BinaryExpression>(
            expr_statement->get_expression());
        node.set_rhs(std::shared_ptr<ast::Expression>(bin_expr->get_rhs()->clone()));
    }
};

static std::shared_ptr<ast::DerivativeBlock> get_derivative_block(ast::Program& node) {
    auto derivative_blocks = collect_nodes(node, {ast::AstNodeType::DERIVATIVE_BLOCK});
    if (derivative_blocks.empty()) {
        return nullptr;
    }

    // steady state adds a DERIVATIVE block with a `_steadystate` suffix
    auto not_steadystate = [](const auto& item) {
        auto name = std::dynamic_pointer_cast<const ast::DerivativeBlock>(item)->get_node_name();
        return !stringutils::ends_with(name, "_steadystate");
    };
    decltype(derivative_blocks) derivative_blocks_copy;
    std::copy_if(derivative_blocks.begin(),
                 derivative_blocks.end(),
                 std::back_inserter(derivative_blocks_copy),
                 not_steadystate);
    if (derivative_blocks_copy.size() > 1) {
        auto message = "CvodeVisitor :: cannot have multiple DERIVATIVE blocks";
        logger->error(message);
        throw std::runtime_error(message);
    }

    return std::dynamic_pointer_cast<ast::DerivativeBlock>(derivative_blocks_copy[0]);
}


void CvodeVisitor::visit_program(ast::Program& node) {
    auto derivative_block = get_derivative_block(node);
    if (derivative_block == nullptr) {
        return;
    }

    auto non_stiff_block = derivative_block->get_statement_block()->clone();
    remove_conserve_statements(*non_stiff_block);

    auto stiff_block = derivative_block->get_statement_block()->clone();
    remove_conserve_statements(*stiff_block);

    NonStiffVisitor(node.get_symbol_table()).visit_statement_block(*non_stiff_block);
    StiffVisitor(node.get_symbol_table()).visit_statement_block(*stiff_block);
    auto prime_vars = collect_nodes(*derivative_block, {ast::AstNodeType::PRIME_NAME});
    node.emplace_back_node(new ast::CvodeBlock(
        derivative_block->get_name(),
        std::shared_ptr<ast::Integer>(new ast::Integer(prime_vars.size(), nullptr)),
        std::shared_ptr<ast::StatementBlock>(non_stiff_block),
        std::shared_ptr<ast::StatementBlock>(stiff_block)));
}

}  // namespace visitor
}  // namespace nmodl
