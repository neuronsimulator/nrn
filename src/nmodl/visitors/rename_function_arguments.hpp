#pragma once

#include "ast/function_block.hpp"
#include "ast/procedure_block.hpp"
#include "visitors/ast_visitor.hpp"

namespace nmodl {
namespace visitor {

class RenameFunctionArgumentsVisitor: public AstVisitor {
    template <class Block>
    void rename_arguments(Block& block) const;

    void visit_function_block(ast::FunctionBlock& block) override;
    void visit_procedure_block(ast::ProcedureBlock& block) override;
};

}  // namespace visitor
}  // namespace nmodl
