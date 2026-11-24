import pytest

from neuron import h


# Callback times over five fixed steps (dt = 0.025), as released NEURON 9.0.2
# produces them: scatter runs before each step, gather after it (via re_init).
EXPECTED_TIMES = {
    0: [0.0, 0.025, 0.05, 0.075, 0.1],
    1: [0.025, 0.05, 0.075, 0.1, 0.125],
}


@pytest.mark.parametrize("direction", [0, 1], ids=["scatter", "gather"])
def test_extra_scatter_gather_python_callable(direction):
    """Python callbacks still run and can be removed in both directions."""
    cvode = h.CVode()
    active, dt = cvode.active(), h.dt
    section = h.Section(name="extra_scatter_gather_python")
    calls = []

    def callback():
        calls.append(float(h.t))

    registered = False
    try:
        cvode.active(0)
        h.dt = 0.025
        cvode.extra_scatter_gather(direction, callback)
        registered = True
        h.finitialize(-65)
        calls.clear()  # Require invocation during stepping, not just initialization.
        for _ in range(5):
            h.fadvance()
            if direction == 1:
                # Fixed-step gather callbacks run on explicit reinitialization.
                cvode.re_init()
        assert calls == pytest.approx(EXPECTED_TIMES[direction])

        cvode.extra_scatter_gather_remove(callback)
        registered = False
        for _ in range(5):
            h.fadvance()
            if direction == 1:
                cvode.re_init()
        assert len(calls) == 5
    finally:
        if registered:
            cvode.extra_scatter_gather_remove(callback)
        h.delete_section(sec=section)
        h.dt = dt
        cvode.active(active)
