#include <utility>

#include "visitors/perf_visitor.hpp"

using namespace symtab;

PerfVisitor::PerfVisitor(const std::string& filename) : printer(new JSONPrinter(filename)) {
}

/// count math operations from all binary expressions
void PerfVisitor::visit_binary_expression(BinaryExpression* node) {
    bool assign_op = false;

    if (start_measurement) {
        switch (node->op.value) {
            case BOP_ADDITION:
                current_block_perf.add_count++;
                break;

            case BOP_SUBTRACTION:
                current_block_perf.sub_count++;
                break;

            case BOP_MULTIPLICATION:
                current_block_perf.mul_count++;
                break;

            case BOP_DIVISION:
                current_block_perf.div_count++;
                break;

            case BOP_POWER:
                current_block_perf.pow_count++;
                break;

            case BOP_AND:
                current_block_perf.and_count++;
                break;

            case BOP_OR:
                current_block_perf.or_count++;
                break;

            case BOP_GREATER:
                current_block_perf.gt_count++;
                break;

            case BOP_GREATER_EQUAL:
                current_block_perf.ge_count++;
                break;

            case BOP_LESS:
                current_block_perf.lt_count++;
                break;

            case BOP_LESS_EQUAL:
                current_block_perf.le_count++;
                break;

            case BOP_ASSIGN:
                current_block_perf.assign_count++;
                assign_op = true;
                break;

            case BOP_NOT_EQUAL:
                current_block_perf.ne_count++;
                break;

            case BOP_EXACT_EQUAL:
                current_block_perf.ee_count++;
                break;

            default:
                throw std::logic_error("Binary operator not handled in perf visitor");
        }
    }

    /// if visiting assignment expression, symbols from lhs
    /// are written and hence need flag to track
    if (assign_op) {
        visiting_lhs_expression = true;
    }

    node->lhs->accept(this);

    /// lhs is done (rhs is read only)
    visiting_lhs_expression = false;

    node->rhs->accept(this);
}

/// add performance stats to json printer
void PerfVisitor::add_perf_to_printer(PerfStat& perf) {
    auto keys = perf.keys();
    auto values = perf.values();
    assert(keys.size() == values.size());

    for (size_t i = 0; i < keys.size(); i++) {
        printer->add_node(values[i], keys[i]);
    }
}

/** Helper function used by all ast nodes : visit all children
 *  recursively and performance stats get added on stack. Once
 *  all children visited, we get total performance by summing
 *  perfstat of all children.
 */
void PerfVisitor::measure_performance(AST* node) {
    start_measurement = true;

    node->visit_children(this);

    PerfStat perf;
    while (!children_blocks_perf.empty()) {
        perf = perf + children_blocks_perf.top();
        children_blocks_perf.pop();
    }

    auto symtab = node->get_symbol_table();
    if (symtab == nullptr) {
        throw std::runtime_error("Symbol table not setup, Symtab visitor pass did not run?");
    }

    if (printer) {
        printer->push_block(symtab->name());
    }

    perf.title = "Performance Statistics of " + symtab->name();
    perf.print(stream);

    if (printer) {
        add_perf_to_printer(perf);
        printer->pop_block();
    }

    start_measurement = false;
}

/// count function calls and "most useful" or "commonly used" math functions
void PerfVisitor::visit_function_call(FunctionCall* node) {
    under_function_call = true;

    if (start_measurement) {
        auto name = node->name->get_name();
        if (name.compare("exp") == 0) {
            current_block_perf.exp_count++;
        } else if (name.compare("log") == 0) {
            current_block_perf.log_count++;
        } else if (name.compare("pow") == 0) {
            current_block_perf.pow_count++;
        }
        node->visit_children(this);
        current_block_perf.func_call_count++;
    }

    under_function_call = false;
}

/// every variable used is of type name, update counters
void PerfVisitor::visit_name(Name* node) {
    update_memory_ops(node->get_name());
    node->visit_children(this);
}

