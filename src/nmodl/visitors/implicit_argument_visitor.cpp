/*************************************************************************
 * Copyright (C) 2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/
#include "visitors/implicit_argument_visitor.hpp"
#include "ast/ast_decl.hpp"
#include "ast/function_call.hpp"
#include "ast/string.hpp"
#include "codegen/codegen_naming.hpp"
#include "lexer/token_mapping.hpp"

namespace nmodl {
namespace visitor {

void ImplicitArgumentVisitor::visit_function_call(ast::FunctionCall& node) {
    auto function_name = node.get_node_name();
    auto const& arguments = node.get_arguments();
    if (function_name == "nrn_ghk") {
        // This function is traditionally used in MOD files with four arguments, but
        // its value also depends on the global celsius variable so the real
        // function in CoreNEURON has a 5th argument for that.
        if (arguments.size() == 4) {
            auto new_arguments = arguments;
            new_arguments.insert(new_arguments.end(),
                                 std::make_shared<ast::Name>(std::make_shared<ast::String>(
                                     codegen::naming::CELSIUS_VARIABLE)));
            node.set_arguments(std::move(new_arguments));
        }
    } else if (nmodl::details::needs_neuron_thread_first_arg(function_name)) {
        // We need to insert `nt` as the first argument if it's not already
        // there
        auto const is_nt = [](auto const& arg) {
            // Not all node types implement get_node_name(); for example if the
            // first argument is `a+b` then calling `get_node_name` would throw
            auto const node_type = arg.get_node_type();
            using ast::AstNodeType;
            if (node_type == AstNodeType::NAME || node_type == AstNodeType::STRING ||
                node_type == AstNodeType::CONSTANT_VAR || node_type == AstNodeType::VAR_NAME ||
                node_type == AstNodeType::LOCAL_VAR) {
                return arg.get_node_name() == "nt";
            }
            return false;
        };
        if (arguments.empty() || !is_nt(*arguments.front())) {
            auto new_arguments = arguments;
            new_arguments.insert(new_arguments.begin(), std::make_shared<ast::String>("nt"));
            node.set_arguments(std::move(new_arguments));
        }
    }
    node.visit_children(*this);
}

}  // namespace visitor
}  // namespace nmodl
