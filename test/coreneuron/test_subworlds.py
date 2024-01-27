import sys
from neuron import h

h("""objref cvode""")
h.cvode = h.CVode()

use_coreneuron = True
subsize = 3

if use_coreneuron:
    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.verbose = 0


def test_subworlds():
    h.nrnmpi_init()
    pc = h.ParallelContext()
    pc.subworlds(subsize)
    rank = int(pc.id())
    nhost = int(pc.nhost())

    ibbs = pc.id_world() // subsize
    nbbs = pc.nhost_world() // subsize
    x = 0 if pc.nhost_world() % subsize == 0 else 1
    assert pc.nhost_bbs() == ((nbbs + x) if pc.id() == 0 else -1)
    assert pc.id_bbs() == (ibbs if pc.id() == 0 else -1)

    if rank == 0:
        soma = h.Section(name="soma")
        soma.L = 20
        soma.diam = 20
        soma.insert("hh")
        iclamp = h.IClamp(soma(0.5))
        iclamp.delay = 2
        iclamp.dur = 0.1

        if pc.id_bbs() == 0:
            iclamp.amp = 0.0
        elif pc.id_bbs() == 1:
            iclamp.amp = 0.9
        else:
            raise RuntimeError(f"Invalid subworld index {pc.id_bbs()}")

        rec_v = h.Vector()
        rec_t = h.Vector()
        rec_v.record(soma(0.5)._ref_v)  # Membrane potential vector
        rec_t.record(h._ref_t)
        syn = h.ExpSyn(soma(0.5))
        spike_detector = h.NetCon(soma(0.5)._ref_v, None, sec=soma)
        netstim = h.NetStim()
        netstim.number = 1
        netstim.start = 0
        nc = h.NetCon(netstim, syn)
        gid = 1
        pc.set_gid2node(gid, pc.id())
        pc.cell(gid, spike_detector)

    h.finitialize(-65)
    pc.set_maxstep(10)
    pc.psolve(250.0)
    if rank == 0:
        if pc.id_bbs() == 0:
            assert rec_v.max() < 0.0
        elif pc.id_bbs() == 1:
            assert rec_v.max() > 0.0
        else:
            raise RuntimeError(f"Invalid subworld index {pc.id_bbs()}")

        print(f"subworld {pc.id_bbs()}: {rec_v.max()}")

    pc.done()
    h.quit()


if __name__ == "__main__":
    test_subworlds()