/// prime name derived from identifier and hence need to be handled here
void PerfVisitor::visit_prime_name(PrimeName* node) {
    update_memory_ops(node->get_name());
    node->visit_children(this);
}

void PerfVisitor::visit_if_statement(IfStatement* /*node*/) {
    if (start_measurement) {
        current_block_perf.if_count++;
    }
}

void PerfVisitor::visit_else_if_statement(ElseIfStatement* /*node*/) {
    if (start_measurement) {
        current_block_perf.elif_count++;
    }
}

void PerfVisitor::visit_program(Program* node) {
    if (printer) {
        printer->push_block("NMODL");
    }

    node->visit_children(this);
    std::string title = "Total Performance Statistics";
    total_perf.title = title;
    total_perf.print(stream);

    if (printer) {
        printer->push_block("total");
        add_perf_to_printer(total_perf);
        printer->pop_block();
        printer->pop_block();
    }
}

/** Blocks like function can have multiple statement blocks and
 * blocks like net receive has nested initial blocks. Hence need
 * to maintain separate stack.
 */
void PerfVisitor::visit_statement_block(StatementBlock* node) {
    /// starting new block, store current state
    blocks_perf.push(current_block_perf);

    current_symtab = node->get_symbol_table();

    if (current_symtab == nullptr) {
        throw std::runtime_error("Symbol table not setup, Symtab visitor pass did not run?");
    }

    /// new block perf starts from zero
    current_block_perf = PerfStat();

    node->visit_children(this);

    /// add performance of all visited children
    total_perf = total_perf + current_block_perf;

    children_blocks_perf.push(current_block_perf);

    // go back to parent block's state
    current_block_perf = blocks_perf.top();
    blocks_perf.pop();
}

/// solve is not a statement but could have associated block
/// and hence could/should not be skipped completely
void PerfVisitor::visit_solve_block(SolveBlock* node) {
    under_solve_block = true;
    node->visit_children(this);
    under_solve_block = false;
}

void PerfVisitor::visit_unary_expression(UnaryExpression* node) {
    if (start_measurement) {
        switch (node->op.value) {
            case UOP_NEGATION:
                current_block_perf.neg_count++;
                break;

            case UOP_NOT:
                current_block_perf.not_count++;
                break;

            default:
                throw std::logic_error("Unary operator not handled in perf visitor");
        }
    }
    node->visit_children(this);
}

/** Certain statements / symbols needs extra check while measuring
 * read/write operations. For example, for expression "exp(a+b)",
 * "exp" is an external math function and we should not increment read
 * count for "exp" symbol. Same for solve statement where name will
 * be neuron solver method.
 */
bool PerfVisitor::symbol_to_skip(const std::shared_ptr<Symbol>& symbol) {
    bool skip = false;

    auto is_method = symbol->has_properties(NmodlInfo::extern_method);
    if (is_method && under_function_call) {
        skip = true;
    }

    if (is_method && under_solve_block) {
        skip = true;
    }

    return skip;
}

bool PerfVisitor::is_local_variable(const std::shared_ptr<symtab::Symbol>& symbol) {
    bool is_local = false;
    auto properties = NmodlInfo::local_var | NmodlInfo::argument | NmodlInfo::function_block;
    if (symbol->has_properties(properties)) {
        is_local = true;
    }
    return is_local;
}

/** Find symbol in closest scope (up to parent) and update
 * read/write count. Also update ops count in current block.
 */
void PerfVisitor::update_memory_ops(const std::string& name) {
    if (start_measurement) {
        auto symbol = current_symtab->lookup_in_scope(name);
        if (symbol == nullptr || symbol_to_skip(symbol)) {
            return;
        }

        if (is_local_variable(symbol)) {
            if (visiting_lhs_expression) {
                symbol->write();
                current_block_perf.local_write_count++;
            } else {
                symbol->read();
                current_block_perf.local_read_count++;
            }
        } else {
            if (visiting_lhs_expression) {
                symbol->write();
                current_block_perf.global_write_count++;
            } else {
                symbol->read();
                current_block_perf.global_read_count++;
            }
        }
    }
}
