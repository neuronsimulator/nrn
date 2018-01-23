#include <string>
#include <map>
#include <utility>

#include "ast/ast.hpp"

using namespace ast;

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
    for (const auto& statement : node->statements) {
        if (statement->get_type() == Type::LOCAL_LIST_STATEMENT) {
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

void add_local_variable(ast::StatementBlock* node, ast::Identifier* varname) {
    add_local_statement(node);
    auto local_variables = get_local_variables(node);
    local_variables->push_back(std::make_shared<LocalVar>(varname));
}

void add_local_variable(ast::StatementBlock* node, const std::string& varname) {
    auto name = new ast::Name(new ast::String(varname));
    add_local_variable(node, name);
}
