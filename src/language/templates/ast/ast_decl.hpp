/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#pragma once

#include <memory>
#include <utility>
#include <vector>


namespace nmodl {
namespace ast {

    {% for node in nodes %}
    class {{ node.class_name }};
    {% endfor %}

    /* Type for every ast node */
    enum class AstNodeType {
        {% for node in nodes %}
        {{ node.class_name|snake_case|upper }},
        {% endfor %}
    };

    /* std::vector for convenience */
    {% for node in nodes %}
    using {{ node.class_name }}Vector = std::vector<std::shared_ptr<{{ node.class_name }}>>;
    {% endfor %}

}  // namespace ast
}  // namespace nmodl