# Sum of all i_membrane_ should equal sum of all ElectrodeCurrent
# For a demanding test, use a tree with many IClamp and ExpSyn point processes
# sprinkled on zero and non-zero area nodes.
import distutils.util
import os

from neuron import config, gui, h

h.load_file("stdrun.hoc")  # for h.cvode_active


class Cell:
    def __init__(self, id, nsec):
        r = h.Random()
        r.Random123(id, 0, 0)
        nsec += int(r.discunif(0, 4))  # for nontrivial cell_permute=1

        self.id = id
        self.secs = [h.Section(name="d" + str(i), cell=self) for i in range(nsec)]

        # somewhat random tree, d[0] plays role of soma with connections to
        # d[0](0.5) and all others to 1.0
        for i in range(1, nsec):
            iparent = int(r.discunif(0, i - 1))
            x = 0.5 if iparent == 0 else 1.0
            self.secs[i].connect(self.secs[iparent](x))

        # uniform L and diam but somewhat random passive g and e
        for i, sec in enumerate(self.secs):
            sec.L = 10 if i > 0 else 5
            sec.diam = 1 if i > 0 else 5
            sec.insert("pas")
            sec.g_pas = 0.0001 * r.uniform(1.0, 1.1)
            sec.e_pas = -65 * r.uniform(1.0, 1.1)

        # IClamp and ExpSyn at every location (even duplicates) with random
        # parameters (would rather use a Shunt, but ...)
        self.ics = []
        self.syns = []
        self.netcons = []
        self.netstim = h.NetStim()
        self.netstim.number = 1
        self.netstim.start = 0.0
        for sec in self.secs:
            for seg in sec.allseg():
                ic = h.IClamp(seg)
                ic.delay = 0.1
                ic.dur = 1.0
                ic.amp = 0.001 * r.uniform(1.0, 1.1)
                self.ics.append(ic)

                syn = h.ExpSyn(seg)
                syn.e = -65 * r.uniform(1.0, 1.1)
                syn.tau = r.uniform(0.1, 1.0)
                self.syns.append(syn)
                nc = h.NetCon(self.netstim, syn)
                nc.delay = 0.2
                nc.weight[0] = 0.001 * r.uniform(1.0, 1.1)
                self.netcons.append(nc)

    def __str__(self):
        return "Cell" + str(self.id)


def total_imem():
    imem = 0.0
    for sec in h.allsec():
        for seg in sec.allseg():
            if seg.x == 0.0 and sec.parentseg() is not None:
                assert seg.i_membrane_ == sec.parentseg().i_membrane_
                continue  # don't count twice
            imem += seg.i_membrane_
    # print("total_imem ", imem)
    return imem


def total_iclamp(ics):
    icur = 0.0
    for ic in ics:
        icur += ic.i
    # print("total_iclamp ", icur)
    return icur


# verified that events arrived at syn to change g.
def total_syn_g(syns):
    g = 0
    for syn in syns:
        g += syn.g
    # print("total syn g ", g)
    return g


# Remark:
#  The iteration over all segments (including 0 area nodes) without
#  counting 0 area nodes twice is
"""
    for sec in h.allsec():
     for seg in sec.allseg():
       if seg.x == sec.orientation() and sec.parentseg() is not None:
         continue
       # rest of the "for seg..." body
"""
#  Note that sec.parentseg() is needed to count the root and use of
#  sec.trueparentseg() would miss the root node. Also, although an
#  extremely rare edge case, sec.orientation() is needed to match
#  which segment is closest to root.


def test_allseg_unique_iter():
    a = h.Section("a")
    b = h.Section("b")
    c = h.Section("c")
    d = h.Section("d")
    e = h.Section("e")
    f = h.Section("f")
    g = h.Section("g")
    g1 = h.Section("g1")
    g2 = h.Section("g2")
    g3 = h.Section("g3")
    g4 = h.Section("g4")

    b.connect(a(0), 0)
    c.connect(a(0), 0)
    d.connect(a(0), 1)
    e.connect(a(0), 1)
    f.connect(b(1), 0)
    g.connect(b(1), 1)
    g1.connect(g(0), 0)
    g2.connect(g(0), 1)
    g3.connect(g(1), 0)
    g4.connect(g(1), 1)

    # h.topology()

    unique_segs = []
    for sec in h.allsec():
        for seg in sec.allseg():
            if seg.x == sec.orientation() and sec.parentseg() is not None:
                continue
            # print ("%s(%g)" % (sec.name(), seg.x))
            unique_segs.append(seg)

    nseg = sum(sec.nseg for sec in h.allsec())
    nsec = sum(1 for _ in h.allsec())
    ncell = 1
    assert len(unique_segs) == nseg + nsec + ncell
    # nothing left out and nothing done twice
    for sec in h.allsec():
        sec.v = 100.0
    for seg in unique_segs:
        assert seg.v == 100.0  # nothing done twice
        seg.v = 0.0
    for sec in h.allsec():
        for seg in sec.allseg():
            assert seg.v == 0.0  # nothing left out


