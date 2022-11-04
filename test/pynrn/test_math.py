# tests for floating exceptions (using tstmath.mod)

from neuron import h
from neuron.expect_hocerr import expect_err, set_quiet

set_quiet(False)
import math


def test_math1():  # without h.nrn_feenableexcept(1)
    assert math.isinf(h.div_tstmath(1, 0))
    assert math.isinf(h.expon_tstmath(1000))
    expect_err("h.sqrt(-1)")  # argument out of domain
    expect_err("math.exp(1000.)")  # OverflowError: math range error
    assert math.isnan(h.root2_tstmath(-1))
    assert math.isnan(h.natlog_tstmath(-1))
    assert math.isnan(h.Vector([-1.0]).log()[0])


def test_math2():  # with h.nrn_feenableexcept(1)
    if h.unix_mac_pc() == 3:  # windows does not have it
        assert h.nrn_feenableexcept(0) == -2.0
        return
    enter_val = h.nrn_feenableexcept(0)
    assert enter_val >= 0
    old0 = h.nrn_feenableexcept(1)
    old1 = h.nrn_feenableexcept(1)
    print("enter_val old0 old1 ", enter_val, old0, old1)

    h.nrn_feenableexcept(1)
    # calls clear_fe_except in src/oc/math.cpp, no more perpetual Divide by zero
    h.sqrt(1)
    expect_err("print(h.sqrt(-1))")
    expect_err("print(h.root2_tstmath(-1))")
    expect_err("print(h.div_tstmath(1, 0))")
    # Following crashes (Signal and then hoc_execerror in python world?)
    # expect_err("print(math.exp(1000.))")

    expect_err("print(h.div_tstmath(1, 0))")
    expect_err("print(h.div_tstmath(1, 0))")
    expect_err("print(h.div_tstmath(0, 0))")

    expect_err("print(h.root2_tstmath(-1))")
    # Mac arm44 following natlog and expon generate trace trap (SIGTRAP)
    #  Termination Reason:    Namespace SIGNAL, Code 5 Trace/BPT trap: 5
    #  2   libunwind.dylib     0x1b13c7314 _Unwind_RaiseException + 468
    #  3   libc++abi.dylib     0x1a6850bc0 __cxa_throw + 132
    #  4   libnrniv.dylib      0x104ee90ec hoc_execerror_mes(char const*, char const*, int) + 356 (hoc.cpp:753)
    """
    expect_err("print(h.natlog_tstmath(0))")
    expect_err("print(h.natlog_tstmath(-1))")
    expect_err("print(h.expon_tstmath(1000))")
    """
    # do in separate process, expect error
    import subprocess

    def run(cmd):
        s = "nrniv -c '{nrn_feenableexcept(1) " + cmd + "}'"
        print(s)
        x = subprocess.run(s, shell=True).returncode
        return x != 0

    assert run("natlog_tstmath(0)")
    assert run("natlog_tstmath(-1)")
    assert run("expon_tstmath(1000)")

    h.nrn_feenableexcept(0)
    print(h.div_tstmath(1, 0))


if __name__ == "__main__":
    test_math1()
    test_math2()
    test_math1()
