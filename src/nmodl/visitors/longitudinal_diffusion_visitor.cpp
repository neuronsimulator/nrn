#include "longitudinal_diffusion_visitor.hpp"

#include "ast/ast_decl.hpp"
#include "ast/kinetic_block.hpp"
#include "ast/longitudinal_diffusion_block.hpp"
#include "ast/name.hpp"
#include "ast/program.hpp"
#include "ast/statement.hpp"
#include "ast/statement_block.hpp"
#include "ast/string.hpp"
#include "visitor_utils.hpp"

#include <memory>


namespace nmodl {
namespace visitor {

static std::shared_ptr<ast::StatementBlock> make_statement_block(
    ast::KineticBlock& kinetic_block,
    nmodl::ast::AstNodeType node_type) {
    auto nodes = collect_nodes(kinetic_block, {node_type});

    ast::StatementVector statements;
    statements.reserve(nodes.size());
    for (auto& node: nodes) {
        statements.push_back(std::dynamic_pointer_cast<ast::Statement>(node));
    }

    return std::make_shared<ast::StatementBlock>(std::move(statements));
}


static std::shared_ptr<ast::LongitudinalDiffusionBlock> create_block(ast::KineticBlock& node) {
    return std::make_shared<ast::LongitudinalDiffusionBlock>(
        std::make_shared<ast::Name>(std::make_shared<ast::String>("ld_" + node.get_node_name())),
        make_statement_block(node, nmodl::ast::AstNodeType::LON_DIFFUSE),
        make_statement_block(node, nmodl::ast::AstNodeType::COMPARTMENT));
}

void CreateLongitudinalDiffusionBlocks::visit_program(ast::Program& node) {
    auto kinetic_blocks = collect_nodes(node, {nmodl::ast::AstNodeType::KINETIC_BLOCK});

    for (const auto& ast_node: kinetic_blocks) {
        auto kinetic_block = std::dynamic_pointer_cast<ast::KineticBlock>(ast_node);
        node.emplace_back_node(create_block(*kinetic_block));
    }
}

}  // namespace visitor
}  // namespace nmodl
