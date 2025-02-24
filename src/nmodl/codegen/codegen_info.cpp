/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "codegen/codegen_info.hpp"

#include "ast/all.hpp"
#include "ast/longitudinal_diffusion_block.hpp"
#include "visitors/rename_visitor.hpp"
#include "visitors/var_usage_visitor.hpp"
#include "visitors/visitor_utils.hpp"


namespace nmodl {
namespace codegen {

using visitor::VarUsageVisitor;

LongitudinalDiffusionInfo::LongitudinalDiffusionInfo(
    const std::shared_ptr<ast::Name>& volume_index_name,
    std::shared_ptr<ast::Expression> volume_expr,
    const std::shared_ptr<ast::Name>& rate_index_name,
    std::shared_ptr<ast::Expression> rate_expr)
    : volume_index_name(volume_index_name ? volume_index_name->get_node_name() : std::string{})
    , volume_expr(std::move(volume_expr))
    , rate_index_name(rate_index_name ? rate_index_name->get_node_name() : std::string{})
    , rate_expr(std::move(rate_expr)) {}

std::shared_ptr<ast::Expression> LongitudinalDiffusionInfo::volume(
    const std::string& index_name) const {
    return substitute_index(index_name, volume_index_name, volume_expr);
}
std::shared_ptr<ast::Expression> LongitudinalDiffusionInfo::diffusion_rate(
    const std::string& index_name) const {
    return substitute_index(index_name, rate_index_name, rate_expr);
}

double LongitudinalDiffusionInfo::dfcdc(const std::string& /* index_name */) const {
    // Needed as part of the Jacobian to stabalize
    // the implicit time-integration. However,
    // currently, it's set to `0.0` for simplicity.
    return 0.0;
}

std::shared_ptr<ast::Expression> LongitudinalDiffusionInfo::substitute_index(
    const std::string& index_name,
    const std::string& old_index_name,
    const std::shared_ptr<ast::Expression>& old_expr) const {
    if (old_index_name == "") {
        // The expression doesn't contain an index that needs substituting.
        return old_expr;
    }
    auto new_expr = old_expr->clone();

    auto v = visitor::RenameVisitor(old_index_name, index_name);
    new_expr->accept(v);

    return std::shared_ptr<ast::Expression>(dynamic_cast<ast::Expression*>(new_expr));
}

/// if any ion has write variable
bool CodegenInfo::ion_has_write_variable() const noexcept {
    return std::any_of(ions.begin(), ions.end(), [](auto const& ion) {
        return !ion.writes.empty();
    });
}


/// if given variable is ion write variable
bool CodegenInfo::is_ion_write_variable(const std::string& name) const noexcept {
    return std::any_of(ions.begin(), ions.end(), [&name](auto const& ion) {
        return std::any_of(ion.writes.begin(), ion.writes.end(), [&name](auto const& var) {
            return var == name;
        });
    });
}


/// if given variable is ion read variable
bool CodegenInfo::is_ion_read_variable(const std::string& name) const noexcept {
    return std::any_of(ions.begin(), ions.end(), [&name](auto const& ion) {
        return std::any_of(ion.reads.begin(), ion.reads.end(), [&name](auto const& var) {
            return var == name;
        });
    });
}


/// if either read or write variable
bool CodegenInfo::is_ion_variable(const std::string& name) const noexcept {
    return is_ion_read_variable(name) || is_ion_write_variable(name);
}


/// if a current (ionic or non-specific)
bool CodegenInfo::is_current(const std::string& name) const noexcept {
    return std::any_of(currents.begin(), currents.end(), [&name](auto const& var) {
        return var == name;
    });
}

/// true is a given variable name if a ionic current
/// (i.e. currents excluding non-specific current)
bool CodegenInfo::is_ionic_current(const std::string& name) const noexcept {
    return std::any_of(ions.begin(), ions.end(), [&name](auto const& ion) {
        return ion.is_ionic_current(name);
    });
}

/// true if given variable name is a ionic concentration
bool CodegenInfo::is_ionic_conc(const std::string& name) const noexcept {
    return std::any_of(ions.begin(), ions.end(), [&name](auto const& ion) {
        return ion.is_ionic_conc(name);
    });
}

bool CodegenInfo::function_uses_table(const std::string& name) const noexcept {
    return std::any_of(functions_with_table.begin(),
                       functions_with_table.end(),
                       [&name](auto const& function) { return name == function->get_node_name(); });
}

/**
 * Check if NrnState node in the AST has EigenSolverBlock node
 *
 * @return True if EigenSolverBlock exist in the node
 */
bool CodegenInfo::nrn_state_has_eigen_solver_block() const {
    if (nrn_state_block == nullptr) {
        return false;
    }
    return !collect_nodes(*nrn_state_block, {ast::AstNodeType::EIGEN_NEWTON_SOLVER_BLOCK}).empty();
}

/**
 * Check if WatchStatement uses voltage variable v
 *
 * Watch statement has condition expression which could use voltage
 * variable `v`. To avoid memory access into voltage array we check
 * if `v` is used and then print necessary code.
 *
 * @return true if voltage variable b is used otherwise false
 */
bool CodegenInfo::is_voltage_used_by_watch_statements() const {
    return std::any_of(watch_statements.begin(), watch_statements.end(), [](auto const& statement) {
        return VarUsageVisitor{}.variable_used(*statement, "v");
    });
}

}  // namespace codegen
}  // namespace nmodl
