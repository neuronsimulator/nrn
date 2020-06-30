/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

///
/// THIS FILE IS GENERATED AT BUILD TIME AND SHALL NOT BE EDITED.
///

#pragma once

/**
 * \dir
 * \brief Auto generated AST Implementations
 *
 * \file
 * \brief Auto generated AST classes declaration
 */

#include "ast/ast.hpp"

{% for node in nodes %}
#ifndef {{ node.cpp_fence }}
#define {{ node.cpp_fence }}
{% if node.has_template_methods %}
#define {{ node.cpp_fence }}_INLINE_DEFINITION_REQUIRED
{% endif %}
{% include "ast/node_class.template" %}
#endif // !{{ node.cpp_fence }}
{% endfor %}

{# add inline definitions of template member methods #}
namespace nmodl {
namespace ast {
{% for node in nodes %}
{%- if node.has_template_methods %}
#ifdef {{ node.cpp_fence }}_INLINE_DEFINITION_REQUIRED
  {% include "ast/node_class_inline_definition.template" %}
#endif  // !{{ node.cpp_fence }}_INLINE_DEFINITION_REQUIRED

{% endif %}
{%- endfor %}
}  // namespace ast
}  // namespace nmodl
