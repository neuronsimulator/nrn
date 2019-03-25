/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include <map>
#include <memory>
#include <string>

#include "ast/ast.hpp"
#include "parser/nmodl_driver.hpp"
#include "visitor_utils.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"


namespace nmodl {

using namespace ast;
using symtab::syminfo::NmodlType;

std::string get_new_name(const std::string& name,
                         const std::string& suffix,
                         std::map<std::string, int>& variables) {
    int counter = 0;
    if (variables.find(name) != variables.end()) {
        counter = variables[name];
    }
    variables[name] = counter + 1;
    return (name + "_" + suffix + "_" + std::to_string(counter));
}

LocalVarVector* get_local_variables(const StatementBlock* node) {
    for (const auto& statement: node->statements) {
        if (statement->is_local_list_statement()) {
            auto local_statement = std::static_pointer_cast<LocalListStatement>(statement);
            return &(local_statement->variables);
        }
    }
    return nullptr;
}

void add_local_statement(StatementBlock* node) {
    auto variables = get_local_variables(node);
    if (variables == nullptr) {
        auto statement = std::make_shared<LocalListStatement>(LocalVarVector());
        node->statements.insert(node->statements.begin(), statement);
    }
}

LocalVar* add_local_variable(StatementBlock* node, Identifier* varname) {
    add_local_statement(node);
    auto local_variables = get_local_variables(node);
    auto var = std::make_shared<LocalVar>(varname);
    local_variables->push_back(var);
    return var.get();
}

LocalVar* add_local_variable(StatementBlock* node, const std::string& varname) {
    auto name = new Name(new String(varname));
    return add_local_variable(node, name);
}

LocalVar* add_local_variable(StatementBlock* node, const std::string& varname, int dim) {
    auto name = new IndexedName(new Name(new String(varname)), new Integer(dim, nullptr));
    return add_local_variable(node, name);
}

/**
 * Convert given code statement (in string format) to corresponding ast node
 *
 * We create dummy nmodl procedure containing given code statement and then
 * parse it using NMODL parser. As there will be only one block with single
 * statement, we return first statement.
 *
 * \todo : Need to revisit this during code generation passes to make sure
 *  if all statements can be part of procedure block.
 */
std::shared_ptr<Statement> create_statement(const std::string& code_statement) {
    nmodl::parser::NmodlDriver driver;
    auto nmodl_text = "PROCEDURE dummy() { " + code_statement + " }";
    driver.parse_string(nmodl_text);
    auto ast = driver.ast();
    auto procedure = std::dynamic_pointer_cast<ProcedureBlock>(ast->blocks[0]);
    auto statement = std::shared_ptr<Statement>(
        procedure->get_statement_block()->get_statements()[0]->clone());
    return statement;
}

/**
 * Convert given code statement (in string format) to corresponding ast node
 *
 * We create dummy nmodl procedure containing given code statement and then
 * parse it using NMODL parser. As there will be only one block with single
 * statement, we return first statement.
 */
std::shared_ptr<StatementBlock> create_statement_block(
    const std::vector<std::string>& code_statements) {
    nmodl::parser::NmodlDriver driver;
    std::string nmodl_text = "PROCEDURE dummy() {\n";
    for (auto& statement: code_statements) {
        nmodl_text += statement + "\n";
    }
    nmodl_text += "}";
    driver.parse_string(nmodl_text);
    auto ast = driver.ast();
    auto procedure = std::dynamic_pointer_cast<ProcedureBlock>(ast->blocks[0]);
    auto statement_block = std::shared_ptr<StatementBlock>(
        procedure->get_statement_block()->clone());
    return statement_block;
}


void remove_statements_from_block(ast::StatementBlock* block,
                                  const std::set<ast::Node*> statements) {
    auto& statement_vec = block->statements;
    statement_vec.erase(std::remove_if(statement_vec.begin(), statement_vec.end(),
                                       [&statements](std::shared_ptr<ast::Statement>& s) {
                                           return statements.find(s.get()) != statements.end();
                                       }),
                        statement_vec.end());
}


std::set<std::string> get_global_vars(Program* node) {
    std::set<std::string> vars;
    if (auto* symtab = node->get_symbol_table()) {
        NmodlType property =
            NmodlType::global_var | NmodlType::range_var | NmodlType::param_assign |
            NmodlType::extern_var | NmodlType::prime_name | NmodlType::dependent_def |
            NmodlType::read_ion_var | NmodlType::write_ion_var | NmodlType::nonspecific_cur_var |
            NmodlType::electrode_cur_var | NmodlType::section_var | NmodlType::constant_var |
            NmodlType::extern_neuron_variable | NmodlType::state_var | NmodlType::factor_def;
        for (const auto& globalvar: symtab->get_variables_with_properties(property)) {
            vars.insert(globalvar->get_name());
        }
    }
    return vars;
}


std::string to_nmodl(ast::AST* node, const std::set<ast::AstNodeType>& exclude_types) {
    std::stringstream stream;
    NmodlPrintVisitor v(stream, exclude_types);
    node->accept(&v);
    return stream.str();
}

std::string to_json(ast::AST* node, bool compact, bool expand) {
    std::stringstream stream;
    JSONVisitor v(stream);
    v.compact_json(compact);
    v.expand_keys(expand);
    node->accept(&v);
    v.flush();
    return stream.str();
}

}  // namespace nmodl
