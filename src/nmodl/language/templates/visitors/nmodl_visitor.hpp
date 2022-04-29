/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#pragma once

/**
 * \file
 * \brief \copybrief nmodl::visitor::NmodlPrintVisitor
 */

#include <set>

#include "visitors/visitor.hpp"
#include "printer/nmodl_printer.hpp"

namespace nmodl {
namespace visitor {

/**
 * \addtogroup visitor_classes
 * \{
 */

/**
 * \class NmodlPrintVisitor
 * \brief %Visitor for printing AST back to NMODL
 */
class NmodlPrintVisitor: public ConstVisitor {
  private:
    std::unique_ptr<printer::NMODLPrinter> printer;

    /// node types to exclude while printing
    std::set<ast::AstNodeType> exclude_types;

    /// check if node is to be excluded while printing
    bool is_exclude_type(ast::AstNodeType type) const {
        return exclude_types.find(type) != exclude_types.end();
    }

  public:
    NmodlPrintVisitor()
        : printer(new printer::NMODLPrinter()) {}

    NmodlPrintVisitor(std::string filename)
        : printer(new printer::NMODLPrinter(filename)) {}

    NmodlPrintVisitor(std::ostream& stream)
        : printer(new printer::NMODLPrinter(stream)) {}

    NmodlPrintVisitor(std::ostream& stream, const std::set<ast::AstNodeType>& types)
        : printer(new printer::NMODLPrinter(stream))
        , exclude_types(types){}

    // clang-format off
    {% for node in nodes %}
    virtual void visit_{{ node.class_name|snake_case }}(const ast::{{ node.class_name }}& node) override;
    {% endfor %}
    // clang-format on

    template <typename T>
    void visit_element(const std::vector<T>& elements,
                       const std::string& separator,
                       bool program,
                       bool statement);
};

/** \} */  // end of visitor_classes

}  // namespace visitor
}  // namespace nmodl

