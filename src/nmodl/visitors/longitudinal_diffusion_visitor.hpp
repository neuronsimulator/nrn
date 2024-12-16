#pragma once

#include "visitors/ast_visitor.hpp"


namespace nmodl {
namespace ast {
class Program;
}

namespace visitor {

class CreateLongitudinalDiffusionBlocks: public AstVisitor {
  public:

    void visit_program(ast::Program& node) override;
};

}  // namespace visitor
}  // namespace nmodl
