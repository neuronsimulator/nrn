from nmodl import ast, visitor

import pytest

class TestAST(object):

    def test_empty_program(self):
        pnode = ast.Program()
        assert str(pnode) == '{"Program":[]}'
