"""Point processes that share a node and write an ion current: CoreNEURON adds every
instance's contribution to the node's current, on the CPU and on the GPU, as NEURON does.
"""

from neuron.tests.utils.strtobool import strtobool
import os

from neuron import h

pc = h.ParallelContext()


def test_pp_ion_sum():
    secs, pps, ncs = [], [], []
    for i in range(4):
        s = h.Section(name=f"pp_ion_{i}")
        s.L = s.diam = 10
        s.insert("pas")
        for k in range(32):
            p = h.CaWriterPP(s(0.5))
            p.amp = 0.001 * (k + 1 + i)
            pps.append(p)
        pc.set_gid2node(i, pc.id())
        nc = h.NetCon(s(0.5)._ref_v, None, sec=s)
        pc.cell(i, nc)
        secs.append(s)
        ncs.append(nc)
    vecs = [h.Vector().record(s(0.5)._ref_ica) for s in secs]

    def run():
        pc.set_maxstep(10)
        h.finitialize(-65)
        pc.psolve(1)
        return [v.to_python() for v in vecs]

    std = run()
    assert all(trace[-1] > 0 for trace in std)

    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    result = run()
    coreneuron.enable = False
    pc.gid_clear()

    for expected, got in zip(std, result):
        assert len(expected) == len(got)
        for x, y in zip(expected, got):
            assert abs(x - y) <= 1e-12 * abs(x), f"ica {y} where NEURON gives {x}"


if __name__ == "__main__":
    test_pp_ion_sum()
