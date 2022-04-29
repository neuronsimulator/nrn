# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

import nmodl
from nmodl.dsl import ast, visitor
import pytest


def test_lookup_visitor(ch_ast):
    lookup_visitor = visitor.AstLookupVisitor()
    eqs = lookup_visitor.lookup(ch_ast, ast.AstNodeType.DIFF_EQ_EXPRESSION)
    eq_str = nmodl.dsl.to_nmodl(eqs[0])
    assert eq_str == "m' = mInf-m"


def test_lookup_visitor_any_node():
    """Ensure the AstLookupVisitor.lookup methods accept any node"""
    lookup_visitor = visitor.AstLookupVisitor(ast.AstNodeType.INTEGER)
    int42 = ast.Integer(42, None)

    eqs = lookup_visitor.lookup(int42)
    assert len(eqs) == 1

    eqs = lookup_visitor.lookup(int42, ast.AstNodeType.DOUBLE)
    assert len(eqs) == 0


def test_lookup_visitor_constructor(ch_ast):
    lookup_visitor = visitor.AstLookupVisitor(ast.AstNodeType.DIFF_EQ_EXPRESSION)
    eqs = lookup_visitor.lookup(ch_ast)
    eq_str = nmodl.dsl.to_nmodl(eqs[0])


def test_json_visitor(ch_ast):
    lookup_visitor = visitor.AstLookupVisitor(ast.AstNodeType.PRIME_NAME)
    primes = lookup_visitor.lookup(ch_ast)

    # test compact json
    prime_str = nmodl.dsl.to_nmodl(primes[0])
    prime_json = nmodl.dsl.to_json(primes[0], True)
    assert prime_json == '{"PrimeName":[{"String":[{"name":"m"}]},{"Integer":[{"name":"1"}]}]}'

    # test json with expanded keys
    result_json = nmodl.dsl.to_json(primes[0], compact=True, expand=True)
    expected_json = ('{"children":[{"children":[{"name":"m"}],'
                   '"name":"String"},{"children":[{"name":"1"}],'
                   '"name":"Integer"}],"name":"PrimeName"}')
    assert result_json == expected_json

    # test json with nmodl embedded
    result_json = nmodl.dsl.to_json(primes[0], compact=True, expand=True, add_nmodl=True)
    expected_json = ('{"children":[{"children":[{"name":"m"}],"name":"String","nmodl":"m"},'
                     '{"children":[{"name":"1"}],"name":"Integer","nmodl":"1"}],'
                     '"name":"PrimeName","nmodl":"m\'"}')
    assert result_json == expected_json


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


def test_modify_ast():
    one_var = """NEURON {
    SUFFIX test
    RANGE x
}
    """
    class ModifyVisitor(visitor.AstVisitor):
        def __init__(self, old_name, new_name):
            visitor.AstVisitor.__init__(self)
            self.old_name = old_name
            self.new_name = new_name

        def visit_range_var(self, node):
            if nmodl.to_nmodl(node.name) == self.old_name:
                node.name.value = ast.String(self.new_name)
            node.visit_children(self)

    driver = nmodl.NmodlDriver()
    modast = driver.parse_string(one_var)
    mod_visitor = ModifyVisitor("x", "y")
    mod_visitor.visit_program(modast)
    one_var_after = """NEURON {
    SUFFIX test
    RANGE y
}
"""
    assert str(modast) == one_var_after
