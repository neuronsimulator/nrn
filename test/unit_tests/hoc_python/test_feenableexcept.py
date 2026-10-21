"""Round-trip, faulthandler, and the three hardware traps for nrn_feenableexcept.

print(1/0) is a HOC check and does not raise SIGFPE. log(0) is the hardware
divide-by-zero. The interpreter exp() clamps at 700, so overflow is the multiply.
Windows reports the trap and then aborts, so these tests stay on the Unix path.
"""

import faulthandler
import math
import os
import sys

import pytest
from neuron import h

if sys.platform == "win32":
    pytest.skip(
        "Windows reports the trap and aborts instead of raising hoc_execerror",
        allow_module_level=True,
    )

try:
    h.nrn_feenableexcept(0)
except RuntimeError as exc:
    if "not available" in str(exc):
        pytest.skip(
            "nrn_feenableexcept is not available on this build",
            allow_module_level=True,
        )
    raise

if h.ParallelContext().nhost() > 1:
    pytest.skip(
        "hoc_execerror aborts when nhost > 1",
        allow_module_level=True,
    )


@pytest.fixture
def traps_off():
    h.nrn_feenableexcept(0)
    yield
    h.nrn_feenableexcept(0)


def _arm(capfd):
    # pytest enables faulthandler, which replaces SIGFPE and SIGILL.
    # Install it first, then arm, so the trap handler has to take the signal back.
    faulthandler.enable()
    previous = h.nrn_feenableexcept(1)
    capfd.readouterr()
    return previous


def _hoc_trap(capfd, stmt, kind):
    assert h(stmt) is False
    err = capfd.readouterr().err
    assert f"Floating exception: {kind}" in err
    assert "Floating point exception" in err
    assert "Fatal Python error" not in err


def _py_trap(capfd, call, kind):
    with pytest.raises(RuntimeError, match="Floating point exception") as caught:
        call()
    assert "argument out of domain" not in str(caught.value)
    err = capfd.readouterr().err
    assert f"Floating exception: {kind}" in err
    assert "Fatal Python error" not in err


def test_round_trip(traps_off):
    assert h.nrn_feenableexcept(1) == 0.0
    assert h.nrn_feenableexcept(1) == 1.0
    assert h.nrn_feenableexcept() == 1.0
    assert h.nrn_feenableexcept(0) == 1.0
    assert h.nrn_feenableexcept(0) == 0.0

    # No argument arms. A saved 0 restores the off state.
    assert h.nrn_feenableexcept() == 0.0
    assert h.nrn_feenableexcept(0) == 1.0
    old = h.nrn_feenableexcept(1)
    assert old == 0.0
    assert h.nrn_feenableexcept(old) == 1.0
    assert h.nrn_feenableexcept(0) == 0.0


def test_argument_out_of_range(traps_off):
    with pytest.raises(RuntimeError, match="Arg out of range"):
        h.nrn_feenableexcept(2)
    with pytest.raises(RuntimeError, match="Arg out of range"):
        h.nrn_feenableexcept(-1)
    assert h.nrn_feenableexcept(0) == 0.0


def _thread_sanitizer():
    # The trap becomes a signal. ThreadSanitizer does not deliver that signal
    # into hoc_execerror; the test process dies.
    blob = " ".join(
        os.environ.get(name, "")
        for name in (
            "NRN_SANITIZER_PRELOAD_VAL",
            "LD_PRELOAD",
            "DYLD_INSERT_LIBRARIES",
        )
    )
    return "tsan" in blob.lower()


@pytest.mark.skipif(
    _thread_sanitizer(),
    reason="ThreadSanitizer does not deliver a floating-point trap into hoc_execerror",
)
def test_faulthandler_and_three_traps(traps_off, capfd):
    assert _arm(capfd) == 0.0

    _hoc_trap(capfd, "sqrt(-1)", "Invalid (no well defined result)")
    _py_trap(capfd, lambda: h.sqrt(-1), "Invalid (no well defined result)")

    _hoc_trap(capfd, "log(0)", "Divide by zero")
    _py_trap(capfd, lambda: h.log(0), "Divide by zero")

    _hoc_trap(capfd, "print(1e308*1e308)", "Overflow")
    _hoc_trap(capfd, "print(1e308*1e308)", "Overflow")

    assert h.nrn_feenableexcept(0) == 1.0
    with pytest.raises(RuntimeError, match="argument out of domain"):
        h.sqrt(-1)
    err = capfd.readouterr().err
    assert "Floating exception" not in err
    assert math.isinf(h.log(0))
    err = capfd.readouterr().err
    assert "Floating exception" not in err
