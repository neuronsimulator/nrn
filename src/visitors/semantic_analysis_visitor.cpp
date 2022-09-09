#include "visitors/semantic_analysis_visitor.hpp"
#include "ast/function_block.hpp"
#include "ast/procedure_block.hpp"
#include "ast/program.hpp"
#include "ast/suffix.hpp"
#include "ast/table_statement.hpp"
#include "symtab/symbol_properties.hpp"
#include "utils/logger.hpp"
#include "visitors/visitor_utils.hpp"

namespace nmodl {
namespace visitor {

bool SemanticAnalysisVisitor::check(const ast::Program& node) {
    check_fail = false;

    /// <-- This code is for check 2
    const auto& suffix_node = collect_nodes(node, {ast::AstNodeType::SUFFIX});
    if (!suffix_node.empty()) {
        const auto& suffix = std::dynamic_pointer_cast<const ast::Suffix>(suffix_node[0]);
        const auto& type = suffix->get_type()->get_node_name();
        is_point_process = (type == "POINT_PROCESS" || type == "ARTIFICIAL_CELL");
    }
    /// -->

    /// <-- This code is for check 4
    using namespace symtab::syminfo;
    const auto& with_prop = NmodlType::read_ion_var | NmodlType::write_ion_var;

    const auto& sym_table = node.get_symbol_table();
    assert(sym_table != nullptr);

    // get all ion variables
    const auto& ion_variables = sym_table->get_variables_with_properties(with_prop, false);

    /// make sure ion variables aren't redefined in a `CONSTANT` block.
    for (const auto& var: ion_variables) {
        if (var->has_any_property(NmodlType::constant_var)) {
            logger->critical(
                fmt::format("SemanticAnalysisVisitor :: ion variable {} from the USEION statement "
                            "can not be re-declared in a CONSTANT block",
                            var->get_name()));
            check_fail = true;
        }
    }
    /// -->

    visit_program(node);
    return check_fail;
}

void SemanticAnalysisVisitor::visit_procedure_block(const ast::ProcedureBlock& node) {
    /// <-- This code is for check 1
    in_procedure = true;
    one_arg_in_procedure_function = node.get_parameters().size() == 1;
    node.visit_children(*this);
    in_procedure = false;
    /// -->
}

void SemanticAnalysisVisitor::visit_function_block(const ast::FunctionBlock& node) {
    /// <-- This code is for check 1
    in_function = true;
    one_arg_in_procedure_function = node.get_parameters().size() == 1;
    node.visit_children(*this);
    in_function = false;
    /// -->
}

void SemanticAnalysisVisitor::visit_table_statement(const ast::TableStatement& tableStmt) {
    /// <-- This code is for check 1
    if ((in_function || in_procedure) && !one_arg_in_procedure_function) {
        logger->critical(
            "SemanticAnalysisVisitor :: The procedure or function containing the TABLE statement "
            "should contains exactly one argument.");
        check_fail = true;
    }
    /// -->
    /// <-- This code is for check 3
    const auto& table_vars = tableStmt.get_table_vars();
    if (in_function && !table_vars.empty()) {
        logger->critical(
            "SemanticAnalysisVisitor :: TABLE statement in FUNCTION cannot have a table name "
            "list.");
    }
    if (in_procedure && table_vars.empty()) {
        logger->critical(
            "SemanticAnalysisVisitor :: TABLE statement in PROCEDURE must have a table name list.");
    }
    /// -->
}

void SemanticAnalysisVisitor::visit_destructor_block(const ast::DestructorBlock& /* node */) {
    /// <-- This code is for check 2
    if (!is_point_process) {
        logger->warn(
            "SemanticAnalysisVisitor :: This mod file is not point process but contains a "
            "destructor.");
        check_fail = true;
    }
    /// -->
}

}  // namespace visitor
}  // namespace nmodl
