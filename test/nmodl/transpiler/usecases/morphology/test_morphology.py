import numpy as np
from neuron import gui
from neuron import h

# This checks the use of `diam` and `area` in MOD files.


def check_morphology(sections):
    d2_expected = np.array([s(0.5).diam ** 2 for s in sections])
    a2_expected = np.array([s(0.5).area() ** 2 for s in sections])

    d2_actual = np.array([s(0.5).two_radii.square_diam() for s in sections])
    a2_actual = np.array([s(0.5).two_radii.square_area() for s in sections])

    assert np.all(d2_actual == d2_expected), f"delta: {d2_actual - d2_expected}"
    assert np.all(a2_actual == a2_expected), f"delta: {a2_actual - a2_expected}"


def check_voltage(t, v, sections):
    v0 = -65.0
    erev = 20.0
    rates = [s(0.5).two_radii.square_diam() + s(0.5).area() for s in sections]

    v_exact = np.array([erev + (v0 - erev) * np.exp(-c * t) for c in rates])
    assert np.all(
        np.abs(v - v_exact) < 0.01 * np.abs(v0)
    ), f"{np.max(np.abs(v - v_exact))=}"


def simulate():
    nsec = 10

    # Independent sections are needed to create independent ODEs.
    diams = 1.0 + 0.1 * np.arange(1, nsec + 1)
    sections = [h.Section() for _ in range(nsec)]
    for d, s in zip(diams, sections):
        s.nseg = 1
        s.insert("two_radii")
        s.pt3dadd(0.0, 0.0, 0.0, d)
        s.pt3dadd(1.0, 2.0, 0.0, d)

    t_hoc = h.Vector().record(h._ref_t)
    v_hoc = [h.Vector().record(s(0.5)._ref_v) for s in sections]

    check_morphology(sections)

    h.stdinit()
    h.dt = 0.001
    h.run()

    check_morphology(sections)

    t = np.array(t_hoc.as_numpy())
    v = np.array([vv.as_numpy() for vv in v_hoc])

    check_voltage(t, v, sections)


if __name__ == "__main__":
    simulate()
