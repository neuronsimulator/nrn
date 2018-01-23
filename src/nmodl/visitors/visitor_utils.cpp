#include <string>
#include <map>

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
