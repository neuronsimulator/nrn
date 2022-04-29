/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::RenameVisitor
 */

#include <regex>
#include <string>
#include <unordered_map>

#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class RenameVisitor
 * \brief `Blindly` rename given variable to new name
 *
 * During inlining related passes we have to rename variables
 * to avoid name conflicts. This pass "blindly" rename any given
 * variable to new name. The error handling / legality checks are
 * supposed to be done by other higher level passes. For example,
 * local renaming pass should be done from inner-most block to top
 * level block;
 *
 * \todo Add log/warning messages.
 */
class RenameVisitor: public ConstAstVisitor {
  private:
    /// ast::Ast* node
    std::shared_ptr<ast::Program> ast;

    /// regex for searching which variables to replace
    std::regex var_name_regex;

    /// new name
    std::string new_var_name;

    /// variable prefix
    std::string new_var_name_prefix;

    /// Map that keeps the renamed variables to keep the same random suffix when a variable is
    /// renamed across the whole file
    std::unordered_map<std::string, std::string> renamed_variables;

    /// add prefix to variable name
    bool add_prefix = false;

    /// add random suffix
    bool add_random_suffix = false;

    /// rename verbatim blocks as well
    bool rename_verbatim = true;

  public:
    RenameVisitor() = default;

    RenameVisitor(std::string old_name, std::string new_name)
        : var_name_regex(std::move(old_name))
        , new_var_name(std::move(new_name)) {}

    RenameVisitor(std::shared_ptr<ast::Program> ast,
                  std::string old_name,
                  std::string new_var_name_or_prefix,
                  bool add_prefix,
                  bool add_random_suffix)
        : ast(std::move(ast))
        , var_name_regex(std::move(old_name))
        , add_prefix(std::move(add_prefix))
        , add_random_suffix(std::move(add_random_suffix)) {
        if (add_prefix) {
            new_var_name_prefix = std::move(new_var_name_or_prefix);
        } else {
            new_var_name = std::move(new_var_name_or_prefix);
        }
    }

    /// Check if variable is already renamed and use the same naming otherwise add the new_name
    /// to the renamed_variables map
    std::string new_name_generator(const std::string& old_name);

    void set(std::string old_name, std::string new_name) {
        var_name_regex = std::move(old_name);
        new_var_name = std::move(new_name);
    }

    void enable_verbatim(bool state) noexcept {
        rename_verbatim = state;
    }

    void visit_name(const ast::Name& node) override;
    void visit_prime_name(const ast::PrimeName& node) override;
    void visit_verbatim(const ast::Verbatim& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
