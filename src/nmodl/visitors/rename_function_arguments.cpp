#include "rename_function_arguments.hpp"

#include "rename_visitor.hpp"

namespace nmodl {
namespace visitor {

template <class Block>
void RenameFunctionArgumentsVisitor::rename_arguments(Block& block) const {
    auto parameters = block.get_parameters();
    auto parameter_names = std::vector<std::string>{};

    for (const auto& parameter: parameters) {
        parameter_names.push_back(parameter->get_node_name());
    }

    for (const auto& parameter_name: parameter_names) {
        auto v = RenameVisitor(parameter_name, "_l" + parameter_name);
        block.accept(v);
    }
}

void RenameFunctionArgumentsVisitor::visit_function_block(ast::FunctionBlock& block) {
    rename_arguments(block);
}

void RenameFunctionArgumentsVisitor::visit_procedure_block(ast::ProcedureBlock& block) {
    rename_arguments(block);
}

}  // namespace visitor
}  // namespace nmodl
