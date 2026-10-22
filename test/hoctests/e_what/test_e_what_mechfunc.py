# Density-mechanism FUNCTION/PROCEDURE args go through NPyMechFunc_call.
from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)


def test_nonfloatable_mech_arg():
    s = h.Section()
    s.insert("sdata")
    m = s(0.5).sdata
    m.A(1, 2, 3)
    assert m.a == 1.0
    expect_err("m.A(1j, 0, 0)")


if __name__ == "__main__":
    test_nonfloatable_mech_arg()
