/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "ast/ast.hpp"
#include "printer/nmodl_printer.hpp"

namespace nmodl {

/* Visitor for printing AST back to NMODL */
class NmodlPrintVisitor : public Visitor {

    private:
        std::unique_ptr<NMODLPrinter> printer;
    public:
        NmodlPrintVisitor() : printer(new NMODLPrinter()) {}
        NmodlPrintVisitor(std::string filename) : printer(new NMODLPrinter(filename)) {}
        NmodlPrintVisitor(std::ostream& stream) : printer(new NMODLPrinter(stream)) {}

        {% for node in nodes %}
        virtual void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
        {% endfor %}
        template<typename T>
        void visit_element(const std::vector<T>& elements, std::string separator, bool program, bool statement);
};

}  // namespace nmodl