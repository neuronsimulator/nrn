from nmodl import ast, visitor, symtab
import nmodl
import pytest

def test_symtab(ch_ast):
    v = symtab.SymtabVisitor()
    v.visit_program(ch_ast)
    s = ch_ast.get_symbol_table()
    m = s.lookup('m')
    assert m is not None
    assert m.get_name() == "m"
    assert m.has_all_properties(symtab.NmodlType.state_var | symtab.NmodlType.prime_name) is True

    mInf = s.lookup('mInf')
    assert mInf is not None
    assert mInf.get_name() == "mInf"
    assert mInf.has_properties(symtab.NmodlType.range_var) is True
    assert mInf.has_properties(symtab.NmodlType.local_var) is False

    variables = s.get_variables_with_properties(symtab.NmodlType.range_var, True)
    assert len(variables) == 2
    assert str(variables[0]) == "mInf [Properties : range dependent_def]"
