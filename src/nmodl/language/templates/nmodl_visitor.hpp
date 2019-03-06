/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <set>

#include "ast/ast.hpp"
#include "printer/nmodl_printer.hpp"

namespace nmodl {

/* Visitor for printing AST back to NMODL */
class NmodlPrintVisitor : public Visitor {

    private:
        std::unique_ptr<NMODLPrinter> printer;

         /// node types to exclude while printing
        std::set<ast::AstNodeType> exclude_types;

        /// check if node is to be excluded while printing
        bool is_exclude_type(ast::AstNodeType type) const {
            return exclude_types.find(type) != exclude_types.end();
        }

    public:
        NmodlPrintVisitor() : printer(new NMODLPrinter()) {}
        NmodlPrintVisitor(std::string filename) : printer(new NMODLPrinter(filename)) {}
        NmodlPrintVisitor(std::ostream& stream) : printer(new NMODLPrinter(stream)) {}
        NmodlPrintVisitor(std::ostream& stream, const std::set<ast::AstNodeType>& types)
            : printer(new NMODLPrinter(stream)),
              exclude_types(types) {}

        {% for node in nodes %}
        virtual void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
        {% endfor %}
        template<typename T>
        void visit_element(const std::vector<T>& elements, std::string separator, bool program, bool statement);
};

}  // namespace nmodl