# verified that there is i_membrane_ != 0.0 in zero area nodes.
def print_imem():
    print("t=%g" % h.t)
    for sec in h.allsec():
        for seg in sec.allseg():
            if seg.x == 0.0 and sec.parentseg() is not None:
                continue  # don't count twice
            print("%s(%g).i_membrane_ = %g" % (sec.name(), seg.x, seg.i_membrane_))


def balanced(ics, tolerance):
    # print_imem() # helped to verify the test is substantive
    bal = abs(total_imem() - total_iclamp(ics))
    if bal > tolerance:
        print(
            "t=%g bal=%g total_imem=%g total_iclamp=%g"
            % (h.t, bal, total_imem(), total_iclamp(ics))
        )
    assert bal <= tolerance


def run(tstop, ics, tolerance):
    # to get nontrivial initialized i_membrane_, initialize to random voltage.
    r = h.Random()
    r.Random123(0, 1, 0)
    for sec in h.allsec():
        for seg in sec.allseg():
            # don't care if some segments counted twice
            seg.v = -65.0 + r.uniform(0, 5)
    h.finitialize()
    balanced(ics, tolerance)
    while h.t < 1.0:
        h.fadvance()
        balanced(ics, tolerance)


def test_fastimem():
    cells = [Cell(id, 10) for id in range(2)]
    # h.topology()
    cvode = h.CVode()
    ics = h.List("IClamp")
    syns = h.List("ExpSyn")
    cvode.use_fast_imem(1)
    h.finitialize(-65)
    run(1.0, ics, 1e-13)
    total_syn_g(syns)
    h.cvode_active(1)
    run(1.0, ics, 1e-12)
    cvode.use_fast_imem(0)
    h.cvode_active(0)


def coreneuron_available():
    if not config.arguments["NRN_ENABLE_CORENEURON"]:
        return False
    # But can it be loaded?
    cvode = h.CVode()
    cvode.cache_efficient(1)
    pc = h.ParallelContext()
    h.finitialize()
    result = 0
    import sys
    from io import StringIO

    original_stderr = sys.stderr
    sys.stderr = StringIO()
    try:
        pc.nrncore_run("--tstop 1 --verbose 0")
        result = 1
    except Exception as e:
        pass
    sys.stderr = original_stderr
    cvode.cache_efficient(0)
    return result


def print_fast_imem():
    ix = h.Vector()
    imem = h.Vector()
    for sec in h.allsec():
        for seg in sec.allseg():
            if seg.x == 0.0 and sec.parentseg() is not None:
                continue  # don't count twice
            ix.append(seg.node_index())
            imem.append(seg.i_membrane_)
    si = ix.sortindex()
    ix.index(ix.c(), si)
    imem.index(imem.c(), si)
    f = open("fastimem.nrn", "w")
    f.write("%d\n" % int(ix.size()))
    for i, x in enumerate(imem):
        assert i == int(ix[i])
        f.write("%d %.20g\n" % (i, x))
    f.close()


