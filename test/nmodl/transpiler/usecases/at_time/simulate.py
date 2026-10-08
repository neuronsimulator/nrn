import numpy as np
from neuron import gui, h
from neuron.units import ms


def simulate():
    nseg = 1

    s = h.Section()
    s.insert("example")
    s.nseg = nseg

    v_hoc = h.Vector().record(s(0.5)._ref_v)
    t_hoc = h.Vector().record(h._ref_t)

    f = s(0.5).example.f
    h.stdinit()
    h.tstop = 1000 * ms
    h.dt = 100 * ms
    h.run()

    def at_time_exact(te):
        return h.t - h.dt < te - 1e-11 <= h.t

    for t in [-5, -1, -1e-13, 0, 1e-13, 1e-5, 0.5, 1, 2, 50]:
        assert np.allclose(f(t), at_time_exact(t))


if __name__ == "__main__":
    simulate()
