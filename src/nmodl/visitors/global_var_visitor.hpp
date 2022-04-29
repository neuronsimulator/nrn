/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::GlobalToRangeVisitor
 */

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class GlobalToRangeVisitor
 * \brief Visitor to convert GLOBAL variables to RANGE variables
 *
 * Some of the existing mod files have GLOBAL variables that are updated in BREAKPOINT
 * or DERIVATIVE blocks. These variables have a single copy and works well when they
 * are read only. If such variables are written as well, they result into race condition.
 * For example,
 *
 * \code{.mod}
 *      NEURON {
 *    	    SUFFIX test
 *    	    GLOBAL x
 *      }
 *
 *      ASSIGNED {
 *          x
 *      }
 *
 *      BREAKPOINT {
 *          x = 1
 *      }
 * \endcode
 *
 * In above example, \a x will be simultaneously updated in case of vectorization. In NEURON,
 * such race condition is avoided by promoting these variables to thread variable (i.e. separate
 * copy per thread). In case of CoreNEURON, this is not sufficient because of vectorisation or
 * GPU execution. To address this issue, this visitor converts GLOBAL variables to RANGE variables
 * when they are assigned / updated in compute functions.
 */

class GlobalToRangeVisitor: public AstVisitor {
  private:
    /// ast::Ast* node
    const ast::Program& ast;

  public:
    /// \name Ctor & dtor
    /// \{

    /// Default constructor
    GlobalToRangeVisitor() = delete;

    /// Constructor that takes as parameter the AST
    explicit GlobalToRangeVisitor(const ast::Program& node)
        : ast(node) {}

    /// \}

    /// Visit ast::NeuronBlock nodes to check if there is any GLOBAL
    /// variables defined in them that are written in any part of the code.
    /// This is checked by reading the write_count member of the variable in
    /// the symtab::SymbolTable. If it's written it removes the variable from
    /// the ast::Global node and adds it to the ast::Range node of the
    /// ast::NeuronBlock
    void visit_neuron_block(ast::NeuronBlock& node) override;
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl
