import ctypes
import sys

import pytest


pytestmark = pytest.mark.usefixtures("errors_do_not_abort_mpi")


def _model(h, rxd, ecs):
    sec = h.Section(name="sec")
    sec.nseg = 3
    if ecs:
        region = rxd.Extracellular(-55, -55, -55, 55, 55, 55, dx=33)
        x = rxd.Species(region, name="fluxerr", d=0.0, initial=0)
        return sec, x, x[region].node_by_location(0, 0, 0)
    region = rxd.Region([sec], nrn_region="i")
    x = rxd.Species(region, name="fluxerr", initial=0)
    return sec, x, x.nodes(sec(0.5))[0]


def _run(h, cvode):
    h.CVode().active(cvode)
    h.finitialize(-65)
    h.continuerun(1)


@pytest.mark.parametrize("ecs", [False, True], ids=["1d", "ecs"])
@pytest.mark.parametrize("cvode", [False, True], ids=["fixed", "cvode"])
def test_include_flux_callback_raises(neuron_nosave_instance, capfd, ecs, cvode):
    """An exception in an include_flux callback stops the run with an error."""
    h, rxd, save_path = neuron_nosave_instance
    model = _model(h, rxd, ecs)

    def callback():
        raise ValueError("flux callback failed on purpose")

    model[2].include_flux(callback)
    with pytest.raises(RuntimeError):
        _run(h, cvode)
    assert "flux callback failed on purpose" in capfd.readouterr().err


@pytest.mark.parametrize("ecs", [False, True], ids=["1d", "ecs"])
@pytest.mark.parametrize("cvode", [False, True], ids=["fixed", "cvode"])
def test_include_flux_callback_not_a_number(neuron_nosave_instance, capfd, ecs, cvode):
    """A callback that returns something other than a number is an error too."""
    h, rxd, save_path = neuron_nosave_instance
    model = _model(h, rxd, ecs)
    model[2].include_flux(lambda: "1000")
    with pytest.raises(RuntimeError):
        _run(h, cvode)
    assert "did not return a number" in capfd.readouterr().err


@pytest.mark.parametrize("cvode", [False, True], ids=["fixed", "cvode"])
def test_include_flux_callback_raises_3d(neuron_nosave_instance, capfd, cvode):
    """The same for the 3D intracellular solver."""
    h, rxd, save_path = neuron_nosave_instance
    sec = h.Section(name="sec")
    sec.L = 1
    sec.nseg = 11
    sec.diam = 2
    rxd.set_solve_type(dimension=3)
    region = rxd.Region([sec], nrn_region="i")
    x = rxd.Species(region, name="fluxerr3d", initial=0)

    def callback():
        raise ValueError("flux callback failed on purpose")

    x.nodes[0].include_flux(callback)
    with pytest.raises(RuntimeError):
        _run(h, cvode)
    assert "flux callback failed on purpose" in capfd.readouterr().err


def test_include_flux_callback_failure_with_a_failing_stderr(
    neuron_nosave_instance, monkeypatch
):
    """Reporting the error can fail too; the C code must still be told the callback failed."""
    h, rxd, save_path = neuron_nosave_instance

    class FailingStream:
        def write(self, text):
            raise OSError("stderr failed on purpose")

        def flush(self):
            raise OSError("stderr failed on purpose")

    def callback():
        raise ValueError("flux callback failed on purpose")

    callables = rxd.rxd._node_flux_callables
    callables.append(callback)
    monkeypatch.setattr(sys, "stderr", FailingStream())
    try:
        assert rxd.rxd._eval_node_flux_callable(len(callables) - 1, None) == 1
    finally:
        monkeypatch.undo()
        callables.pop()


def _in_use_bytes():
    """Bytes malloc has handed out and not got back (glibc)."""

    class MallInfo2(ctypes.Structure):
        _fields_ = [
            (name, ctypes.c_size_t)
            for name in (
                "arena",
                "ordblks",
                "smblks",
                "hblks",
                "hblkhd",
                "usmblks",
                "fsmblks",
                "uordblks",
                "fordblks",
                "keepcost",
            )
        ]

    libc = ctypes.CDLL(None)
    libc.mallinfo2.restype = MallInfo2
    info = libc.mallinfo2()
    return info.uordblks + info.hblkhd


def _assert_failing_runs_do_not_leak(h, cvode, bytes_per_run):
    def failing_run():
        with pytest.raises(RuntimeError):
            h.CVode().active(cvode)
            h.finitialize(-65)
            h.fadvance()

    for _ in range(3):
        failing_run()
    runs = 20
    grown = []
    for _ in range(3):
        before = _in_use_bytes()
        for _ in range(runs):
            failing_run()
        grown.append(_in_use_bytes() - before)
    # a leak grows every batch; a heap extension is one large step in one batch
    assert min(grown) < runs * bytes_per_run / 4


def test_include_flux_error_does_not_leak(neuron_nosave_instance):
    """An error in a variable step run gives back what the right hand side allocated."""
    h, rxd, save_path = neuron_nosave_instance
    try:
        _in_use_bytes()
    except (AttributeError, TypeError):
        pytest.skip("needs glibc mallinfo2")
    sec = h.Section(name="sec")
    sec.nseg = 2001
    region = rxd.Region([sec], nrn_region="i")
    x = rxd.Species(region, name="fluxleak", initial=0)

    def callback():
        raise ValueError("flux callback failed on purpose")

    x.nodes(sec(0.5))[0].include_flux(callback)
    # one right hand side is 8 bytes per state, about 16 kB here
    _assert_failing_runs_do_not_leak(h, True, 8 * 2001)


@pytest.mark.parametrize("cvode", [False, True], ids=["fixed", "cvode"])
def test_include_flux_error_does_not_leak_ecs(neuron_nosave_instance, cvode):
    """With MPI the extracellular fluxes are gathered into a buffer, freed on an error too."""
    h, rxd, save_path = neuron_nosave_instance
    try:
        _in_use_bytes()
    except (AttributeError, TypeError):
        pytest.skip("needs glibc mallinfo2")
    h.Section(name="sec")
    region = rxd.Extracellular(-65, -65, -65, 65, 65, 65, dx=10)
    x = rxd.Species(region, name="fluxleakecs", d=0.0, initial=0)

    def callback():
        raise ValueError("flux callback failed on purpose")

    nodes = list(x[region].nodes)
    for node in nodes:
        node.include_flux(callback)
    # the buffer is 8 bytes per flux on all the ranks
    _assert_failing_runs_do_not_leak(h, cvode, 8 * len(nodes))