def test_fastimem_corenrn():
    pc = h.ParallelContext()
    ncell = 5
    cvode = h.CVode()
    cvode.cache_efficient(0)
    # If the gui has been imported (possibly by another test) then there is a
    # thread asynchronously calling process_events -- make sure that doesn't
    # happen partway through creating cells
    with gui.disabled():
        cells = [Cell(id, 10) for id in range(ncell)]
    cvode.use_fast_imem(1)

    # When nthread changes, or internal model data needs to be reallocated,
    # pointers need to be updated. Use of i_membrane_ requires that the user
    # update the pointers to i_membrane_.
    imem = []

    def imem_update():
        nonlocal imem
        with gui.disabled():
            imem = [
                h.Vector().record(cell.ics[0], cell.secs[3](0.5)._ref_i_membrane_)
                for cell in cells
            ]

    imem_updater = h.PtrVector(1)
    imem_updater.ptr_update_callback(imem_update)
    imem_update()

    tstop = 1.0

    def init_v():
        # to get nontrivial initialized i_membrane_, initialize to random voltage.
        r = h.Random()
        r.Random123(0, 1, 0)
        for sec in h.allsec():
            for seg in sec.allseg():
                # don't care if some segments counted twice
                seg.v = -65.0 + r.uniform(0, 5)
        h.finitialize()

    def run(tstop):
        pc.set_maxstep(10)
        with gui.disabled():
            init_v()
            pc.psolve(tstop)

    # standard
    run(tstop)
    imem_std = [vec.c() for vec in imem]
    max_abs_imem = [max(abs(x) for x in vec) for vec in imem_std]
    if not all(x > 0 for x in max_abs_imem):
        print(max_abs_imem, flush=True)
        assert False

    def compare(name, rel_tol=0.0):
        print("Comparing {}".format(name), flush=True)
        keep_going = True
        for i, (ref_vec, new_vec) in enumerate(zip(imem_std, imem)):
            ref_values = [x for x in ref_vec]
            new_values = [x for x in new_vec]
            if len(ref_values) != len(new_values):
                print(
                    "Got {} new values but {} reference ones in cell {}".format(
                        len(new_values), len(ref_values), i
                    ),
                    flush=True,
                )
                keep_going = False
            for j, (ref_val, new_val) in enumerate(zip(ref_values, new_values)):
                rel_diff = abs(ref_val - new_val) / max(abs(ref_val), abs(new_val))
                if rel_diff > rel_tol:
                    print(
                        "cell {} value {} new={} ref={} diff={}, rel_diff={}".format(
                            i, j, new_val, ref_val, new_val - ref_val, rel_diff
                        ),
                        flush=True,
                    )
                    keep_going = False
            new_vec.resize(0)
        assert keep_going

    # null comparison with the side effect of clearing imem
    compare("cache inefficient NEURON")

    cvode.cache_efficient(1)
    for nth in [2, 1]:  # leaves us in 1-threaded mode
        pc.nthread(nth)
        run(tstop)
        compare("cache efficient NEURON with {} threads".format(nth))

    # This leaves nthread=1, other values cause errors in the CoreNEURON tests below
    if coreneuron_available():
        cvode.cache_efficient(1)  # coreneuron_available() resets this
        from neuron import coreneuron

        coreneuron.enable = True
        coreneuron.verbose = 0
        coreneuron.gpu = distutils.util.strtobool(
            os.environ.get("CORENRN_ENABLE_GPU", "false")
        )
        tolerance = 5e-11
        run(tstop)
        compare("CoreNEURON online mode", rel_tol=tolerance)
        coreneuron.enable = False

        tvec = h.Vector().record(h._ref_t)
        init_v()
        while h.t < tstop - h.dt / 2:
            dt_above = 1.1 * h.dt  # comfortably above dt to avoid 0 step advance
            coreneuron.enable = True
            told = h.t
            pc.psolve(h.t + dt_above)
            assert h.t > told
            coreneuron.enable = False
            pc.psolve(h.t + dt_above)
        compare("Checking i_membrane_ trajectories", rel_tol=tolerance)

        print(
            "For file mode (offline) coreneuron comparison of i_membrane_ initialization",
            flush=True,
        )

        init_v()
        print_fast_imem()

        # The cells must have gids.
        for i, cell in enumerate(cells):
            pc.set_gid2node(i, pc.id())
            sec = cell.secs[0]
            pc.cell(i, h.NetCon(sec(0.5)._ref_v, None, sec=sec))

        # Write the data files
        init_v()
        pc.nrncore_write("./corenrn_data")

        # args needed for offline run of coreneuron
        coreneuron.enable = True
        coreneuron.file_mode = True

        arg = coreneuron.nrncore_arg(tstop)
        coreneuron.enable = False
        pc.gid_clear()
        print(arg)

    del imem_updater, imem
    cvode.use_fast_imem(0)


if __name__ == "__main__":
    test_allseg_unique_iter()
    test_fastimem()
    test_fastimem_corenrn()
    h.quit()
