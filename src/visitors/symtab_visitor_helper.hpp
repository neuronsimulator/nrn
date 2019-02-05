#include <utility>

#include "lexer/token_mapping.hpp"
#include "visitors/symtab_visitor.hpp"

using namespace ast;
using namespace symtab;
using namespace syminfo;

// create symbol for given node
static std::shared_ptr<Symbol> create_symbol_for_node(Node* node,
                                                      NmodlType property,
                                                      bool under_state_block) {
    ModToken token;
    auto token_ptr = node->get_token();
    if (token_ptr != nullptr) {
        token = *token_ptr;
    }

    /// add new symbol
    auto symbol = std::make_shared<Symbol>(node->get_node_name(), node, token);
    symbol->add_property(property);

    // non specific variable is range
    if (property == NmodlType::nonspecific_cur_var) {
        symbol->add_property(NmodlType::range_var);
    }

    /// extra property for state variables
    if (under_state_block) {
        symbol->add_property(NmodlType::state_var);
    }
    return symbol;
}


/// helper function to setup/insert symbol into symbol table
/// for the ast nodes which are of variable types
void SymtabVisitor::setup_symbol(Node* node, NmodlType property) {
    std::shared_ptr<Symbol> symbol;
    auto name = node->get_node_name();

    /// if prime variable is already exist in symbol table
    /// then just update the order
    if (node->is_prime_name()) {
        auto prime = dynamic_cast<PrimeName*>(node);
        symbol = modsymtab->lookup(name);
        if (symbol) {
            symbol->set_order(prime->get_order()->eval());
            symbol->add_property(property);
            return;
        }
    }

    /// range and non_spec_cur can appear in any order in neuron block.
    /// for both properties, we have to check if symbol is already exist.
    /// if so we have to return to avoid duplicate definition error.
    if (property == NmodlType::range_var || property == NmodlType::nonspecific_cur_var) {
        auto s = modsymtab->lookup(name);
        if (s && s->has_properties(NmodlType::nonspecific_cur_var | NmodlType::range_var)) {
            s->add_property(property);
            return;
        }
    }

    symbol = create_symbol_for_node(node, property, under_state_block);

    /// insert might return different symbol if already exist in the same scope
    symbol = modsymtab->insert(symbol);

    if (node->is_param_assign()) {
        auto parameter = dynamic_cast<ParamAssign*>(node);
        auto value = parameter->get_value();
        auto name = parameter->get_name();
        if (value) {
            symbol->set_value(value->to_double());
        }
        if (name->is_indexed_name()) {
            auto index_name = dynamic_cast<IndexedName*>(name.get());
            auto length = dynamic_cast<Integer*>(index_name->get_length().get());
            symbol->set_as_array(length->eval());
        }
    }

    if (node->is_dependent_def()) {
        auto variable = dynamic_cast<DependentDef*>(node);
        auto length = variable->get_length();
        if (length) {
            symbol->set_as_array(length->eval());
        }
    }

    if (node->is_constant_var()) {
        auto constant = dynamic_cast<ConstantVar*>(node);
        auto value = constant->get_value();
        if (value) {
            symbol->set_value(value->to_double());
        }
    }

    if (node->is_local_var()) {
        auto variable = dynamic_cast<LocalVar*>(node);
        auto name = variable->get_name();
        if (name->is_indexed_name()) {
            auto index_name = dynamic_cast<IndexedName*>(name.get());
            auto length = dynamic_cast<Integer*>(index_name->get_length().get());
            symbol->set_as_array(length->eval());
        }
    }

    /// visit children, most likely variables are already
    /// leaf nodes, not necessary to visit
    node->visit_children(this);
}


void SymtabVisitor::add_model_symbol_with_property(Node* node, NmodlType property) {
    auto token = node->get_token();
    auto name = node->get_node_name();
    auto symbol = std::make_shared<Symbol>(name, node, *token);
    symbol->add_property(property);

    if (name == block_to_solve) {
        symbol->add_property(NmodlType::to_solve);
    }

    modsymtab->insert(symbol);
}


static void add_external_symbols(symtab::ModelSymbolTable* symtab) {
    ModToken tok(true);
    auto variables = nmodl::get_external_variables();
    for (auto variable : variables) {
        auto symbol = std::make_shared<Symbol>(variable, nullptr, tok);
        symbol->add_property(NmodlType::extern_neuron_variable);
        symtab->insert(symbol);
    }
    auto methods = nmodl::get_external_functions();
    for (auto method : methods) {
        auto symbol = std::make_shared<Symbol>(method, nullptr, tok);
        symbol->add_property(NmodlType::extern_method);
        symtab->insert(symbol);
    }
}


void SymtabVisitor::setup_symbol_table(AST* node, const std::string& name, bool is_global) {
    /// entering into new nmodl block
    auto symtab = modsymtab->enter_scope(name, node, is_global, node->get_symbol_table());

    if (node->is_state_block()) {
        under_state_block = true;
    }

    /// there is only one solve statement allowed in mod file
    if (node->is_solve_block()) {
        auto solve_block = dynamic_cast<SolveBlock*>(node);
        block_to_solve = solve_block->get_block_name()->get_node_name();
    }

    /// not required at the moment but every node
    /// has pointer to associated symbol table
    node->set_symbol_table(symtab);

    /// when visiting highest level node i.e. Program, we insert
    /// all global variables to the global symbol table
    if (node->is_program()) {
        add_external_symbols(modsymtab);
    }

    /// look for all children blocks recursively
    node->visit_children(this);

    /// existing nmodl block
    modsymtab->leave_scope();

    if (node->is_state_block()) {
        under_state_block = false;
    }
}


/**
 *  Symtab visitor could be called multiple times, after optimization passes,
 *  in which case we have to throw awayold symbol tables and setup new ones.
 */
void SymtabVisitor::setup_symbol_table_for_program_block(Program* node) {
    modsymtab = node->get_model_symbol_table();
    modsymtab->set_mode(update);
    setup_symbol_table(node, node->get_node_type_name(), true);
}


void SymtabVisitor::setup_symbol_table_for_global_block(Node* node) {
    setup_symbol_table(node, node->get_node_type_name(), true);
}


void SymtabVisitor::setup_symbol_table_for_scoped_block(Node* node, const std::string& name) {
    setup_symbol_table(node, name, false);
}


/**
 * Visit table statement and update symbol in symbol table
 *
 * @todo : we assume table statement follows variable declaration
 */
void SymtabVisitor::visit_table_statement(ast::TableStatement* node) {
    auto update_symbol = [this](const NameVector& variables, NmodlType property, int num_values) {
        for (auto& var : variables) {
            auto name = var->get_node_name();
            auto symbol = modsymtab->lookup(name);
            if (symbol) {
                symbol->add_property(property);
                symbol->set_num_values(num_values);
            }
        }
    };
    int num_values = node->get_with()->eval() + 1;
    update_symbol(node->get_table_vars(), NmodlType::table_statement_var, num_values);
    update_symbol(node->get_depend_vars(), NmodlType::table_dependent_var, num_values);
}
