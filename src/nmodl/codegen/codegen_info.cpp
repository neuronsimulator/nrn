/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "codegen/codegen_info.hpp"

#include "ast/all.hpp"
#include "visitors/lookup_visitor.hpp"


namespace nmodl {
namespace codegen {

using visitor::AstLookupVisitor;

/// if any ion has write variable
bool CodegenInfo::ion_has_write_variable() const {
    for (const auto& ion: ions) {
        if (!ion.writes.empty()) {
            return true;
        }
    }
    return false;
}


/// if given variable is ion write variable
bool CodegenInfo::is_ion_write_variable(const std::string& name) const {
    for (const auto& ion: ions) {
        for (auto& var: ion.writes) {
            if (var == name) {
                return true;
            }
        }
    }
    return false;
}


/// if given variable is ion read variable
bool CodegenInfo::is_ion_read_variable(const std::string& name) const {
    for (const auto& ion: ions) {
        for (auto& var: ion.reads) {
            if (var == name) {
                return true;
            }
        }
    }
    return false;
}


/// if either read or write variable
bool CodegenInfo::is_ion_variable(const std::string& name) const {
    return is_ion_read_variable(name) || is_ion_write_variable(name);
}


/// if a current
bool CodegenInfo::is_current(const std::string& name) const {
    for (auto& var: currents) {
        if (var == name) {
            return true;
        }
    }
    return false;
}


bool CodegenInfo::function_uses_table(std::string& name) const {
    for (auto& function: functions_with_table) {
        if (name == function->get_node_name()) {
            return true;
        }
    }
    return false;
}

/**
 * Check if coreneuron internal derivimplicit solver needs to be used
 *
 * - if derivimplicit method is not used or solve block is empty
 *   then there is nothing to do
 * - if eigen solver block is used then coreneuron solver is not needed
 */

bool CodegenInfo::derivimplicit_coreneuron_solver() const {
    return !derivimplicit_callbacks.empty();
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
    return !AstLookupVisitor()
                .lookup(*nrn_state_block, ast::AstNodeType::EIGEN_NEWTON_SOLVER_BLOCK)
                .empty();
}

}  // namespace codegen
}  // namespace nmodl
