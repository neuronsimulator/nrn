/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include "ast/ast.hpp"
#include "visitors/visitor.hpp"


namespace nmodl {

/**
 * \class AstLookupVisitor
 * \brief Visitor to find ast nodes based on on the ast types
 */

class AstLookupVisitor : public Visitor {
    private:
        /// node types to search in the ast
        std::vector<ast::AstNodeType> types;

        /// matching nodes found in the ast
        std::vector<std::shared_ptr<ast::AST>> nodes;

    public:

        AstLookupVisitor() = default;

        AstLookupVisitor(ast::AstNodeType type) {
            types.push_back(type);
        }

        AstLookupVisitor(std::vector<ast::AstNodeType> types) : types(types) {}

        std::vector<std::shared_ptr<ast::AST>> lookup(ast::Program* node, ast::AstNodeType type);

        std::vector<std::shared_ptr<ast::AST>> lookup(ast::Program* node, std::vector<ast::AstNodeType>& types);

        std::vector<std::shared_ptr<ast::AST>> get_nodes() {
            return nodes;
        }

        void clear() {
            types.clear();
            nodes.clear();
        }

        {% for node in nodes %}
            void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) override;
        {% endfor %}
};

}  // namespace nmodl