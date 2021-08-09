import os
import pytest
import sys
import traceback

# Hacky, but it's non-trivial to pass commandline arguments to pytest tests.
enable_gpu = bool(os.environ.get("CORENRN_ENABLE_GPU", ""))
mpi4py_option = bool(os.environ.get("NRN_TEST_SPIKES_MPI4PY", ""))
file_mode_option = bool(os.environ.get("NRN_TEST_SPIKES_FILE_MODE", ""))
nrnmpi_init_option = bool(os.environ.get("NRN_TEST_SPIKES_NRNMPI_INIT", ""))


def test_spikes(
    use_mpi4py=mpi4py_option,
    use_nrnmpi_init=nrnmpi_init_option,
    file_mode=file_mode_option,
):
    print(
        "test_spikes(use_mpi4py={}, use_nrnmpi_init={}, file_mode={})".format(
            use_mpi4py, use_nrnmpi_init, file_mode
        )
    )
    # mpi4py needs tp be imported before importing h
    if use_mpi4py:
        from mpi4py import MPI
        from neuron import h, gui
    # without mpi4py we need to call nrnmpi_init explicitly
    elif use_nrnmpi_init:
        from neuron import h, gui

        h.nrnmpi_init()
    # otherwise serial execution
    else:
        from neuron import h, gui

    h("""create soma""")
    h.soma.L = 5.6419
    h.soma.diam = 5.6419
    h.soma.insert("hh")
    h.soma.nseg = 3
    ic = h.IClamp(h.soma(0.25))
    ic.delay = 0.1
    ic.dur = 0.1
    ic.amp = 0.3

    ic2 = h.IClamp(h.soma(0.75))
    ic2.delay = 5.5
    ic2.dur = 1
    ic2.amp = 0.3

    h.tstop = 10
    h.cvode.use_fast_imem(1)
    h.cvode.cache_efficient(1)

    pc = h.ParallelContext()

    pc.set_gid2node(pc.id() + 1, pc.id())
    myobj = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
    pc.cell(pc.id() + 1, myobj)

    # NEURON run
    nrn_spike_t = h.Vector()
    nrn_spike_gids = h.Vector()

    # rank 0 record spikes for all gid while others
    # for specific gid. this is for better test coverage.
    pc.spike_record(-1 if pc.id() == 0 else (pc.id() + 1), nrn_spike_t, nrn_spike_gids)

    h.run()

    nrn_spike_t = nrn_spike_t.to_python()
    nrn_spike_gids = nrn_spike_gids.to_python()

    # CORENEURON run
    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.gpu = enable_gpu
    coreneuron.file_mode = file_mode
    coreneuron.verbose = 0
    h.stdinit()
    corenrn_all_spike_t = h.Vector()
    corenrn_all_spike_gids = h.Vector()

    pc.spike_record(-1, corenrn_all_spike_t, corenrn_all_spike_gids)
    pc.psolve(h.tstop)

    corenrn_all_spike_t = corenrn_all_spike_t.to_python()
    corenrn_all_spike_gids = corenrn_all_spike_gids.to_python()

    # check spikes match
    assert len(nrn_spike_t)  # check we've actually got spikes
    assert len(nrn_spike_t) == len(nrn_spike_gids)  # matching no. of gids
    assert nrn_spike_t == corenrn_all_spike_t
    assert nrn_spike_gids == corenrn_all_spike_gids

    return h


if __name__ == "__main__":
    try:
        h = test_spikes()
    except:
        traceback.print_exc()
        # Make the CTest test fail
        sys.exit(42)
    # The test doesn't exit without this.
    h.quit()
