/*
 * Copyright 2023 Blue Brain Project, EPFL.
 * See the top-level LICENSE file for details.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "kinetic_block_visitor.hpp"

#include "ast/all.hpp"
#include "constant_folder_visitor.hpp"
#include "index_remover.hpp"
#include "loop_unroll_visitor.hpp"
#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "utils/string_utils.hpp"
#include "visitor_utils.hpp"

#include <regex>


namespace nmodl {
namespace visitor {

using symtab::syminfo::NmodlType;

void KineticBlockVisitor::process_reac_var(const std::string& varname, int count) {
    // lookup index of state var
    const auto it = state_var_index.find(varname);
    if (it == state_var_index.cend()) {
        // not a state var
        // so this is a constant variable in the reaction statement
        // this should be included in the fluxes:
        if (in_reaction_statement_lhs) {
            non_state_var_fflux[i_statement] = varname;
            logger->debug("KineticBlockVisitor :: adding non-state fflux[{}] \"{}\"",
                          i_statement,
                          varname);
        } else {
            non_state_var_bflux[i_statement] = varname;
            logger->debug("KineticBlockVisitor :: adding non-state bflux[{}] \"{}\"",
                          i_statement,
                          varname);
        }
        // but as it is not a state var, no ODE should be generated for it, i.e. no nu_L or nu_R
        // entry.
    } else {
        // found state var index
        int i_statevar = it->second;
        if (in_reaction_statement_lhs) {
            // set element of nu_L matrix
            rate_eqs.nu_L[i_statement][i_statevar] += count;
            logger->debug("KineticBlockVisitor :: nu_L[{}][{}] += {}",
                          i_statement,
                          i_statevar,
                          count);
        } else {
            // set element of nu_R matrix
            rate_eqs.nu_R[i_statement][i_statevar] += count;
            logger->debug("KineticBlockVisitor :: nu_R[{}][{}] += {}",
                          i_statement,
                          i_statevar,
                          count);
        }
    }
}

void KineticBlockVisitor::process_conserve_reac_var(const std::string& varname, int count) {
    // subtract previous term from both sides of equation
    if (!conserve_equation_statevar.empty()) {
        if (conserve_equation_factor.empty()) {
            conserve_equation_str += " - " + conserve_equation_statevar;

        } else {
            conserve_equation_str += " - (" + conserve_equation_factor + " * " +
                                     conserve_equation_statevar + ")";
        }
    }
    // construct new term
    auto compartment_factor = compartment_factors[state_var_index[varname]];
    if (compartment_factor.empty()) {
        if (count == 1) {
            conserve_equation_factor = "";
        } else {
            conserve_equation_factor = std::to_string(count);
        }
    } else {
        conserve_equation_factor = compartment_factor + "*" + std::to_string(count);
    }
    // if new term is not a state var raise error
    if (state_var_index.find(varname) == state_var_index.cend()) {
        logger->error(
            "KineticBlockVisitor :: Error : CONSERVE statement should only contain state vars "
            "on LHS, but found {}",
            varname);
    } else {
        conserve_equation_statevar = varname;
    }
}

std::shared_ptr<ast::Expression> create_expr(const std::string& str_expr) {
    auto statement = create_statement("dummy = " + str_expr);
    auto expr = std::dynamic_pointer_cast<ast::ExpressionStatement>(statement)->get_expression();
    return std::dynamic_pointer_cast<ast::BinaryExpression>(expr)->get_rhs();
}

void KineticBlockVisitor::visit_conserve(ast::Conserve& node) {
    ++conserve_statement_count;
    // rewrite CONSERVE statement in form x = ...
    // where x was the last state var on LHS, and whose ODE should later be replaced with this
    // equation note: CONSERVE statement "implicitly takes into account COMPARTMENT factors on LHS"
    // this means that each state var on LHS must be multiplied by its compartment factor
    // the RHS is just an expression, no compartment factors are taken into account
    // see p244 of NEURON book
    logger->debug("KineticBlockVisitor :: CONSERVE statement: {}", to_nmodl(node));
    conserve_equation_str = "";
    conserve_equation_statevar = "";
    conserve_equation_factor = "";

    in_conserve_statement = true;
    // construct equation to replace ODE in conserve_equation_str
    node.visit_children(*this);
    in_conserve_statement = false;

    conserve_equation_str = to_nmodl(node.get_expr()) + conserve_equation_str;
    if (!conserve_equation_factor.empty()) {
        // divide by compartment factor of conserve_equation_statevar
        conserve_equation_str = "(" + conserve_equation_str + ")/(" + conserve_equation_factor +
                                ")";
    }

    // note: The following 4 lines result in a still valid (and equivalent) CONSERVE statement.
    // later this block will become a DERIVATIVE block where it is no longer valid
    // to have a CONSERVE statement. Parsing the equivalent nmodl in between the
    // kinetic visitor and the sympysolvervisitor in presence of a conserve statement
    // should result in an error since we do not want to add new functionalities to the language.
    // the SympySolver will use to it replace the ODE (to replicate what neuron does)
    auto statement = create_statement("CONSERVE " + conserve_equation_statevar + " = " +
                                      conserve_equation_str);
    auto expr = std::dynamic_pointer_cast<ast::Conserve>(statement);
    // set react (lhs) of CONSERVE to the state variable whose ODE should be replaced
    node.set_react(expr->get_react());
    // set expr (rhs) of CONSERVE to the equation that should replace the ODE
    node.set_expr(expr->get_expr());

    logger->debug("KineticBlockVisitor :: --> {}", to_nmodl(node));
}

void KineticBlockVisitor::set_compartment_factor(int var_index, const std::string& factor) {
    if (compartment_factors[var_index] != "") {
        throw std::runtime_error("Setting compartment volume twice.");
    }

    compartment_factors[var_index] = factor;
    logger->debug("KineticBlockVisitor :: COMPARTMENT factor {} for state var {} (index {})",
                  factor,
                  state_var[var_index],
                  var_index);
}

void KineticBlockVisitor::compute_compartment_factor(ast::Compartment& node,
                                                     const ast::Name& name) {
    const auto& var_name = name.get_node_name();
    const auto it = state_var_index.find(var_name);
    if (it != state_var_index.cend()) {
        int var_index = it->second;
        auto expr = node.get_volume();
        std::string expression = to_nmodl(expr);

        set_compartment_factor(var_index, expression);
    } else {
        logger->debug(
            "KineticBlockVisitor :: COMPARTMENT specified volume for non-state variable {}",
            var_name);
    }
}

void KineticBlockVisitor::compute_indexed_compartment_factor(ast::Compartment& node,
                                                             const ast::Name& name) {
    auto array_var_name = name.get_node_name();
    auto index_name = node.get_index_name()->get_node_name();

    auto pattern = fmt::format("^{}\\[([0-9]*)\\]$", array_var_name);
    std::regex re(pattern);
    std::smatch m;

    for (size_t var_index = 0; var_index < state_var.size(); ++var_index) {
        auto matches = std::regex_match(state_var[var_index], m, re);

        if (matches) {
            int index_value = std::stoi(m[1]);
            auto volume_expr = node.get_volume();
            auto expr = std::shared_ptr<ast::Expression>(volume_expr->clone());
            IndexRemover(index_name, index_value).visit_expression(*expr);

            std::string expression = to_nmodl(*expr);
            set_compartment_factor(var_index, expression);
        }
    }
}

void KineticBlockVisitor::visit_compartment(ast::Compartment& node) {
    // COMPARTMENT block has an expression, and a list of state vars it applies to.
    // For each state var, the rhs of the differential eq should be divided by the expression.
    // Here we store the expressions in the compartment_factors vector
    logger->debug("KineticBlockVisitor :: COMPARTMENT expr: {}", to_nmodl(node.get_volume()));
    for (const auto& name_ptr: node.get_species()) {
        if (node.get_index_name() == nullptr) {
            compute_compartment_factor(node, *name_ptr);
        } else {
            compute_indexed_compartment_factor(node, *name_ptr);
        }
    }
    // add COMPARTMENT state to list of statements to remove
    // since we don't want this statement to be present in the final DERIVATIVE block
    statements_to_remove.insert(&node);
}

void KineticBlockVisitor::visit_reaction_operator(ast::ReactionOperator& node) {
    auto reaction_op = node.get_value();
    if (reaction_op == ast::ReactionOp::LTMINUSGT) {
        // <->
        // reversible reaction
        // we go from visiting the lhs to visiting the rhs of the reaction statement
        in_reaction_statement_lhs = false;
    }
}

void KineticBlockVisitor::visit_react_var_name(
    ast::ReactVarName& node) {  // NOLINT(readability-function-cognitive-complexity)
    // ReactVarName node contains a VarName and an Integer
    // the VarName is the state variable which we convert to an index
    // the Integer is the value to be added to the stoichiometric matrix at this index
    auto varname = to_nmodl(node.get_name());
    int count = node.get_value() ? node.get_value()->eval() : 1;
    if (in_reaction_statement) {
        process_reac_var(varname, count);
    } else if (in_conserve_statement) {
        if (array_state_var_size.find(varname) != array_state_var_size.cend()) {
            // state var is an array: need to sum over each element
            for (int i = 0; i < array_state_var_size[varname]; ++i) {
                process_conserve_reac_var(varname + "[" + std::to_string(i) + "]", count);
            }
        } else {
            process_conserve_reac_var(varname, count);
        }
    }
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
void KineticBlockVisitor::visit_reaction_statement(ast::ReactionStatement& node) {
    statements_to_remove.insert(&node);

    auto reaction_op = node.get_op().get_value();
    // special case for << statements
    if (reaction_op == ast::ReactionOp::LTLT) {
        logger->debug("KineticBlockVisitor :: '<<' reaction statement: {}", to_nmodl(node));
        // statements involving the "<<" operator
        // must have a single state var on lhs
        // and a single expression on rhs that corresponds to d{state var}/dt
        // So if x is a state var, then
        // ~ x << (a*b)
        // translates to the ODE contribution x' += a*b
        const auto& lhs = node.get_reaction1();

        /// check if reaction statement is a single state variable
        bool single_state_var = true;
        if (lhs->is_react_var_name()) {
            auto value = std::dynamic_pointer_cast<ast::ReactVarName>(lhs)->get_value();
            if (value && (value->eval() != 1)) {
                single_state_var = false;
            }
        }
        if (!lhs->is_react_var_name() || !single_state_var) {
            throw std::runtime_error(fmt::format(
                "KineticBlockVisitor :: LHS of \"<<\" reaction statement must be a single state "
                "var, but instead found {}: ignoring this statement",
                to_nmodl(lhs)));
        }
        const auto& rhs = node.get_expression1();
        std::string varname = to_nmodl(lhs);
        // get index of state var
        const auto it = state_var_index.find(varname);
        if (it != state_var_index.cend()) {
            int var_index = it->second;
            std::string expr = to_nmodl(rhs);
            if (!additive_terms[var_index].empty()) {
                additive_terms[var_index] += " + ";
            }
            // add to additive terms for this state var
            additive_terms[var_index] += fmt::format("({})", expr);
            logger->debug("KineticBlockVisitor :: '<<' reaction statement: {}' += {}",
                          varname,
                          expr);
        }
        return;
    }

    // forwards reaction rate
    const auto& kf = node.get_expression1();
    // backwards reaction rate
    const auto& kb = node.get_expression2();

    // add reaction rates to vectors kf, kb
    auto kf_str = to_nmodl(kf);
    logger->debug("KineticBlockVisitor :: k_f[{}] = {}", i_statement, kf_str);
    rate_eqs.k_f.emplace_back(kf_str);

    if (kb) {
        // kf is always defined, but for statements with operator "->" kb is not
        auto kb_str = to_nmodl(kb);
        logger->debug("KineticBlockVisitor :: k_b[{}] = {}", i_statement, kb_str);
        rate_eqs.k_b.emplace_back(kb_str);
    } else {
        rate_eqs.k_b.emplace_back();
    }

    // add empty non state var fluxes for this statement
    non_state_var_fflux.emplace_back();
    non_state_var_bflux.emplace_back();

    // add a row of zeros to the stoichiometric matrices
    rate_eqs.nu_L.emplace_back(std::vector<int>(state_var_count, 0));
    rate_eqs.nu_R.emplace_back(std::vector<int>(state_var_count, 0));

    // visit each term in reaction statement and
    // add the corresponding integer to the new row in the matrix
    in_reaction_statement = true;
    in_reaction_statement_lhs = true;
    node.visit_children(*this);
    in_reaction_statement = false;

    // generate fluxes
    modfile_fflux = rate_eqs.k_f.back();
    modfile_bflux = rate_eqs.k_b.back();

    // contribution from state vars
    for (int j = 0; j < state_var_count; ++j) {
        std::string multiply_var = std::string("*").append(state_var[j]);
        int nu_L = rate_eqs.nu_L[i_statement][j];
        while (nu_L-- > 0) {
            modfile_fflux += multiply_var;
        }
        int nu_R = rate_eqs.nu_R[i_statement][j];
        while (nu_R-- > 0) {
            modfile_bflux += multiply_var;
        }
    }
    // contribution from non-state vars
    if (!non_state_var_fflux[i_statement].empty()) {
        modfile_fflux += std::string("*").append(non_state_var_fflux[i_statement]);
    }
    if (!non_state_var_bflux[i_statement].empty()) {
        modfile_bflux += std::string("*").append(non_state_var_bflux[i_statement]);
    }
    fflux.emplace_back(modfile_fflux);
    bflux.emplace_back(modfile_bflux);

    // for substituting into modfile, empty flux should be 0
    if (modfile_fflux.empty()) {
        modfile_fflux = "0";
    }
    if (modfile_bflux.empty()) {
        modfile_bflux = "0";
    }

    logger->debug("KineticBlockVisitor :: fflux[{}] = {}", i_statement, fflux[i_statement]);
    logger->debug("KineticBlockVisitor :: bflux[{}] = {}", i_statement, bflux[i_statement]);

    // increment statement counter
    ++i_statement;
}

void KineticBlockVisitor::visit_wrapped_expression(ast::WrappedExpression& node) {
    // If a wrapped expression contains a variable with name "f_flux" or "b_flux",
    // this variable should be replaced by the expression for the corresponding flux
    // which depends on the previous reaction statement. The current expressions are
    // stored as strings in "modfile_fflux" and "modfile_bflux"
    if (node.get_expression()->is_name()) {
        auto var_name = std::dynamic_pointer_cast<ast::Name>(node.get_expression());
        if (var_name->get_node_name() == "f_flux") {
            auto expr = create_expr(modfile_fflux);
            logger->debug("KineticBlockVisitor :: replacing f_flux with {}", to_nmodl(expr));
            node.set_expression(std::move(expr));
        } else if (var_name->get_node_name() == "b_flux") {
            auto expr = create_expr(modfile_bflux);
            logger->debug("KineticBlockVisitor :: replacing b_flux with {}", to_nmodl(expr));
            node.set_expression(std::move(expr));
        }
    }
    node.visit_children(*this);
}

void KineticBlockVisitor::visit_statement_block(ast::StatementBlock& node) {
    auto prev_statement_block = current_statement_block;
    current_statement_block = &node;
    node.visit_children(*this);
    // remove processed statements from current statement block
    current_statement_block->erase_statement(statements_to_remove);
    current_statement_block = prev_statement_block;
}

void KineticBlockVisitor::visit_kinetic_block(ast::KineticBlock& node) {
    rate_eqs.nu_L.clear();
    rate_eqs.nu_R.clear();
    rate_eqs.k_f.clear();
    rate_eqs.k_b.clear();
    fflux.clear();
    bflux.clear();
    odes.clear();
    modfile_fflux = "0";
    modfile_bflux = "0";

    // allocate these vectors with empty strings
    compartment_factors = std::vector<std::string>(state_var_count);
    additive_terms = std::vector<std::string>(state_var_count);
    i_statement = 0;

    // construct stochiometric matrices and fluxes
    node.visit_children(*this);

    // number of reaction statements
    int Ni = static_cast<int>(rate_eqs.k_f.size());

    // number of ODEs (= number of state vars)
    int Nj = state_var_count;

    // generate ODEs
    for (int j = 0; j < Nj; ++j) {
        // rhs of ODE eq
        std::string ode_rhs = additive_terms[j];
        for (int i = 0; i < Ni; ++i) {
            int delta_nu = rate_eqs.nu_R[i][j] - rate_eqs.nu_L[i][j];
            if (delta_nu != 0) {
                // if not the first RHS term, add + sign first
                if (!ode_rhs.empty()) {
                    ode_rhs += " + ";
                }
                if (bflux[i].empty()) {
                    ode_rhs += fmt::format("({}*({}))", delta_nu, fflux[i]);
                } else if (fflux[i].empty()) {
                    ode_rhs += fmt::format("({}*(-{}))", delta_nu, bflux[i]);
                } else {
                    ode_rhs += fmt::format("({}*({}-{}))", delta_nu, fflux[i], bflux[i]);
                }
            }
        }
        // divide by COMPARTMENT factor if present
        if (!compartment_factors[j].empty() && !ode_rhs.empty()) {
            ode_rhs = fmt::format("({})/({})", ode_rhs, compartment_factors[j]);
        }
        // if rhs of ODE is not empty, add to list of ODEs
        if (!ode_rhs.empty()) {
            auto state_var_split = stringutils::split_string(state_var[j], '[');
            std::string var_str = state_var_split[0];
            std::string index_str;
            if (state_var_split.size() > 1) {
                index_str = "[" + state_var_split[1];
            }
            odes.push_back(fmt::format("{}'{} = {}", var_str, index_str, ode_rhs));
        }
    }

    for (const auto& ode: odes) {
        logger->debug("KineticBlockVisitor :: ode : {}", ode);
    }

    const auto& kinetic_statement_block = node.get_statement_block();
    // remove any remaining kinetic statements
    kinetic_statement_block->erase_statement(statements_to_remove);
    // add new statements
    for (const auto& ode: odes) {
        logger->debug("KineticBlockVisitor :: -> adding statement: {}", ode);
        kinetic_statement_block->emplace_back_statement(create_statement(ode));
    }

    // store pointer to kinetic block
    kinetic_blocks.push_back(&node);
}

class LocalRateNames {
    static constexpr auto kf_stem = "kf";
    static constexpr auto kb_stem = "kb";
    static constexpr auto source_stem = "source";

    std::shared_ptr<ast::Name> generate_local_name(const std::string& stem) {
        std::string unmangled_name = fmt::format("{}{}_", stem, n_equations);
        std::string mangled_name = unmangled_name;

        size_t mangle_attempt = 0;
        while (symtab->lookup_in_scope(mangled_name)) {
            mangled_name = fmt::format("{}{:04d}", unmangled_name, mangle_attempt++);

            if (mangle_attempt >= 10000) {
                throw std::runtime_error("Failed to find unique local name.");
            }
        }

        auto name = std::make_shared<ast::Name>(std::make_shared<ast::String>(mangled_name));
        local_names.push_back(mangled_name);

        return name;
    }

  public:
    LocalRateNames() = default;
    LocalRateNames(const LocalRateNames&) = default;
    LocalRateNames(LocalRateNames&&) = default;

    LocalRateNames(symtab::SymbolTable const* symtab)
        : symtab(symtab) {}

    LocalRateNames& operator=(const LocalRateNames&) = default;
    LocalRateNames& operator=(LocalRateNames&&) = default;

    std::shared_ptr<ast::Name> generate_forward_rate_name() {
        auto kf = generate_local_name(kf_stem);
        ++n_equations;

        return kf;
    }

    std::pair<std::shared_ptr<ast::Name>, std::shared_ptr<ast::Name>> generate_rate_names() {
        auto kf = generate_local_name(kf_stem);
        auto kb = generate_local_name(kb_stem);
        ++n_equations;

        return {kf, kb};
    }

    std::shared_ptr<ast::Name> generate_source_name() {
        auto source = generate_local_name(source_stem);
        ++n_equations;

        return source;
    }

    std::vector<std::string> get_local_variable_names() {
        return local_names;
    }

  private:
    size_t n_equations = 0;
    std::vector<std::string> local_names;
    symtab::SymbolTable const* symtab = nullptr;
};

class LocalizeKineticRatesVisitor: public AstVisitor {
  public:
    LocalRateNames local_names;

  private:
    std::shared_ptr<ast::ExpressionStatement> localize_expression(
        const std::shared_ptr<ast::Name>& name,
        const std::shared_ptr<ast::Expression>& expression) {
        auto local = std::make_shared<ast::BinaryExpression>(name,
                                                             ast::BinaryOperator(ast::BOP_ASSIGN),
                                                             expression);
        return std::make_shared<ast::ExpressionStatement>(local);
    }

    template <class Node>
    std::shared_ptr<Node> clone(const Node& node) {
        return std::shared_ptr<Node>(node.clone());
    }

  public:
    void visit_kinetic_block(ast::KineticBlock& node) {
        auto stmt_block = node.get_statement_block();
        local_names = LocalRateNames(stmt_block->get_symbol_table());

        // process the statement block first.
        node.visit_children(*this);

        // We now know the names of the created LOCAL variables. If a LOCAL
        // block exists, append to that, otherwise create a new one.
        auto locals = local_names.get_local_variable_names();

        for (const auto& local: locals) {
            add_local_variable(*stmt_block, local);
        }
    }


    void visit_statement_block(ast::StatementBlock& node) {
        // process any nested statement blocks first:
        node.visit_children(*this);

        // process all reaction equations:
        const auto& statements = node.get_statements();
        for (auto iter = statements.begin(); iter != statements.end(); ++iter) {
            if ((*iter)->is_reaction_statement()) {
                auto reaction_equation = std::dynamic_pointer_cast<ast::ReactionStatement>(*iter);
                auto op = reaction_equation->get_op();

                ast::StatementVector localized_statements;

                auto local_expr1 = reaction_equation->get_expression1();
                auto local_expr2 = reaction_equation->get_expression2();

                std::shared_ptr<ast::Name> expr1_name = nullptr;
                std::shared_ptr<ast::Name> expr2_name = nullptr;

                if (op.get_value() == ast::LTLT) {
                    expr1_name = local_names.generate_source_name();
                } else if (op.get_value() == ast::LTMINUSGT) {
                    std::tie(expr1_name, expr2_name) = local_names.generate_rate_names();
                } else if (op.get_value() == ast::MINUSGT) {
                    expr1_name = local_names.generate_forward_rate_name();
                }

                if (local_expr1) {
                    auto assignment = localize_expression(clone(*expr1_name),
                                                          reaction_equation->get_expression1());
                    localized_statements.push_back(assignment);
                    local_expr1 = clone(*expr1_name);
                }

                if (local_expr2) {
                    auto assignment = localize_expression(clone(*expr2_name),
                                                          reaction_equation->get_expression2());
                    localized_statements.push_back(assignment);
                    local_expr2 = clone(*expr2_name);
                }

                auto local_reaction =
                    std::make_shared<ast::ReactionStatement>(reaction_equation->get_reaction1(),
                                                             reaction_equation->get_op(),
                                                             reaction_equation->get_reaction2(),
                                                             local_expr1,
                                                             local_expr2);
                localized_statements.push_back(local_reaction);

                iter = node.erase_statement(iter);
                for (const auto& stmt: localized_statements) {
                    // `iter` points to the element after the one
                    // we've inserted.
                    iter = ++node.insert_statement(iter, stmt);
                }
                --iter;
            }
        }
    }
};


void KineticBlockVisitor::unroll_kinetic_blocks(ast::Program& node) {
    const auto& kineticBlockNodes = collect_nodes(node, {ast::AstNodeType::KINETIC_BLOCK});

    // Before: FROM i = 0 TO N-1
    // After:  FROM i = 0 TO 2
    visitor::ConstantFolderVisitor const_folder;
    for (const auto& ii: kineticBlockNodes) {
        ii->accept(const_folder);
    }

    // Before:
    // FROM i = 0 TO 2 {
    //   ~ ca[i] <-> ca[i+1] (a, b)
    // }
    //
    // After:
    // ~ ca[0] <-> ca[0+1] (a, b)
    // ~ ca[1] <-> ca[1+1] (a, b)
    // ~ ca[2] <-> ca[2+1] (a, b)
    //
    visitor::LoopUnrollVisitor unroller;
    for (const auto& ii: kineticBlockNodes) {
        ii->accept(unroller);
    }

    // Before: ca[0+1]
    // After:  ca[1]
    for (const auto& ii: kineticBlockNodes) {
        ii->accept(const_folder);
    }

    auto visitor = LocalizeKineticRatesVisitor{};
    node.accept(visitor);
}

void KineticBlockVisitor::visit_program(ast::Program& node) {
    unroll_kinetic_blocks(node);

    conserve_statement_count = 0;
    statements_to_remove.clear();
    current_statement_block = nullptr;

    // get state variables - assign an index to each
    state_var_index.clear();
    array_state_var_size.clear();
    state_var.clear();
    state_var_count = 0;
    if (auto symtab = node.get_symbol_table()) {
        auto statevars = symtab->get_variables_with_properties(NmodlType::state_var);
        for (const auto& v: statevars) {
            std::string var_name = v->get_name();
            if (v->is_array()) {
                // CONSERVE statement needs to know this is an array state var, and its size:
                array_state_var_size[var_name] = v->get_length();
                // for array state vars we need to add each element of the array separately
                var_name += "[";
                for (int i = 0; i < v->get_length(); ++i) {
                    std::string var_name_i = var_name + std::to_string(i) + "]";
                    logger->debug("KineticBlockVisitor :: state_var_index[{}] = {}",
                                  var_name_i,
                                  state_var_count);
                    state_var_index[var_name_i] = state_var_count++;
                    state_var.push_back(var_name_i);
                }
            } else {
                logger->debug("KineticBlockVisitor :: state_var_index[{}] = {}",
                              var_name,
                              state_var_count);
                state_var_index[var_name] = state_var_count++;
                state_var.push_back(var_name);
            }
        }
    }

    const auto& kineticBlockNodes = collect_nodes(node, {ast::AstNodeType::KINETIC_BLOCK});
    // replace reaction statements within each kinetic block with equivalent ODEs
    for (const auto& ii: kineticBlockNodes) {
        ii->accept(*this);
    }

    // change KINETIC blocks -> DERIVATIVE blocks
    auto blocks = node.get_blocks();
    for (auto* kinetic_block: kinetic_blocks) {
        for (auto it = blocks.begin(); it != blocks.end(); ++it) {
            if (it->get() == kinetic_block) {
                auto dblock =
                    std::make_shared<ast::DerivativeBlock>(kinetic_block->get_name(),
                                                           kinetic_block->get_statement_block());
                ModToken tok{};
                dblock->set_token(tok);
                *it = dblock;
            }
        }
    }
    node.set_blocks(std::move(blocks));
}

}  // namespace visitor
}  // namespace nmodl
