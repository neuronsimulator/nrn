#ifndef _SYMTAB_VISITOR_HELPER_HPP_
#define _SYMTAB_VISITOR_HELPER_HPP_

#include "lexer/token_mapping.hpp"
#include "symtab/symbol_table.hpp"

template <typename T>
void SymtabVisitor::setupSymbol(T* node, symtab::details::NmodlInfo property, int order) {
    std::shared_ptr<Symbol> symbol;
    auto token = node->getToken();

    if (order) {
        symbol = modsymtab->lookup(node->getName());
        if (symbol) {
            symbol->set_order(order);
            symbol->add_property(property);
            return;
        }
    }

    symbol = std::make_shared<Symbol>(node->getName(), node, *token);
    symbol->add_property(property);
    modsymtab->insert(symbol);
    node->visitChildren(this);
}

template <typename T>
void SymtabVisitor::setupSymbolTable(T* node,
                                     symtab::details::NmodlInfo property,
                                     bool global_block) {
    auto token = node->getToken();
    auto symbol = std::make_shared<Symbol>(node->getName(), node, *token);
    symbol->add_property(property);
    modsymtab->insert(symbol);
    setupSymbolTable(node, global_block);
}

template <typename T>
void SymtabVisitor::setupSymbolTable(T* node, bool global_block) {
    auto symtab = modsymtab->enter_scope(node->getName(), node, global_block);
    node->setBlockSymbolTable(symtab);

    if (node->isProgram()) {
        auto variables = nmodl::all_external_variables();
        for (auto variable : variables) {
            ModToken tok(true);
            auto symbol = std::make_shared<Symbol>(variable, nullptr, tok);
            symbol->add_property(symtab::details::NmodlInfo::extern_token);
            modsymtab->insert(symbol);
        }
    }

    node->visitChildren(this);
    modsymtab->leave_scope();
}

#endif