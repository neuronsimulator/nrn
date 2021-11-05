#include "visitors/semantic_analysis_visitor.hpp"
#include "ast/function_block.hpp"
#include "ast/procedure_block.hpp"
#include "ast/table_statement.hpp"

namespace nmodl {
namespace visitor {

void SemanticAnalysisVisitor::visit_procedure_block(const ast::ProcedureBlock& node) {
    in_procedure_function = true;
    one_arg_in_procedure_function = node.get_parameters().size() == 1;
    node.visit_children(*this);
    in_procedure_function = false;
}

void SemanticAnalysisVisitor::visit_function_block(const ast::FunctionBlock& node) {
    in_procedure_function = true;
    one_arg_in_procedure_function = node.get_parameters().size() == 1;
    node.visit_children(*this);
    in_procedure_function = false;
}

void SemanticAnalysisVisitor::visit_table_statement(const ast::TableStatement&) {
    if (in_procedure_function && !one_arg_in_procedure_function) {
        throw std::runtime_error(
            "The procedure or function containing the TABLE statement should contains exactly one "
            "argument.");
    }
}

}  // namespace visitor
}  // namespace nmodl
