/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "visitors/verbatim_visitor.hpp"

#include <iostream>

#include "ast/string.hpp"
#include "ast/verbatim.hpp"

namespace nmodl {
namespace visitor {

void VerbatimVisitor::visit_verbatim(const ast::Verbatim& node) {
    std::string block;
    const auto& statement = node.get_statement();
    if (statement) {
        block = statement->eval();
    }
    if (!block.empty() && verbose) {
        std::cout << "BLOCK START" << block << "\nBLOCK END \n\n";
    }

    blocks.push_back(block);
}

}  // namespace visitor
}  // namespace nmodl
