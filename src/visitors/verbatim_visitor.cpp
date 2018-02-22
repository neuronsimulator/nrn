#include <iostream>
#include "visitors/verbatim_visitor.hpp"

void VerbatimVisitor::visit_verbatim(Verbatim* node) {
    std::string block;
    auto statement = node->get_statement();
    if (statement) {
        block = statement->eval();
    }
    if (!block.empty() && verbose) {
        std::cout << "BLOCK START";
        std::cout << block;
        std::cout << "\nBLOCK END \n\n";
    }

    blocks.push_back(block);
}