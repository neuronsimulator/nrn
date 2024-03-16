from neuron.tests.utils.strtobool import strtobool
import os

from neuron import h, gui


def test_direct_memory_transfer():
    h("""create soma""")
    h.soma.L = 5.6419
    h.soma.diam = 5.6419
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(0.5))
    ic.delay = 0.5
    ic.dur = 0.1
    ic.amp = 0.3

    # for testing external mod file
    h.soma.insert("Sample")

    h.cvode.use_fast_imem(1)

    v = h.Vector()
    v.record(h.soma(0.5)._ref_v, sec=h.soma)
    i_mem = h.Vector()
    i_mem.record(h.soma(0.5)._ref_i_membrane_, sec=h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    h.run()
    vstd = v.cl()
    tvstd = tv.cl()
    i_memstd = i_mem.cl()
    # Save current (after run) value to compare with transfer back from coreneuron
    tran_std = [h.t, h.soma(0.5).v, h.soma(0.5).hh.m]

    from neuron import coreneuron

    coreneuron.enable = True
    coreneuron.verbose = 0
    coreneuron.model_stats = True
    coreneuron.gpu = bool(strtobool(os.environ.get("CORENRN_ENABLE_GPU", "false")))
    coreneuron.num_gpus = 1

    pc = h.ParallelContext()

    def run(mode):
        pc.set_maxstep(10)
        h.stdinit()
        if mode == 0:
            pc.psolve(h.tstop)
        elif mode == 1:
            while abs(h.t - h.tstop) > 0.1 * h.dt:
                pc.psolve(h.t + 1.0)
        else:
            while abs(h.t - h.tstop) > 0.1 * h.dt:
                h.continuerun(h.t + 0.5)
                pc.psolve(h.t + 0.5)
        tran = [h.t, h.soma(0.5).v, h.soma(0.5).hh.m]

        assert tv.eq(tvstd)
        assert (
            v.cl().sub(vstd).abs().max() < 1e-10
        )  # usually v == vstd, some compilers might give slightly different results
        assert i_mem.cl().sub(i_memstd).abs().max() < 1e-10
        assert h.Vector(tran_std).sub(h.Vector(tran)).abs().max() < 1e-10

    for mode in [0, 1, 2]:
        run(mode)

    cnargs = coreneuron.nrncore_arg(h.tstop)
    h.stdinit()
    assert tv.size() == 1 and tvstd.size() != 1
    pc.nrncore_run(cnargs, 1)
    assert tv.eq(tvstd)

    # print warning if HocEvent on event queue when CoreNEURON starts
    def test_hoc_event():
        print("in test_hoc_event() at t=%g" % h.t)
        if h.t < 1.001:
            h.CVode().event(h.t + 1.0, test_hoc_event)

    fi = h.FInitializeHandler(2, test_hoc_event)
    coreneuron.enable = False
    run(0)
    coreneuron.enable = True
    run(0)


if __name__ == "__main__":
    test_direct_memory_transfer()
    h.quit()
