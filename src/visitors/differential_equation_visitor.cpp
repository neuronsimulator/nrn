#include "visitors/differential_equation_visitor.hpp"
#include <iostream>

using namespace ast;

void DifferentialEquationVisitor::visit_diff_eq_expression(DiffEqExpression* node) {
    equations.push_back(node);
}
