/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "visitors/ast_visitor.hpp"
#include "printer/json_printer.hpp"

namespace nmodl {

/* Concrete visitor for printing AST in JSON format */
class JSONVisitor : public AstVisitor {

    private:
        std::unique_ptr<JSONPrinter> printer;

    public:
        JSONVisitor() : printer(new JSONPrinter()) {}
        JSONVisitor(std::string filename) : printer(new JSONPrinter(filename)) {}
        JSONVisitor(std::stringstream &ss) : printer(new JSONPrinter(ss)) {}

        void flush() { printer->flush(); }
        void compact_json(bool flag) { printer->compact_json(flag); }
        void expand_keys(bool flag) { printer->expand_keys(flag); }

        {% for node in nodes %}
        void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
        {% endfor %}
};

}  // namespace nmodl