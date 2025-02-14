# Copyright 2023 Blue Brain Project, EPFL.
# See the top-level LICENSE file for details.
#
# SPDX-License-Identifier: Apache-2.0

from neuron.nmodl.dsl import ast, to_json, to_nmodl


class TestAst:
    def test_empty_program(self):
        pnode = ast.Program()
        assert str(pnode) == ""

    def test_ast_construction(self):
        string = ast.String("tau")
        name = ast.Name(string)
        assert to_nmodl(name) == "tau"

        int_macro = ast.Integer(1, ast.Name(ast.String("x")))
        assert to_nmodl(int_macro) == "x"

        statements = []
        block = ast.StatementBlock(statements)
        neuron_block = ast.NeuronBlock(block)
        assert to_nmodl(neuron_block) == "NEURON {\n}"

    def test_get_parent(self):
        x_name = ast.Name(ast.String("x"))
        int_macro = ast.Integer(1, x_name)
        assert x_name.parent == int_macro  # getting the parent

    def test_set_parent(self):
        x_name = ast.Name(ast.String("x"))
        y_name = ast.Name(ast.String("y"))
        int_macro = ast.Integer(1, x_name)
        y_name.parent = int_macro  # setting the parent
        int_macro.macro = y_name
        assert to_nmodl(int_macro) == "y"

    def test_ast_node_repr(self):
        string = ast.String("tau")
        name = ast.Name(string)
        assert repr(name) == to_json(name, compact=True)

    def test_ast_node_str(self):
        string = ast.String("tau")
        name = ast.Name(string)
        assert str(name) == to_nmodl(name)
