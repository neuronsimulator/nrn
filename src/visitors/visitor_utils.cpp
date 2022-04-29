/*************************************************************************
 * Copyright (C) 2018-2022 Blue Brain Project
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

#include <fmt/format.h>

namespace nmodl {
namespace visitor {

using namespace ast;
using symtab::syminfo::NmodlType;

using nmodl::utils::UseNumbersInString;

std::string suffix_random_string(const std::set<std::string>& vars,
                                 const std::string& original_string,
                                 const UseNumbersInString use_num) {
    // If the "original_string" is not in the set of the variables to check then
    // return the "original_string" without suffix
    if (vars.find(original_string) == vars.end()) {
        return original_string;
    }
    std::string new_string = original_string;
    auto& singleton_random_string_class = nmodl::utils::SingletonRandomString<4>::instance();
    // Check if there is a variable defined in the mod file and, if yes, try to use
    // a different string in the form "original_string"_"random_string"
    // If there is already a "random_string" assigned to the "originl_string" return it
    if (singleton_random_string_class.random_string_exists(original_string)) {
        const auto random_suffix = "_" +
                                   singleton_random_string_class.get_random_string(original_string);
        new_string = original_string + random_suffix;
    } else {
        // Check if the "random_string" already exists in the set of variables and if it does try
        // to find another random string to add as suffix
        while (vars.find(new_string) != vars.end()) {
            const auto random_suffix =
                "_" + singleton_random_string_class.reset_random_string(original_string, use_num);
            new_string = original_string + random_suffix;
        }
    }
    return new_string;
}

std::string get_new_name(const std::string& name,
                         const std::string& suffix,
                         std::map<std::string, int>& variables) {
    auto it = variables.emplace(name, 0);
    auto counter = it.first->second;
    ++it.first->second;

    std::ostringstream oss;
    oss << name << '_' << suffix << '_' << counter;
    return oss.str();
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

std::vector<std::shared_ptr<Statement>> create_statements(
    const std::vector<std::string>::const_iterator& code_statements_beg,
    const std::vector<std::string>::const_iterator& code_statements_end) {
    std::vector<std::shared_ptr<Statement>> statements;
    statements.reserve(code_statements_end - code_statements_beg);
    std::transform(code_statements_beg,
                   code_statements_end,
                   std::back_inserter(statements),
                   [](const std::string& s) { return create_statement(s); });
    return statements;
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


bool calls_function(const ast::Ast& node, const std::string& name) {
    const auto& function_calls = collect_nodes(node, {ast::AstNodeType::FUNCTION_CALL});
    return std::any_of(
        function_calls.begin(),
        function_calls.end(),
        [&name](const std::shared_ptr<const ast::Ast>& f) {
            return std::dynamic_pointer_cast<const ast::FunctionCall>(f)->get_node_name() == name;
        });
}

}  // namespace visitor

std::vector<std::shared_ptr<const ast::Ast>> collect_nodes(
    const ast::Ast& node,
    const std::vector<ast::AstNodeType>& types) {
    visitor::ConstAstLookupVisitor visitor;
    return visitor.lookup(node, types);
}

std::vector<std::shared_ptr<ast::Ast>> collect_nodes(ast::Ast& node,
                                                     const std::vector<ast::AstNodeType>& types) {
    visitor::AstLookupVisitor visitor;
    return visitor.lookup(node, types);
}

bool sparse_solver_exists(const ast::Ast& node) {
    const auto solve_blocks = collect_nodes(node, {ast::AstNodeType::SOLVE_BLOCK});
    for (const auto& solve_block: solve_blocks) {
        const auto& method = dynamic_cast<const ast::SolveBlock*>(solve_block.get())->get_method();
        if (method && method->get_node_name() == "sparse") {
            return true;
        }
    }
    return false;
}

std::string to_nmodl(const ast::Ast& node, const std::set<ast::AstNodeType>& exclude_types) {
    std::stringstream stream;
    visitor::NmodlPrintVisitor v(stream, exclude_types);
    node.accept(v);
    return stream.str();
}


std::string to_json(const ast::Ast& node, bool compact, bool expand, bool add_nmodl) {
    std::stringstream stream;
    visitor::JSONVisitor v(stream);
    v.compact_json(compact);
    v.add_nmodl(add_nmodl);
    v.expand_keys(expand);
    node.accept(v);
    v.flush();
    return stream.str();
}

std::pair<std::string, std::unordered_set<std::string>> statement_dependencies(
    const std::shared_ptr<ast::Expression>& lhs,
    const std::shared_ptr<ast::Expression>& rhs) {
    std::string key;
    std::unordered_set<std::string> out;

    if (!lhs->is_var_name()) {
        return {key, out};
    }

    const auto& lhs_var_name = std::dynamic_pointer_cast<ast::VarName>(lhs);
    key = get_full_var_name(*lhs_var_name);

    visitor::AstLookupVisitor lookup_visitor;
    lookup_visitor.lookup(*rhs, ast::AstNodeType::VAR_NAME);
    auto rhs_nodes = lookup_visitor.get_nodes();
    std::for_each(rhs_nodes.begin(),
                  rhs_nodes.end(),
                  [&out](const std::shared_ptr<ast::Ast>& node) { out.emplace(to_nmodl(node)); });


    return {key, out};
}

std::string get_indexed_name(const ast::IndexedName& node) {
    return fmt::format("{}[{}]", node.get_node_name(), to_nmodl(node.get_length()));
}

std::string get_full_var_name(const ast::VarName& node) {
    std::string full_var_name;
    if (node.get_name()->is_indexed_name()) {
        auto index_name_node = std::dynamic_pointer_cast<ast::IndexedName>(node.get_name());
        full_var_name = get_indexed_name(*index_name_node);
    } else {
        full_var_name = node.get_node_name();
    }
    return full_var_name;
}

}  // namespace nmodl
