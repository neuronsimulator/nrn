/*************************************************************************
 * Copyright (C) 2018-2019 Blue Brain Project
 *
 * This file is part of NMODL distributed under the terms of the GNU
 * Lesser General Public License. See top-level LICENSE file for details.
 *************************************************************************/

#include "visitor_utils.hpp"

#include <map>
#include <memory>
#include <string>

#include "ast/all.hpp"
#include "parser/nmodl_driver.hpp"
#include "utils/string_utils.hpp"
#include "visitors/json_visitor.hpp"
#include "visitors/lookup_visitor.hpp"
#include "visitors/nmodl_visitor.hpp"


namespace nmodl {
namespace visitor {

using namespace ast;
using symtab::syminfo::NmodlType;

std::string suffix_random_string(const std::set<std::string>& vars,
                                 const std::string& original_string) {
    std::string new_string = original_string;
    std::string random_string;
    auto singleton_random_string_class = nmodl::utils::SingletonRandomString<4>::instance();
    // Check if there is a variable defined in the mod file as original_string and if yes
    // try to use a different string in the form "original_string"_"random_string"
    while (vars.find(new_string) != vars.end()) {
        random_string = singleton_random_string_class->reset_random_string(original_string);
        new_string = original_string;
        new_string += "_" + random_string;
    }
    return new_string;
}

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

std::shared_ptr<ast::LocalListStatement> get_local_list_statement(const StatementBlock& node) {
    const auto& statements = node.get_statements();
    for (const auto& statement: statements) {
        if (statement->is_local_list_statement()) {
            return std::static_pointer_cast<LocalListStatement>(statement);
        }
    }
    return nullptr;
}

void add_local_statement(StatementBlock& node) {
    auto variables = get_local_list_statement(node);
    const auto& statements = node.get_statements();
    if (variables == nullptr) {
        auto statement = std::make_shared<LocalListStatement>(LocalVarVector());
        node.insert_statement(statements.begin(), statement);
    }
}

LocalVar* add_local_variable(StatementBlock& node, Identifier* varname) {
    add_local_statement(node);

    const ast::StatementVector& statements = node.get_statements();

    auto local_list_statement = get_local_list_statement(node);
    /// each block should already have local statement
    if (local_list_statement == nullptr) {
        throw std::logic_error("no local statement");
    }
    auto var = std::make_shared<LocalVar>(varname);
    local_list_statement->emplace_back_local_var(var);

    return var.get();
}

LocalVar* add_local_variable(StatementBlock& node, const std::string& varname) {
    auto name = new Name(new String(varname));
    return add_local_variable(node, name);
}

LocalVar* add_local_variable(StatementBlock& node, const std::string& varname, int dim) {
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
 * \todo Need to revisit this during code generation passes to make sure
 *  if all statements can be part of procedure block.
 */
std::shared_ptr<Statement> create_statement(const std::string& code_statement) {
    nmodl::parser::NmodlDriver driver;
    auto nmodl_text = "PROCEDURE dummy() { " + code_statement + " }";
    auto ast = driver.parse_string(nmodl_text);
    auto procedure = std::dynamic_pointer_cast<ProcedureBlock>(ast->get_blocks().front());
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
    auto ast = driver.parse_string(nmodl_text);
    auto procedure = std::dynamic_pointer_cast<ProcedureBlock>(ast->get_blocks().front());
    auto statement_block = std::shared_ptr<StatementBlock>(
        procedure->get_statement_block()->clone());
    return statement_block;
}

std::set<std::string> get_global_vars(const Program& node) {
    std::set<std::string> vars;
    if (auto* symtab = node.get_symbol_table()) {
        // NB: local_var included here as locals can be declared at global scope
        NmodlType property = NmodlType::global_var | NmodlType::local_var | NmodlType::range_var |
                             NmodlType::param_assign | NmodlType::extern_var |
                             NmodlType::prime_name | NmodlType::assigned_definition |
                             NmodlType::read_ion_var | NmodlType::write_ion_var |
                             NmodlType::nonspecific_cur_var | NmodlType::electrode_cur_var |
                             NmodlType::section_var | NmodlType::constant_var |
                             NmodlType::extern_neuron_variable | NmodlType::state_var |
                             NmodlType::factor_def;
        for (const auto& globalvar: symtab->get_variables_with_properties(property)) {
            std::string var_name = globalvar->get_name();
            if (globalvar->is_array()) {
                var_name += "[" + std::to_string(globalvar->get_length()) + "]";
            }
            vars.insert(var_name);
        }
    }
    return vars;
}


bool calls_function(ast::Ast& node, const std::string& name) {
    AstLookupVisitor lv(ast::AstNodeType::FUNCTION_CALL);
    for (const auto& f: lv.lookup(node)) {
        if (std::dynamic_pointer_cast<ast::FunctionCall>(f)->get_node_name() == name) {
            return true;
        }
    }
    return false;
}

}  // namespace visitor


std::string to_nmodl(ast::Ast& node, const std::set<ast::AstNodeType>& exclude_types) {
    std::stringstream stream;
    visitor::NmodlPrintVisitor v(stream, exclude_types);
    node.accept(v);
    return stream.str();
}


std::string to_json(ast::Ast& node, bool compact, bool expand, bool add_nmodl) {
    std::stringstream stream;
    visitor::JSONVisitor v(stream);
    v.compact_json(compact);
    v.add_nmodl(add_nmodl);
    v.expand_keys(expand);
    node.accept(v);
    v.flush();
    return stream.str();
}

}  // namespace nmodl
