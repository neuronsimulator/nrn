#ifndef _SYMTAB_VISITOR_HELPER_HPP_
#define _SYMTAB_VISITOR_HELPER_HPP_

#include "lexer/token_mapping.hpp"
#include "symtab/symbol_table.hpp"

/// helper function to setup/insert symbol into symbol table
/// for the ast nodes which are of variable types
template <typename T>
void SymtabVisitor::setupSymbol(T* node, SymbolInfo property, int order) {
    std::shared_ptr<Symbol> symbol;
    auto token = node->getToken();

    /// if prime variable is already exist in symbol table
    /// then just update the order
    if (order) {
        symbol = modsymtab->lookup(node->getName());
        if (symbol) {
            symbol->set_order(order);
            symbol->add_property(property);
            return;
        }
    }

    /// add new symbol
    symbol = std::make_shared<Symbol>(node->getName(), node, *token);
    symbol->add_property(property);
    modsymtab->insert(symbol);

    /// visit childrens, most likely variables are already
    /// leaf nodes, not necessary to visit
    node->visitChildren(this);
}

template <typename T>
void SymtabVisitor::setupSymbolTable(T* node,
                                     std::string name,
                                     SymbolInfo property,
                                     bool global_block) {
    auto token = node->getToken();
    auto symbol = std::make_shared<Symbol>(name, node, *token);
    symbol->add_property(property);
    modsymtab->insert(symbol);
    setupSymbolTable(node, name, global_block);
}

template <typename T>
void SymtabVisitor::setupSymbolTable(T* node, std::string name, bool global_block) {

    /// entering into new nmodl block
    auto symtab = modsymtab->enter_scope(name, node, global_block);

    /// not required at the moment but every node
    /// has pointer to associated symbol table
    node->setBlockSymbolTable(symtab);

    /// when visiting highest level node i.e. Program, we insert
    /// all global variables to the global symbol table
    if (node->isProgram()) {
        auto variables = nmodl::all_external_variables();
        for (auto variable : variables) {
            ModToken tok(true);
            auto symbol = std::make_shared<Symbol>(variable, nullptr, tok);
            symbol->add_property(symtab::details::NmodlInfo::extern_token);
            modsymtab->insert(symbol);
        }
    }

    /// look for all children blocks recursively
    node->visitChildren(this);

    /// exisiting nmodl block
    modsymtab->leave_scope();
}

#endif