import nmodl
from nmodl.dsl import ast, visitor
import pytest

class TestAST(object):
    def test_empty_program(self):
        pnode = ast.Program()
        assert str(pnode) == '{"Program":[]}'


def test_lookup_visitor(ch_ast):
    lookup_visitor = visitor.AstLookupVisitor()
    eqs = lookup_visitor.lookup(ch_ast, ast.AstNodeType.DIFF_EQ_EXPRESSION)
    eq_str = nmodl.dsl.to_nmodl(eqs[0])
    assert eq_str == "m' = mInf-m"


def test_lookup_visitor_constructor(ch_ast):
    lookup_visitor = visitor.AstLookupVisitor(ast.AstNodeType.DIFF_EQ_EXPRESSION)
    ch_ast.accept(lookup_visitor)
    eqs = lookup_visitor.get_nodes()
    eq_str = nmodl.dsl.to_nmodl(eqs[0])
    eq_json = nmodl.dsl.to_json(eqs[0], True)
    assert eq_str == "m' = mInf-m"
    assert '"DiffEqExpression":[{"BinaryExpression"' in eq_json


def test_custom_visitor(ch_ast):

    class StateVisitor(visitor.AstVisitor):
        def __init__(self):
            visitor.AstVisitor.__init__(self)
            self.in_state = False
            self.states = []

        def visit_state_block(self, node):
            self.in_state = True
            node.visit_children(self)
            self.in_state = False

        def visit_name(self, node):
            if self.in_state:
                self.states.append(nmodl.dsl.to_nmodl(node))

    myvisitor = StateVisitor()
    ch_ast.accept(myvisitor)

    assert len(myvisitor.states) is 2
    assert myvisitor.states[0] == "m"
    assert myvisitor.states[1] == "h"
