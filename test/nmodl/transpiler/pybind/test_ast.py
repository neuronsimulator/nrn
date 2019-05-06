# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

from nmodl.dsl import ast
import nmodl.dsl as nmodl
import pytest

class TestAst(object):
    def test_empty_program(self):
        pnode = ast.Program()
        assert str(pnode) == '{"Program":[]}'

    def test_ast_construction(self):
        string = ast.String("tau")
        name = ast.Name(string)
        assert nmodl.to_nmodl(name) == 'tau'

        int_macro = nmodl.ast.Integer(1, ast.Name(ast.String("x")))
        assert nmodl.to_nmodl(int_macro) == 'x'

        statements = []
        block = ast.StatementBlock(statements)
        neuron_block = ast.NeuronBlock(block)
        assert nmodl.to_nmodl(neuron_block) == 'NEURON {\n}'
