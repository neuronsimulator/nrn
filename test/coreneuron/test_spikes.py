import pytest
import sys

from neuron import h, gui

def test_spikes(use_mpi4py=False):
    if use_mpi4py:
        from mpi4py import MPI
    h('''create soma''')
    h.soma.L=5.6419
    h.soma.diam=5.6419
    h.soma.insert("hh")
    h.soma.nseg = 3
    ic = h.IClamp(h.soma(.25))
    ic.delay = .1
    ic.dur = 0.1
    ic.amp = 0.3

    ic2 = h.IClamp(h.soma(.75))
    ic2.delay = 5.5
    ic2.dur = 1
    ic2.amp = 0.3

    h.tstop = 10
    h.cvode.use_fast_imem(1)
    h.cvode.cache_efficient(1)

    pc = h.ParallelContext()
  
    pc.set_gid2node(pc.id()+1, pc.id())
    myobj = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
    pc.cell(pc.id()+1, myobj)


    # NEURON spikes run
    nrn_spike_t = h.Vector()
    nrn_spike_gids = h.Vector()
    pc.spike_record(-1, nrn_spike_t, nrn_spike_gids)

    h.run()
   
    nrn_spike_t = nrn_spike_t.to_python()
    nrn_spike_gids = nrn_spike_gids.to_python()

    # CORENEURON spike_record(-1) / spike_record(gidlist):
    from neuron import coreneuron
    coreneuron.enable = True
    coreneuron.verbose = 0
    h.stdinit()
    corenrn_all_spike_t = h.Vector()
    corenrn_all_spike_gids = h.Vector()
    
    pc.spike_record( -1 if pc.id() == 0 else (pc.id()),
                     corenrn_all_spike_t,
                     corenrn_all_spike_gids )
    pc.psolve(h.tstop)

    corenrn_all_spike_t = corenrn_all_spike_t.to_python()
    corenrn_all_spike_gids = corenrn_all_spike_gids.to_python()


    # check spikes match
    assert(len(nrn_spike_t)) # check we've actually got spikes
    assert(len(nrn_spike_t) == len(nrn_spike_gids)); # matching no. of gids
    assert(nrn_spike_t == corenrn_all_spike_t)
    assert(nrn_spike_gids == corenrn_all_spike_gids)

    h.quit()
  

if __name__ == "__main__":
    test_spikes('mpi4py' in sys.argv)
