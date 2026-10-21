"""Windows nrn_feenableexcept arms traps, reports the kind, and aborts.

/EHsc does not turn a hardware trap into a C++ exception, so a trap is run
in a child process. MinGW libm checks sqrt and log in software and does not
raise; MSVC libm does. The multiply is a hardware overflow on both.
"""

import subprocess
import sys
import textwrap

import pytest
from neuron import h

if sys.platform != "win32":
    pytest.skip("Windows abort path", allow_module_level=True)

try:
    h.nrn_feenableexcept(0)
except RuntimeError as exc:
    if "not available" in str(exc):
        pytest.skip(
            "nrn_feenableexcept is not available on this build",
            allow_module_level=True,
        )
    raise

_CHILD = textwrap.dedent(
    """
    import sys
    from neuron import h

    op = sys.argv[1]
    h.nrn_feenableexcept(1)
    if op == "sqrt":
        try:
            h.sqrt(-1)
        except RuntimeError as exc:
            sys.stderr.write("SOFTWARE " + str(exc))
            raise SystemExit(0)
    elif op == "log":
        value = h.log(0)
        sys.stderr.write("SOFTWARE " + str(value))
        raise SystemExit(0)
    elif op == "mul":
        h("print(1e308*1e308)")
        sys.stderr.write("SOFTWARE mul")
        raise SystemExit(0)
    else:
        raise SystemExit(2)
    sys.stderr.write("NO_TRAP")
    raise SystemExit(0)
    """
)


@pytest.fixture(autouse=True)
def traps_off():
    h.nrn_feenableexcept(0)
    yield
    h.nrn_feenableexcept(0)


def _child(op):
    return subprocess.run(
        [sys.executable, "-c", _CHILD, op],
        capture_output=True,
        text=True,
        timeout=90,
    )


def _assert_abort(proc, kind):
    assert proc.returncode != 0
    assert f"Floating exception: {kind}" in proc.stderr
    assert "Floating point exception" in proc.stderr
    assert "SOFTWARE" not in proc.stderr
    assert "Fatal Python error" not in proc.stderr


def _assert_software_or_abort(proc, kind, software_marker):
    if proc.returncode == 0:
        assert "SOFTWARE" in proc.stderr
        assert software_marker in proc.stderr
        assert "Floating exception" not in proc.stderr
        return
    _assert_abort(proc, kind)


def test_round_trip():
    assert h.nrn_feenableexcept(1) == 0.0
    assert h.nrn_feenableexcept(1) == 1.0
    assert h.nrn_feenableexcept() == 1.0
    assert h.nrn_feenableexcept(0) == 1.0
    assert h.nrn_feenableexcept(0) == 0.0

    assert h.nrn_feenableexcept() == 0.0
    assert h.nrn_feenableexcept(0) == 1.0
    old = h.nrn_feenableexcept(1)
    assert old == 0.0
    assert h.nrn_feenableexcept(old) == 1.0
    assert h.nrn_feenableexcept(0) == 0.0


def test_argument_out_of_range():
    with pytest.raises(RuntimeError, match="Arg out of range"):
        h.nrn_feenableexcept(2)
    with pytest.raises(RuntimeError, match="Arg out of range"):
        h.nrn_feenableexcept(-1)
    assert h.nrn_feenableexcept(0) == 0.0


def test_disable_then_multiply_does_not_abort():
    assert h.nrn_feenableexcept(1) == 0.0
    assert h.nrn_feenableexcept(0) == 1.0
    assert h("print(1e308*1e308)") is True


def test_overflow_aborts():
    _assert_abort(_child("mul"), "Overflow")


def test_invalid_aborts_or_stays_software():
    _assert_software_or_abort(
        _child("sqrt"),
        "Invalid (no well defined result)",
        "out of domain",
    )


def test_divide_by_zero_aborts_or_stays_software():
    proc = _child("log")
    if proc.returncode == 0:
        assert "SOFTWARE" in proc.stderr
        assert "inf" in proc.stderr
        assert "Floating exception" not in proc.stderr
        return
    _assert_abort(proc, "Divide by zero")
