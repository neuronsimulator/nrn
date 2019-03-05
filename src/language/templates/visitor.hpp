/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once


namespace nmodl {

/* Abstract base class for all visitor implementations */
class Visitor {

    public:
        virtual ~Visitor() = default;

        {% for node in nodes %}
        virtual void visit_{{ node.class_name|snake_case }}(ast::{{ node.class_name }}* node) = 0;
        {% endfor %}
};

}  // namespace nmodl