#include "visitors/verbatim_visitor.hpp"

void VerbatimVisitor::visitVerbatim(Verbatim* node) {
    std::string block;

    if(node->statement) {
        block = node->statement->eval();
    }

    if (block.size() && verbose) {
        std::cout << "BLOCK START";
        std::cout << block;
        std::cout << "\nBLOCK END \n\n";
    }

    blocks.push_back(block);
}