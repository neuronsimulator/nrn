/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::codegen::CodegenCompatibilityVisitor
 */

#include <map>
#include <set>

#include "ast/ast.hpp"
#include "codegen_naming.hpp"
#include "lexer/modtoken.hpp"
#include "symtab/symbol_table.hpp"
#include "utils/logger.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace codegen {

using namespace ast;

/**
 * \addtogroup codegen_backends
 * \{
 */

/**
 * \class CodegenCompatibilityVisitor
 * \brief %Visitor for printing compatibility issues of the mod file
 *
 * INDEPENDENT_BLOCK is ignored (no error raised) as stated in:
 * https://www.neuron.yale.edu/neuron/static/py_doc/modelspec/programmatic/mechanisms/nmodl.html
 */
class CodegenCompatibilityVisitor: public visitor::AstVisitor {
    /// Typedef for defining FunctionPointer that points to the
    /// function needed to be called for every kind of error
    typedef std::string (CodegenCompatibilityVisitor::*FunctionPointer)(
        ast::Ast& node,
        const std::shared_ptr<ast::Ast>&) const;

    /// associated container to find the function needed to be called in
    /// for every ast::AstNodeType that is unsupported
    static const std::map<ast::AstNodeType, FunctionPointer> unhandled_ast_types_func;

    /// Set of handled solvers by the NMODL \c C++ code generator
    static const inline std::set<std::string> handled_solvers{codegen::naming::CNEXP_METHOD,
                                                              codegen::naming::EULER_METHOD,
                                                              codegen::naming::DERIVIMPLICIT_METHOD,
                                                              codegen::naming::SPARSE_METHOD,
                                                              codegen::naming::AFTER_CVODE_METHOD};

    const std::string simulator = "coreneuron";

  public:
    /// \name Ctor & dtor
    /// \{

    /// Default CodegenCompatibilityVisitor constructor
    ///
    /// The argument `simulator` must be `"neuron"` for NEURON, everything else
    /// refers to CoreNEURON. The compatibility will be checked as if
    /// generating code for the specified simulator.
    CodegenCompatibilityVisitor(const std::string& simulator = "coreneuron")
        : simulator(simulator) {}

    /// \}

    /// Search the ast::Ast for nodes that
    /// are incompatible with NMODL \c C++ code generator
    ///
    /// \param node Ast
    /// \return bool if there are unhandled nodes or not
    bool find_unhandled_ast_nodes(Ast& node) const;

  private:
    /// Takes as parameter an std::shared_ptr<ast::Ast>,
    /// searches if the method used for solving is supported
    /// and if it is not it returns a relative error message
    ///
    /// \param node Not used by the function
    /// \param ast_node Ast node which is checked
    /// \return std::string error
    std::string return_error_if_solve_method_is_unhandled(
        ast::Ast& node,
        const std::shared_ptr<ast::Ast>& ast_node) const;

    /// Takes as parameter an std::shared_ptr<ast::Ast> node
    /// and returns a relative error with the name, the type
    /// and the location of the unhandled statement
    ///
    /// \tparam T Type of node parameter in the ast::Ast
    /// \param node Not used by the function
    /// \param ast_node Ast node which is checked
    /// \return std::string error
    template <typename T>
    std::string return_error_with_name(ast::Ast& /* node */,
                                       const std::shared_ptr<ast::Ast>& ast_node) const {
        auto real_type_block = std::dynamic_pointer_cast<T>(ast_node);
        auto token = real_type_block->get_token();
        try {
            return fmt::format("\"{}\" {}construct found at [{}] is not handled\n",
                               ast_node->get_node_name(),
                               real_type_block->get_nmodl_name(),
                               token ? token->position() : "unknown location");
        } catch (const std::logic_error&) {
            return fmt::format("{}construct found at [{}] is not handled\n",
                               real_type_block->get_nmodl_name(),
                               token ? token->position() : "unknown location");
        }
    }

    /// Takes as parameter an std::shared_ptr<ast::Ast> node
    /// and returns a relative error with the type and the
    /// location of the unhandled statement
    ///
    /// \tparam T Type of node parameter in the ast::Ast
    /// \param node Not used by the function
    /// \param ast_node Ast node which is checked
    /// \return std::string error
    template <typename T>
    std::string return_error_without_name(ast::Ast& /* node */,
                                          const std::shared_ptr<ast::Ast>& ast_node) const {
        auto real_type_block = std::dynamic_pointer_cast<T>(ast_node);
        return fmt::format("{}construct found at [{}] is not handled\n",
                           real_type_block->get_nmodl_name(),
                           real_type_block->get_token()->position());
    }

    /// Callback when detecting EXTERNAL.
    std::string return_error_extern(ast::Ast& node,
                                    const std::shared_ptr<ast::Ast>& ast_node) const;

    /// Takes as parameter the ast::Ast to read the symbol table
    /// and an std::shared_ptr<ast::Ast> node and returns relative
    /// error if a variable that is writen in the mod file is
    /// defined as GLOBAL instead of RANGE
    ///
    /// \param node Ast
    /// \param ast_node Ast node which is checked
    /// \return std::string error
    std::string return_error_global_var(ast::Ast& node,
                                        const std::shared_ptr<ast::Ast>& ast_node) const;

    std::string return_error_param_var(ast::Ast& node,
                                       const std::shared_ptr<ast::Ast>& ast_node) const;

    /// Takes as parameter the ast::Ast and checks if the
    /// functions "bbcore_read" and "bbcore_write" are defined
    /// in any of the ast::Ast VERBATIM blocks. The function is
    /// called if there is a BBCORE_POINTER defined in the mod
    /// file
    ///
    /// \param node Ast
    /// \param ast_node Not used by the function
    /// \return std::string error
    std::string return_error_if_no_bbcore_read_write(
        ast::Ast& node,
        const std::shared_ptr<ast::Ast>& ast_node) const;
};

/** \} */  // end of codegen_backends

}  // namespace codegen
}  // namespace nmodl
