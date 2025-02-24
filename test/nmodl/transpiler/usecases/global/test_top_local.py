import numpy as np

from neuron import h, gui
from neuron.units import ms


def test_top_local():
    nseg = 1

    s0 = h.Section()
    s0.insert("top_local")
    s0.nseg = nseg

    s1 = h.Section()
    s1.insert("top_local")
    s1.nseg = nseg

    pc = h.ParallelContext()
    pc.nthread(2)
    pc.partition(0, h.SectionList([s0]))
    pc.partition(1, h.SectionList([s1]))

    t = h.Vector().record(h._ref_t)
    y0 = h.Vector().record(s0(0.5).top_local._ref_y)
    y1 = h.Vector().record(s1(0.5).top_local._ref_y)

    h.stdinit()
    h.continuerun(1.0 * ms)

    t = np.array(t.as_numpy())
    y0 = np.array(y0.as_numpy())
    y1 = np.array(y1.as_numpy())

    i0 = t < 0.33
    i1 = 0.33 <= t

    # The values on thread 0:
    assert np.all(y0[i0] == 2.0)
    assert np.all(y0[i1] == 3.0)

    # The values on thread 1:
    assert np.all(y1[i0] == 2.0)
    assert np.all(y1[i1] == 3.0)


if __name__ == "__main__":
    test_top_local()
