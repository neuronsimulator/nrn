import re
import sys
from neuron.expect_hocerr import expect_hocerr, expect_err, set_quiet
from neuron.tests.utils import (
    num_threads,
    parallel_context,
)

import numpy as np

from neuron import config, h, hoc


def test_soma():
    h("""create soma""")

    assert h.soma is not None

    h.soma.L = 5.6419
    h.soma.diam = 5.6419

    assert h.soma.L == 5.6419
    assert h.soma.diam == 5.6419


def test_simple_sim():
    h("""create soma""")
    h.load_file("stdrun.hoc")
    h.soma.L = 5.6419
    h.soma.diam = 5.6419
    expect_hocerr(h.ion_register, ("na", 2))
    assert h.ion_charge("na_ion") == 1.0
    expect_hocerr(h.ion_register, ("ca", 3))
    assert h.ion_charge("ca_ion") == 2.0
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(0.5))
    ic.delay = 0.5
    ic.dur = 0.1
    ic.amp = 0.3

    v = h.Vector()
    v.record(h.soma(0.5)._ref_v, sec=h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    h.run()
    assert v[0] == -65.0


def test_spikes():
    ref_spikes = np.array([1.05])

    h("""create soma""")
    h.load_file("stdrun.hoc")
    h.soma.L = 5.6419
    h.soma.diam = 5.6419
    h.soma.insert("hh")
    ic = h.IClamp(h.soma(0.5))
    ic.delay = 0.5
    ic.dur = 0.1
    ic.amp = 0.3

    v = h.Vector()
    v.record(h.soma(0.5)._ref_v, sec=h.soma)
    tv = h.Vector()
    tv.record(h._ref_t, sec=h.soma)
    nc = h.NetCon(h.soma(0.5)._ref_v, None, sec=h.soma)
    spikestime = h.Vector()
    nc.record(spikestime)
    h.run()

    simulation_spikes = spikestime.to_python()

    assert np.allclose(simulation_spikes, ref_spikes)


def test_nrntest_test_2():
    h = hoc.HocObject()

    h(
        """
    create axon, soma, dend[3]
    connect axon(0), soma(0)
    for i=0, 2 connect dend[i](0), soma(1)
    axon { nseg = 5  L=100 diam = 1 insert hh }
    forsec "dend" { nseg = 3 L=50 diam = 2 insert pas e_pas = -65 }
    soma { L=10 diam=10 insert hh }
    """
    )

    assert h.axon.name() == "axon"
    assert str(h.dend) == "dend[?]"
    assert str(h.dend[1]) == "dend[1]"
    assert str(h.dend[1].name()) == "dend[1]"

    def e(stmt):
        err = 0
        try:
            exec(stmt, globals(), locals())
        except Exception:
            assert "not assignable" in str(sys.exc_info()[1])
            err = 1
        assert err == 1

    e("h.axon = 1")
    e("h.dend[1] = 1")

    assert str(h) == "<TopLevelHocInterpreter>"
    assert str(h.axon) == "axon"
    assert str(h.axon.name()) == "axon"
    assert str(h.axon(0.5)) == "axon(0.5)"
    assert str(h.axon(0.5).hh) == "hh"
    assert str(h.axon(0.5).hh.name()) == "hh"
    assert h.axon(0.5).hh.gnabar == 0.12
    assert h.axon(0.5) in h.axon
    assert h.axon(0.5) not in h.soma


def test_newobject_err_recover():
    err = 0
    try:
        nc = h.NetCon(5, 12)  # raises error
    except:
        err = 1
    assert err == 1
    h.finitialize()  # succeeds without seg fault


def test_push_section():
    h("""create hCable1, hCable2""")
    h.push_section("hCable1")
    assert h.secname() == "hCable1"
    h.pop_section()
    h.push_section("hCable2")
    assert h.secname() == "hCable2"
    h.pop_section()
    h.delete_section(sec=h.hCable1)
    h.delete_section(sec=h.hCable2)

    sections = [h.Section(name="pCable%d" % i) for i in range(2)]
    sections.append(h.Section())  # Anonymous in the past
    for sec in sections:
        name_in_hoc = h.secname(sec=sec)
        h.push_section(name_in_hoc)
        assert h.secname() == name_in_hoc
        h.pop_section()
    s = sections[-1]
    h.delete_section(sec=s)  # but not yet freed (though the name is now "")
    # not [no longer] a section pointer
    expect_err(
        "s.hoc_internal_name()"
    )  # this is what is generating the error in the next statement.
    expect_err('h.push_section(int(s.hoc_internal_name().replace("__nrnsec_", ""), 0))')

    # not a sectionname
    expect_hocerr(h.push_section, ("not_a_sectionname",))


def test_nonvint_block_supervisor():
    # making sure we can import
    from neuron import nonvint_block_supervisor

    # and can still use threads (no registered callbacks)
    pc = h.ParallelContext()
    pc.nthread(2)
    h.finitialize()
    # but if there is a callback it will fail with threads
    nonvint_block_supervisor.register([None for _ in range(11)])
    expect_hocerr(h.finitialize, (-65,))
    # but can be cleared
    nonvint_block_supervisor.clear()
    h.finitialize()
    pc.nthread(1)


# Before this test was introduced, HOC Object deletion was deferred
# when a Python HocObject was destoyed.
# Now deferred deletion is avoided for this case.
h(
    """
begintemplate Cell
    objref all
    create soma[1], dend[1]
    proc init() {
        all = new SectionList()
        soma {all.append()}
        dend {all.append()}
    }
endtemplate Cell
"""
)


def test_HocObject_no_deferred_unref():
    # previous tests should really destroy the model they created
    for sec in h.allsec():
        h.delete_section(sec=sec)

    sl = h.SectionList()
    cell = h.Cell()
    assert len(list(cell.all)) == 2

    del cell

    # When deferred deletion was in effect, following for loop  would segfault
    # because actual deletion of the HOC Cell object would not occur til the next
    # h.functioncall(). I.e. the first call to sl.append below would destroy
    # the HOC Cell which would destroy the two Sections and invalidate the
    # iterator causing a segfault.
    for sec in h.allsec():
        print(sec)
        sl.append(sec=sec)
    assert len([s for s in sl]) == 0


def test_deleted_sec():
    for sec in h.allsec():
        h.delete_section(sec=sec)
    s = h.Section()
    s.insert("hh")
    seg = s(0.5)
    ic = h.IClamp(seg)
    mech = seg.hh
    rvlist = [rv for rv in mech]
    vref = seg._ref_v
    gnabarref = mech._ref_gnabar
    sec_methods_ok = set([s.cell, s.name, s.hname, s.same, s.hoc_internal_name])
    sec_methods_chk = set(
        [
            getattr(s, n)
            for n in dir(s)
            if "__" not in n
            and type(getattr(s, n)) == type(s.x3d)
            and getattr(s, n) not in sec_methods_ok
        ]
    )
    seg_methods_chk = set(
        [
            getattr(seg, n)
            for n in dir(seg)
            if "__" not in n and type(getattr(seg, n)) == type(seg.area)
        ]
    )
    mech_methods_chk = set(
        [
            getattr(mech, n)
            for n in dir(mech)
            if "__" not in n and type(getattr(mech, n)) == type(mech.segment)
        ]
    )
    rv_methods_chk = set(
        [
            getattr(rvlist[0], n)
            for n in dir(rvlist[0])
            if "__" not in n and type(getattr(rvlist[0], n)) == type(rvlist[0].name)
        ]
    )
    rv_methods_chk.remove(rvlist[0].name)
    h.delete_section(sec=s)

    for methods in [sec_methods_chk, seg_methods_chk, mech_methods_chk, rv_methods_chk]:
        for m in methods:
            # Most would fail because of no args, but expect a check
            # for valid section first.
            words = str(m).split()
            print("m is " + words[4] + "." + words[2])
            if "name" != words[2]:
                expect_err("m()")

    # Mere printing of an invalid object is not supposed to be an error.
    print(s)
    print(seg)
    print(mech)

    assert str(s) == "<deleted section>"
    assert str(seg) == "<segment of deleted section>"
    assert str(mech) == "<mechanism of deleted section>"
    expect_err("h.sin(0.0, sec=s)")
    expect_err("print(s.L)")
    expect_err("s.L = 100.")
    expect_err("s.nseg")
    expect_err("s.nseg = 5")
    expect_err("print(s.v)")
    expect_err("print(s.orientation())")
    expect_err("s.insert('pas')")
    expect_err("s.insert(h.pas)")
    expect_err("s(.5)")
    expect_err("s(.5).v")
    expect_err("seg.v")
    expect_err("seg._ref_v")
    expect_err("s(.5).v = -65.")
    expect_err("seg.v = -65.")
    expect_err("seg.gnabar_hh")
    expect_err("mech.gnabar")
    expect_err("seg.gnabar_hh = .1")
    expect_err("mech.gnabar = .1")
    expect_err("rvlist[0][0]")
    expect_err("rvlist[0][0] = .1")
    expect_err("for sg in s: pass")
    expect_err("for m in seg: pass")
    expect_err("for r in mech: pass")

    assert s == s
    # See https://github.com/neuronsimulator/nrn/issues/1343
    if sys.version_info >= (3, 8):
        expect_err("dir(s)")
        expect_err("dir(seg)")
        expect_err("dir(mech)")
    assert type(dir(rvlist[0])) == list
    expect_err("help(s)")
    expect_err("help(seg)")
    expect_err("help(mech)")
    help(rvlist[0])  # not an error since it does not access s

    dend = h.Section()
    expect_err("dend.connect(s)")
    expect_err("dend.connect(seg)")

    # Note vref and gnabarref if used may cause segfault or other indeterminate result

    assert ic.get_segment() == None
    assert ic.has_loc() == 0.0
    expect_err("ic.get_loc()")
    expect_err("ic.amp")
    expect_err("ic.amp = .001")
    ic.loc(dend(0.5))
    assert ic.get_segment() == dend(0.5)

    imp = h.Impedance()
    expect_err("imp.loc(seg)")
    expect_err("h.distance(0, seg)")
    expect_hocerr(imp.loc, (seg,))
    expect_hocerr(h.distance, (0, seg))

    del ic, imp, dend
    del vref, gnabarref, rvlist, mech, seg, s
    locals()


def test_disconnect():
    print("test_disconnect")
    for sec in h.allsec():
        h.delete_section(sec=sec)
    h.topology()
    n = 5

    def setup(n):
        sections = [h.Section(name="s%d" % i) for i in range(n)]
        for i, sec in enumerate(sections[1:]):
            sec.connect(sections[i])
        return sections

    sl = setup(n)

    def chk(sections, i):
        print(sections, i)
        h.topology()
        x = len(sections[0].wholetree())
        assert x == i

    chk(sl, n)

    h.disconnect(sec=sl[2])
    chk(sl, 2)

    sl = setup(n)
    sl[2].disconnect()
    chk(sl, 2)

    sl = setup(n)
    expect_err("h.disconnect(sl[2])")
    expect_err("h.delete_section(sl[2])")

    del sl
    locals()


def test_py_alltoall_dict_err():
    pc = h.ParallelContext()
    src = {i: (100 + i) for i in range(2)}
    expect_hocerr(pc.py_alltoall, src, ("hocobj_call error",))


def test_nosection():
    expect_err("h.IClamp(.5)")
    expect_err("h.IClamp(5)")
    s = h.Section()
    expect_err("h.IClamp(5)")
    del s
    locals()


def test_nrn_mallinfo():
    # figure out if ASan or TSan was enabled, see comment in unit_test.cpp
    sanitizers = config.arguments["NRN_SANITIZERS"]
    if "address" in sanitizers or "thread" in sanitizers:
        print("Skipping nrn_mallinfo checks because ASan was enabled")
        return
    assert h.nrn_mallinfo(0) > 0


def test_errorcode():
    import os, sys, subprocess

    process = subprocess.run('nrniv -c "1/0"', shell=True)
    assert process.returncode != 0

    exe = os.environ.get("NRN_PYTHON_EXECUTABLE", sys.executable)
    env = os.environ.copy()
    try:
        env[os.environ["NRN_SANITIZER_PRELOAD_VAR"]] = os.environ[
            "NRN_SANITIZER_PRELOAD_VAL"
        ]
    except:
        pass
    process = subprocess.run(
        [exe, "-c", "from neuron import h; h.sqrt(-1)"], env=env, shell=False
    )
    assert process.returncode != 0


def test_hocObj_error_in_construction():
    # test unref hoc obj when error during construction
    expect_hocerr(h.List, "A")
    expect_hocerr(h.List, h.NetStim())


def test_recording_deleted_node():
    soma = h.Section()
    soma_v = h.Vector().record(soma(0.5)._ref_v)
    del soma
    # Now soma_v is still alive, but the node whose voltage it is recording is
    # dead. The current behaviour is that the record instance is silently deleted in this case
    h.finitialize()


def test_nworker():
    threads_enabled = config.arguments["NRN_ENABLE_THREADS"]
    with parallel_context() as pc:
        # single threaded mode, no workers
        with num_threads(pc, threads=1):
            assert pc.nworker() == 0

        # parallel argument specifies serial execution, no workers
        with num_threads(pc, threads=2, parallel=False):
            assert pc.nworker() == 0

        # two workers if threading was enabled at compile time
        with num_threads(pc, threads=2, parallel=True):
            assert pc.nworker() == 2 * threads_enabled


def test_help():
    # a little fragile in the event we change the docs, but easily fixed
    # checks the main paths for generating docstrings
    assert h.Vector().to_python.__doc__.startswith(
        "Syntax:\n    ``pythonlist = vec.to_python()"
    )
    assert h.Vector().__doc__.startswith("class neuron.hoc.HocObject")
    assert h.Vector.__doc__.startswith("class neuron.hoc.HocObject")
    assert h.finitialize.__doc__.startswith("Syntax:\n    ``h.finiti")
    assert h.__doc__.startswith("\n\nneuron.h\n====")


if __name__ == "__main__":
    set_quiet(False)
    test_soma()
    test_simple_sim()
    test_deleted_sec()
    test_disconnect()
    h.topology()
    h.allobjects()
    test_nosection()
    test_nrn_mallinfo()
    test_help()
