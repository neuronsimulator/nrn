# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

import nmodl
from nmodl.dsl import ast, visitor
import pytest

class TestAst(object):
    def test_empty_program(self):
        pnode = ast.Program()
        assert str(pnode) == '{"Program":[]}'

