from neuron.tests.utils.strtobool import strtobool
import os
import tempfile

# Hacky, but it's non-trivial to pass commandline arguments to pytest tests.
enable_gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
mpi4py_option = bool(strtobool(os.environ.get("NRN_TEST_SPIKES_MPI4PY", "false")))
file_mode_option = bool(strtobool(os.environ.get("NRN_TEST_SPIKES_FILE_MODE", "false")))
nrnmpi_init_option = bool(
    strtobool(os.environ.get("NRN_TEST_SPIKES_NRNMPI_INIT", "false"))
)

# following at top level and early enough avoids...
# *** The MPI_Iprobe() function was called after MPI_FINALIZE was invoked.

# mpi4py needs to be imported before importing h
if mpi4py_option:
    from mpi4py import MPI
    from neuron import h, gui
# without mpi4py we need to call nrnmpi_init explicitly
elif nrnmpi_init_option:
    from neuron import h

    h.nrnmpi_init()
    # if mpi is active, don't ask for gui til it is turned off for all ranks > 0
    from neuron import gui
# otherwise serial execution
else:
    from neuron import h, gui


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

    corenrn_all_spike_t = h.Vector()
    corenrn_all_spike_gids = h.Vector()

    pc.spike_record(-1, corenrn_all_spike_t, corenrn_all_spike_gids)

    pc.set_maxstep(10)

    def run(mode):
        h.stdinit()
        if mode == 0:
            pc.psolve(h.tstop)
        elif mode == 1:
            while h.t < h.tstop:
                pc.psolve(h.t + 1.0)
        else:
            while h.t < h.tstop:
                h.continuerun(h.t + 0.5)
                pc.psolve(h.t + 0.5)

        corenrn_all_spike_t_py = corenrn_all_spike_t.to_python()
        corenrn_all_spike_gids_py = corenrn_all_spike_gids.to_python()

        # check spikes match
        assert len(nrn_spike_t)  # check we've actually got spikes
        assert len(nrn_spike_t) == len(nrn_spike_gids)  # matching no. of gids
        if nrn_spike_t != corenrn_all_spike_t_py:
            print(mode)
            print(nrn_spike_t)
            print(nrn_spike_gids)
            print(corenrn_all_spike_t_py)
            print(corenrn_all_spike_gids_py)
            print(
                [
                    corenrn_all_spike_t[i] - nrn_spike_t[i]
                    for i in range(len(nrn_spike_t))
                ]
            )
        assert nrn_spike_t == corenrn_all_spike_t_py
        assert nrn_spike_gids == corenrn_all_spike_gids_py

    # CORENEURON run
    from neuron import coreneuron

    with coreneuron(enable=True, gpu=enable_gpu, file_mode=file_mode, verbose=0):
        run_modes = [0] if file_mode else [0, 1, 2]
        for mode in run_modes:
            run(mode)
        # Make sure that file mode also works with custom coreneuron.data_path
        if file_mode:
            temp_coreneuron_data_folder = tempfile.TemporaryDirectory(
                "coreneuron_input"
            )  # auto removed
            coreneuron.data_path = temp_coreneuron_data_folder.name
            run(0)

    return h


if __name__ == "__main__":
    h = test_spikes()
    if mpi4py_option or nrnmpi_init_option:
        pc = h.ParallelContext()
        pc.barrier()
    h.quit()
