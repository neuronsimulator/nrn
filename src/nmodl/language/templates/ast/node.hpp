/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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

#include "ast/ast_decl.hpp"
{% for header in node.cpp_header_deps() %}
#include "{{ header }}"
{% endfor %}

{% include "ast/node_class.template" %}
