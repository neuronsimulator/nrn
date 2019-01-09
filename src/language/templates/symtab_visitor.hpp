#pragma once

#include "visitors/json_visitor.hpp"
#include "visitors/ast_visitor.hpp"
#include "symtab/symbol_table.hpp"


/* Concrete visitor for constructing symbol table from AST */
class SymtabVisitor : public AstVisitor {

private:
    symtab::ModelSymbolTable* modsymtab;

    std::unique_ptr<JSONPrinter> printer;
    std::string block_to_solve;
    bool update = false;
    bool under_state_block = false;

public:
    explicit SymtabVisitor(bool update = false) : printer(new JSONPrinter()), update(update) {}
    SymtabVisitor(std::stringstream &ss, bool update = false) : printer(new JSONPrinter(ss)), update(update) {}
    SymtabVisitor(std::string filename, bool update = false) : printer(new JSONPrinter(filename)), update(update) {}

    void add_model_symbol_with_property(Node* node, NmodlTypeFlag property);

    void setup_symbol(Node* node, NmodlTypeFlag property);

    void setup_symbol_table(AST* node, const std::string& name, bool is_global);

    void setup_symbol_table(Node* node, const std::string& name, NmodlTypeFlag property, bool is_global);

    void setup_program_symbol_table(Node* node, const std::string& name, bool is_global);

    void setup_symbol_table_for_program_block(Program* node);

    void setup_symbol_table_for_global_block(Node* node);

    void setup_symbol_table_for_scoped_block(Node* node, const std::string& name);

    {% for node in nodes %}
        {% if node.is_symtab_method_required() %}
    void visit_{{ node.class_name|snake_case }}({{ node.class_name }}* node) override;
        {% endif %}
    {% endfor %}
};

