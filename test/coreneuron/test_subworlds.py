import sys
from neuron import h

use_coreneuron = True

if use_coreneuron:
    from neuron import coreneuron
    coreneuron.enable = True
    coreneuron.verbose = 1

h.load_file("stdrun.hoc")


def test_subworlds():
    h.nrnmpi_init()
    pc = h.ParallelContext()
    pc.subworlds(3)
    rank = int(pc.id())
    nhost = int(pc.nhost())
    print(f"rank: {rank} nhost: {nhost} id_bbs: {pc.id_bbs()}")
    soma = h.Section(name="soma")
    soma.L = 20
    soma.diam = 20
    soma.insert("hh")
    iclamp = h.IClamp(soma(0.5))
    iclamp.delay = 2
    iclamp.dur = 0.1
    iclamp.amp = 0.9
    rec_v = h.Vector()
    rec_t = h.Vector()
    rec_v.record(soma(0.5)._ref_v)  # Membrane potential vector
    rec_t.record(h._ref_t)
    #h.fadvance()
    #h.continuerun(250)
    syn = h.ExpSyn(soma(0.5))
    spike_detector = h.NetCon(soma(0.5)._ref_v, None, sec=soma)
    netstim = h.NetStim()
    netstim.number = 1
    netstim.start = 0
    nc = h.NetCon(netstim, syn)
    gid = 1
    pc.set_gid2node(gid, pc.id())
    pc.cell(gid, spike_detector)
    h.cvode.cache_efficient(1)
    h.finitialize(-65)
    pc.set_maxstep(10)
    pc.psolve(250.0)
    
    print(rec_v.max())


if __name__ == "__main__":
    test_subworlds()


