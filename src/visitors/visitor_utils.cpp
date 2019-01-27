#include <string>
#include <map>
#include <memory>

#include "ast/ast.hpp"
#include "parser/nmodl_driver.hpp"

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
    nmodl::Driver driver;
    auto nmodl_text = "PROCEDURE dummy() { " + code_statement + " }";
    driver.parse_string(nmodl_text);
    auto ast = driver.ast();
    auto procedure = std::dynamic_pointer_cast<ProcedureBlock>(ast->blocks[0]);
    auto statement =
        std::shared_ptr<Statement>(procedure->get_block()->get_statements()[0]->clone());
    return statement;
}
