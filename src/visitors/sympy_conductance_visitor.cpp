#include <algorithm>
#include <iostream>

#include "symtab/symbol.hpp"
#include "utils/logger.hpp"
#include "visitor_utils.hpp"
#include "visitors/sympy_conductance_visitor.hpp"

using namespace ast;
namespace py = pybind11;
using namespace py::literals;
using namespace syminfo;

// Generate statement strings to be added to BREAKPOINT section
std::vector<std::string> SympyConductanceVisitor::generate_statement_strings(
    BreakpointBlock* node) {
    std::vector<std::string> statements;
    // iterate over binary expressions from breakpoint
    for (const auto& expr: binary_exprs) {
        auto lhs_str = expr.first;
        auto equation_string = expr.second;
        // look for a current name that matches lhs of expr (current write name)
        auto it = i_name.find(lhs_str);
        if (it != i_name.end()) {
            std::string i_name_str = it->second;
            // differentiate dI/dV
            auto locals = py::dict("equation_string"_a = equation_string, "vars"_a = vars);
            py::exec(R"(
                            from nmodl.ode import differentiate2c
                            exception_message = ""
                            try:
                                rhs = equation_string.split("=")[1]
                                solution = differentiate2c(rhs, "v", vars)
                            except Exception as e:
                                # if we fail, fail silently and return empty string
                                solution = ""
                                exception_message = str(e)
                        )",
                     py::globals(), locals);
            auto dIdV = locals["solution"].cast<std::string>();
            auto exception_message = locals["exception_message"].cast<std::string>();
            if (!exception_message.empty()) {
                logger->warn("SympyConductance :: python exception: " + exception_message);
            }
            if (dIdV.empty()) {
                logger->warn(
                    "SympyConductance :: analytic differentiation of ionic current "
                    "not possible");
            } else {
                std::string g_var = dIdV;
                // if conductance g_var is not an existing variable, need to generate
                // a new variable name, declare it, and asign it
                if (vars.find(g_var) == vars.end()) {
                    // generate new variable name
                    std::map<std::string, int> var_map;
                    for (auto const& v: vars) {
                        var_map[v] = 0;
                    }
                    g_var = get_new_name("g", i_name_str, var_map);
                    // declare it
                    add_local_variable(node->get_statement_block().get(), g_var);
                    // asign dIdV to it
                    std::string statement_str = g_var + " = " + dIdV;
                    statements.insert(statements.begin(), statement_str);
                    logger->debug("SympyConductance :: Adding BREAKPOINT statement: " +
                                  statement_str);
                }
                std::string statement_str = "CONDUCTANCE " + g_var;
                if (i_name_str != "") {
                    statement_str += " USEION " + i_name_str;
                }
                statements.push_back(statement_str);
                logger->debug("SympyConductance :: Adding BREAKPOINT statement: " + statement_str);
            }
        }
    }
    return statements;
}

void SympyConductanceVisitor::visit_binary_expression(BinaryExpression* node) {
    // only want binary expressions of form x = ...
    if (node->lhs->is_var_name() && (node->op.get_value() == BinaryOp::BOP_ASSIGN)) {
        auto lhs_str = std::dynamic_pointer_cast<VarName>(node->lhs)->get_name()->get_node_name();
        binary_exprs[lhs_str] = nmodl::to_nmodl(node);
    }
}

void SympyConductanceVisitor::lookup_nonspecific_statements() {
    // add NONSPECIFIC_CURRENT statements to i_name map between write vars and names
    // note that they don't have an ion name, so we set it to ""
    if (!NONSPECIFIC_CONDUCTANCE_ALREADY_EXISTS) {
        for (auto ns_curr_ast: nonspecific_nodes) {
            logger->debug("SympyConductance :: Found NONSPECIFIC_CURRENT statement");
            for (auto write_name:
                 std::dynamic_pointer_cast<Nonspecific>(ns_curr_ast).get()->get_currents()) {
                std::string curr_write = write_name->get_node_name();
                logger->debug("SympyConductance :: -> Adding non-specific current write name: " +
                              curr_write);
                i_name[curr_write] = "";
            }
        }
    }
}

void SympyConductanceVisitor::lookup_useion_statements() {
    // add USEION statements to i_name map between write vars and names
    for (auto useion_ast: use_ion_nodes) {
        auto ion = std::dynamic_pointer_cast<Useion>(useion_ast).get();
        std::string ion_name = ion->get_node_name();
        logger->debug("SympyConductance :: Found USEION statement " + nmodl::to_nmodl(ion));
        if (i_ignore.find(ion_name) != i_ignore.end()) {
            logger->debug("SympyConductance :: -> Ignoring ion current name: " + ion_name);
        } else {
            auto wl = ion->get_writelist();
            for (auto w: wl) {
                std::string ion_write = w->get_node_name();
                logger->debug("SympyConductance :: -> Adding ion write name: " + ion_write +
                              " for ion current name: " + ion_name);
                i_name[ion_write] = ion_name;
            }
        }
    }
}

void SympyConductanceVisitor::visit_conductance_hint(ConductanceHint* node) {
    // find existing CONDUCTANCE statements - do not want to alter them
    // so keep a set of ion names i_ignore that we should ignore later
    logger->debug("SympyConductance :: Found existing CONDUCTANCE statement: " +
                  nmodl::to_nmodl(node));
    auto ion = node->get_ion();
    if (ion) {
        logger->debug("SympyConductance :: -> Ignoring ion current name: " + ion->get_node_name());
        i_ignore.insert(ion->get_node_name());
    } else {
        logger->debug("SympyConductance :: -> Ignoring all non-specific currents");
        NONSPECIFIC_CONDUCTANCE_ALREADY_EXISTS = true;
    }
};

void SympyConductanceVisitor::visit_breakpoint_block(BreakpointBlock* node) {
    // add any breakpoint local variables to vars
    if (auto symtab = node->get_statement_block()->get_symbol_table()) {
        for (auto localvar: symtab->get_variables_with_properties(NmodlType::local_var)) {
            vars.insert(localvar->get_name());
        }
    }

    // visit BREAKPOINT block statements
    node->visit_children(this);

    // lookup USEION and NONSPECIFIC statements from NEURON block
    lookup_useion_statements();
    lookup_nonspecific_statements();

    // add new CONDUCTANCE statements to BREAKPOINT
    auto new_statements = generate_statement_strings(node);
    if (!new_statements.empty()) {
        // get a copy of existing BREAKPOINT statements
        auto brkpnt_statements = node->get_statement_block()->get_statements();
        // insert new CONDUCTANCE statements at top of BREAKPOINT
        // or just below LOCAL statement if it exists
        auto insertion_point = brkpnt_statements.begin();
        while ((*insertion_point)->is_local_list_statement()) {
            ++insertion_point;
        }
        for (const auto& statement_str: new_statements) {
            insertion_point = brkpnt_statements.insert(insertion_point,
                                                       create_statement(statement_str));
        }
        // replace old set of BREAKPOINT statements in AST with new one
        node->get_statement_block()->set_statements(std::move(brkpnt_statements));
    }
}

void SympyConductanceVisitor::visit_program(Program* node) {
    vars = get_global_vars(node);

    AstLookupVisitor ast_lookup_visitor;
    use_ion_nodes = ast_lookup_visitor.lookup(node, AstNodeType::USEION);
    nonspecific_nodes = ast_lookup_visitor.lookup(node, AstNodeType::NONSPECIFIC);

    node->visit_children(this);
}
