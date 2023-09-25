import math
from neuron import h, gui
from neuron.expect_hocerr import expect_err
from neuron import expect_hocerr
import numpy as np
import os, sys, hashlib

expect_hocerr.quiet = False

from neuron.tests.utils.capture_stdout import capture_stdout
from neuron.tests.utils.checkresult import Chk

# Avoid needing different results depending on NRN_ENABLE_CORENEURON
if hasattr(h, "usetable_hh"):
    h.usetable_hh = False

# Create a helper for managing reference results
dir_path = os.path.dirname(os.path.realpath(__file__))
chk = Chk(os.path.join(dir_path, "test_kschan.json"))

if sys.argv[0].split("/")[-1] == "nrniv":

    def chkstdout(key, capture):
        print(key, " not checked")

else:

    def chkstdout(key, capture):
        chk(key, capture)


def chkpr(key):
    chkstdout(key, capture_stdout("h.ks.pr()", True))


# Cover KSChan::state_consist(int shift) in nrniv/kschan.cpp
h.load_file("chanbild.hoc")

# For checking if a run is doing something useful
# Activate the graphics by uncomment the "# return" statement in
# hrun below. Need to press the "Continue" button after each pause.
# If the graph seems incorrect or unexpected, you can stop by hitting
# (ctrl)C in the terminal window and then pressing the "Continue" button
# From the exception you can determine which hrun you are stopping at.
trec = h.Vector()
vrec = h.Vector()
grph = h.Graph()


def hrun(name, t_tol=0.0, v_tol=0.0, v_tol_per_time=0.0):
    """
    Run the simulation, then compare against reference results.
    The reference results are loaded from the `name` key in test_kschan.json.
    Time values are compared using the relative tolerance `t_tol`.
    Voltage values are linearly interpolated to the reference times, then
    compared using a relative tolerance of v_tol + t * v_tol_per_time.

    The reference files are typically generated with GCC, and the tolerances
    are typically driven by the requirements of NVHPC and oneAPI...
    """
    h.run()
    ref_data = chk.get(name)
    if ref_data is None:
        raise Exception("No reference data for key: " + key)
    ref_tv, ref_vv = ref_data
    new_tv, new_vv = trec.to_python(), vrec.to_python()
    assert len(ref_tv) == len(ref_vv)
    assert len(ref_tv) == len(new_tv)
    assert len(ref_tv) == len(new_vv)
    match = True
    max_diff_t, max_diff_v = 0.0, 0.0
    # Interpolate the new v values to the reference t values
    def interp(new_t, old_t, old_v):
        assert np.all(np.diff(old_t) > 0)
        return np.interp(new_t, old_t, old_v)

    interp_vv = interp(ref_tv, new_tv, new_vv)
    for ref_t, ref_v, new_t, new_v in zip(ref_tv, ref_vv, new_tv, interp_vv):
        if not math.isclose(ref_t, new_t, rel_tol=t_tol):
            diff_t = abs(ref_t - new_t) / max(abs(ref_t), abs(new_t))
            max_diff_t = max(max_diff_t, diff_t)
            print("t diff", ref_t, new_t, diff_t, ">", t_tol)
            match = False
        v_tol_for_this_t = v_tol + v_tol_per_time * ref_t
        if not math.isclose(ref_v, new_v, rel_tol=v_tol_for_this_t):
            diff_v = abs(ref_v - new_v) / max(abs(ref_v), abs(new_v))
            max_diff_v = max(max_diff_v, diff_v)
            print("v diff at t", ref_t, ref_v, new_v, diff_v, ">", v_tol_for_this_t)
            match = False
    if not match:
        print("summary for", name)
    if max_diff_t:
        print("max t diff", max_diff_t)
    if max_diff_v:
        print("max v diff", max_diff_v)
    assert match
    return
    grph.erase()
    trec.printf()
    vrec.printf()
    vrec.line(grph, trec)
    vrec.resize(0)
    trec.resize(0)
    grph.exec_menu("View = plot")
    h.continue_dialog("continue")


def cell():
    s = h.Section(name="soma")
    s.L = 3.18
    s.diam = 10
    s.insert("hh")
    ic = h.IClamp(s(0.5))
    ic.dur = 0.1
    ic.amp = 0.3
    trec.record(h._ref_t, sec=s)
    vrec.record(s(0.5)._ref_v, sec=s)
    return s, ic


def test_1():
    cb = h.ChannelBuild()
    cb.khh()  # HH potassium channel
    s, ic = cell()
    s.insert("khh")  # exists in soma and has one state
    chkstdout("khh inserted", capture_stdout("h.psection()", True))
    # It is not supported (anymore) to change the number of variables
    # of a mechanism while instances of that mechanism are active.
    # In this case the change would be from 1 state to 2 states.
    expect_err("cb.nahh()")  # cb changes name and inserted na_ion before failure
    cb.ks.name("khh")  # change name back
    chkstdout("khh same except for na_ion", capture_stdout("h.psection()", True))
    s.uninsert("khh")
    cb.nahh()  # try again
    s.insert("nahh")
    chkstdout("nahh now", capture_stdout("h.psection()", True))
    chkstdout("cb.ks.pr()", capture_stdout("cb.ks.pr()", True))
    assert cb.ks.ntrans() == 2.0
    assert cb.ks.nstate() == 2.0
    assert cb.ks.ngate() == 2.0
    assert cb.ks.nligand() == 0.0
    assert cb.ks.gate(0).nstate() == 1
    assert cb.ks.gate(0).power() == 3
    assert cb.ks.gate(0).power(3) == 3
    assert cb.ks.gate(0).sindex() == 0
    assert cb.ks.gate(0).index() == 0
    assert cb.ks.trans(1).index() == 1
    assert cb.ks.trans(0).ftype(0) == 3
    assert cb.ks.trans(0).ftype(1) == 2
    assert cb.ks.state(0).gate().index() == 0
    assert cb.ks.state(1).gate().index() == 1
    expect_err("cb.ks.trans(cb.ks.state(0), cb.ks.state(1))")

    # cover interface. Should verify return
    cb.ks.trans(0).ab(h.Vector().indgen(-80, 60, 10), h.Vector(), h.Vector())
    cb.ks.trans(0).inftau(h.Vector().indgen(-80, 60, 10), h.Vector(), h.Vector())
    assert cb.ks.trans(0).f(0, -10) == 3.157187089473768
    assert cb.ks.trans(0).src() == cb.ks.state(0)
    assert cb.ks.trans(0).target() == cb.ks.state(0)
    assert cb.ks.trans(0).parm(0).to_python() == [1.0, 0.1, -40.0]
    assert cb.ks.trans(0).parm(1).to_python() == [4.0, -0.05555555555555555, -65.0]

    expect_err("h.KSState()")  # kss_cons
    expect_err("h.KSGate()")  # ksg_cons
    expect_err("h.KSTrans()")  # kst_cons

    # cover many mechanism runtime interface functions
    h.tstop = 1
    for cvode in [1, 0]:
        h.cvode_active(cvode)
        hrun(
            "nahh cvode={}".format(bool(cvode)),
            t_tol=8e-9 if cvode else 0.0,
            v_tol=2e-9 if cvode else 3e-11,
        )

    s.uninsert("nahh")
    kss = cb.ks.add_hhstate("xxx")
    assert cb.ks.nstate() == 3
    assert kss.name() == "xxx"
    cb.ks.remove_state(kss.index())
    assert cb.ks.nstate() == 2
    kss = cb.ks.add_hhstate("xxx")
    cb.ks.remove_state(kss)
    cb.ks.ion("NonSpecific")

    cb.nahh()
    s.insert("hh")
    hrun("hh", v_tol=3e-11)  # used to be the reference run
    std = (trec.c(), vrec.c())
    # nahh gives same results as sodium channel in hh (usetable_hh is on)
    s.gnabar_hh = 0
    s.insert("nahh")
    s.gmax_nahh = 0.12

    # used to compare to the reference run with 1e-9 tolerance
    hrun("nahh vs hh", v_tol=7e-12)
    # table
    cb.ks.usetable(1)
    # used to compare to the reference run with 0.5 tolerance
    hrun("coarse table", v_tol=1e-11)
    cb.ks.usetable(1, 1000, -80, 60)
    # used to compare to the reference run with 1e-3 tolerance
    hrun("fine table", v_tol=5e-12)
    # cover usetable return info
    vmin = h.ref(0)
    vmax = h.ref(1)
    n = cb.ks.usetable(vmin, vmax)
    assert n == 1000 and vmin[0] == -80 and vmax[0] == 60

    # cover KSChanTable
    cb.ks.usetable(0)
    xvec = h.Vector().indgen(-80, 60, 0.1)
    avec = h.Vector()
    bvec = h.Vector()
    cb.ks.trans(0).ab(xvec, avec, bvec)
    cb.ks.trans(0).set_f(0, 7, avec, xvec[0], xvec[xvec.size() - 1])
    # used to compare to the reference run with 1e-3 tolerance
    hrun("KSChanTable", v_tol=4e-12)
    aref = h.ref(0)
    bref = h.ref(0)
    cb.ks.trans(0).parm(0, aref, bref)
    assert aref[0] == xvec[0] and bref[0] == xvec[xvec.size() - 1]

    # cover some table limit code.
    cb.ks.usetable(1, 200, -50, 30)
    # used to compare to the reference run with 20 tolerance (!)
    hrun("KSChanTable limits", v_tol=2e-12)

    del cb, s, kss, ic, std
    locals()


def mk_khh(chan_name, is_pnt=1):
    # to cover the "shift" fragments. Need a POINT_PROCESS KSChan
    # copy from nrn/share/demo/singhhchan.hoc (concept of shift is
    # obsolete but such a channel is still needed for testing).
    h(
        """
{ ion_register("k", 1) }
objref ks, ksvec, ksgate, ksstates, kstransitions, tobj
{
  ksvec = new Vector()
  ksstates = new List()
  kstransitions = new List()
  ks = new KSChan(%d)
}
// khh0 Point Process (Allow Single Channels)
// k ohmic ion current
//     ik (mA/cm2) = khh0.g * (v - ek)*(0.01/area)
{
  ks.name("%s")
  ks.ion("k")
  ks.iv_type(0)
  ks.gmax(0.036)
  ks.erev(0)
}
// g = gmax * n4
// n4: 5 state, 4 transitions
{
  objref ksgate
  ksstates.append(ks.add_ksstate(ksgate, "n0"))
  ksgate = ksstates.object(0).gate
  ksstates.object(0).frac(0)
  ksstates.append(ks.add_ksstate(ksgate, "n1"))
  ksstates.object(1).frac(0)
  ksstates.append(ks.add_ksstate(ksgate, "n2"))
  ksstates.object(2).frac(0)
  ksstates.append(ks.add_ksstate(ksgate, "n3"))
  ksstates.object(3).frac(0)
  ksstates.append(ks.add_ksstate(ksgate, "n4"))
  ksstates.object(4).frac(1)
}
{
  kstransitions.append(ks.add_transition(ksstates.object(0), ksstates.object(1)))
  kstransitions.append(ks.add_transition(ksstates.object(1), ksstates.object(2)))
  kstransitions.append(ks.add_transition(ksstates.object(2), ksstates.object(3)))
  kstransitions.append(ks.add_transition(ksstates.object(3), ksstates.object(4)))
}
{
  tobj = kstransitions.object(0)
  tobj.type(0)
  tobj.set_f(0, 3, ksvec.c.append(0.4, 0.1, -55))
  tobj.set_f(1, 2, ksvec.c.append(0.125, -0.0125, -65))
}
{
  tobj = kstransitions.object(1)
  tobj.type(0)
  tobj.set_f(0, 3, ksvec.c.append(0.3, 0.1, -55))
  tobj.set_f(1, 2, ksvec.c.append(0.25, -0.0125, -65))
}
{
  tobj = kstransitions.object(2)
  tobj.type(0)
  tobj.set_f(0, 3, ksvec.c.append(0.2, 0.1, -55))
  tobj.set_f(1, 2, ksvec.c.append(0.375, -0.0125, -65))
}
{
  tobj = kstransitions.object(3)
  tobj.type(0)
  tobj.set_f(0, 3, ksvec.c.append(0.1, 0.1, -55))
  tobj.set_f(1, 2, ksvec.c.append(0.5, -0.0125, -65))
}
{ ksstates.remove_all  kstransitions.remove_all }
{ ks.single(%d) }
  """
        % (is_pnt, chan_name, is_pnt)
    )


def test_2():
    print("test_2")
    mk_khh("khh0")
    s, ic = cell()
    kchan = h.khh0(s(0.5))
    chkstdout("kchan with single", capture_stdout("h.psection()", True))
    assert kchan.nsingle(10) == 10.0
    assert kchan.nsingle() == 10.0
    expect_err("h.ks.single(0)")
    chkstdout("kchan failed to turn off single", capture_stdout("h.psection()", True))
    del kchan
    locals()
    h.ks.single(0)
    kchan = h.khh0(s(0.5))
    s.gkbar_hh = 0
    kchan.gmax = 0.036
    chkstdout("kchan without single", capture_stdout("h.psection()", True))
    h.cvode_active(1)
    # At least executes KSChan::mulmat
    hrun(
        "kchan without single cvode=True", t_tol=2e-7, v_tol=1e-11, v_tol_per_time=5e-7
    )
    h.cvode_active(0)

    # location coverage
    assert kchan.has_loc() == True
    assert kchan.get_loc() == 0.5
    h.pop_section()
    assert kchan.get_segment() == s(0.5)
    kchan.loc(s(1))
    assert kchan.get_segment() == s(1)

    # remove transition and state
    del kchan
    locals()
    assert h.ks.nstate() == 5
    assert h.ks.ntrans() == 4
    chkpr("before remove transition")
    h.ks.remove_transition(0)
    chkpr("after remove transition")
    assert h.ks.ntrans() == 3
    h.ks.remove_transition(h.ks.add_transition(0, 1))
    h.ks.add_transition(0, 1)
    h.ks.remove_transition(h.ks.trans(h.ks.state(0), h.ks.state(1)))
    h.ks.remove_state(0)
    assert h.ks.nstate() == 4
    assert h.ks.vres(0.01) == 0.01
    assert h.ks.rseed(10) == 10

    del s, ic
    locals()


def test_3():
    print("test_3")
    # ligand tests (mostly for coverage) start with fresh channel.
    mk_khh("khh2")
    h.ion_register("ca", 2)
    h.ion_register("cl", -1)
    s, ic = cell()

    # replace 1<->2 transition with ligand sensitive transition
    expect_err('h.ks.trans(h.ks.state(1), h.ks.state(2)).type(2, "ca")')
    h.ks.trans(h.ks.state(1), h.ks.state(2)).type(3, "cai")
    chkpr("KSTrans 1<->2 with cai")
    assert h.ks.ligand(0) == "ca_ion"
    assert h.ks.trans(h.ks.state(1), h.ks.state(2)).ligand() == "cai"
    h.ks.trans(h.ks.state(1), h.ks.state(2)).type(2, "cao")
    chkpr("KSTrans 1<->2 change to cao")
    h.ks.trans(h.ks.state(1), h.ks.state(2)).type(3, "cli")
    chkpr("KSTrans 1<->2 change to cli")
    h.ks.trans(h.ks.state(1), h.ks.state(2)).type(0)
    chkpr("KSTrans 1<->2 has no ligand")

    # try for a few more lines of coverage by using ligands for two KSTrans
    h.ks.trans(h.ks.state(1), h.ks.state(2)).type(3, "cai")
    h.ks.trans(h.ks.state(2), h.ks.state(3)).type(3, "cli")

    for cvon in [1, 0]:
        for singleon in [1, 0]:
            h.cvode_active(cvon)
            h.ks.single(singleon)
            kchan = h.khh2(s(0.5))
            t_tol = 0.0
            tols = {}
            if cvon:
                tols["v_tol"] = 1e-12
                if singleon:
                    tols["t_tol"] = 6e-8
                    tols["v_tol_per_time"] = 1e-8
                else:
                    # seems a bit high?
                    tols["t_tol"] = 2e-6
                    tols["v_tol_per_time"] = 6e-8
            else:
                tols["v_tol"] = 8e-11
            hrun(
                "KSTrans cvode={} single={}".format(bool(cvon), bool(singleon)), **tols
            )
            del kchan
            locals()

    h.ks.trans(h.ks.state(2), h.ks.state(3)).type(3, "cai")
    h.ks.trans(h.ks.state(2), h.ks.state(3)).type(0)
    chkpr("bug? cl_ion not used but still ligand 0")
    h.ion_register("u238", 3)
    h.ks.trans(h.ks.state(1), h.ks.state(2)).type(3, "u238i")
    h.ks.trans(h.ks.state(2), h.ks.state(3)).type(2, "u238o")
    h.ks.trans(h.ks.state(1), h.ks.state(2)).type(0)
    h.ks.trans(h.ks.state(2), h.ks.state(3)).type(0)
    chkpr("bug? 4 ligands (cl_ion, 2 u238_ion, ca_ion), none in use")

    del s, ic
    locals()


def test_4():
    print("test_4")
    # KSChan.iv_type tests, mostly for coverage
    mk_khh("khh3")
    kpnt = h.ks
    kpnt.single(0)
    mk_khh("khh4", is_pnt=False)
    kden = h.ks
    s, ic = cell()

    for ivtype in range(3):
        for ion in ["NonSpecific", "k"]:
            kpnt.ion(ion)
            kpnt.iv_type(ivtype)
            kchan = h.khh3(s(0.5))
            kden.ion(ion)
            kden.iv_type(ivtype)
            s.insert("khh4")
            s.gkbar_hh = 0
            if ivtype == 2 and ion == "k":
                s(0.5).khh4.pmax = 0.00025  # works
                assert s.gmax_khh4 == 0.00025
                s.gmax_khh4 = 0.00025  # bug python should know about pmax_khh4
                kchan.pmax = 2.5e-10
            else:
                s.gmax_khh4 = 0.036 / 2.0
                kchan.gmax = 0.036 / 2.0
            hrun("khh4 ivtype={} ion={}".format(ivtype, ion), v_tol=2e-7)
            s.uninsert("khh4")
            del kchan
            locals()

    del s, ic
    locals()


if __name__ == "__main__":
    test_1()
    test_2()
    test_3()
    test_4()

    chk.save()
    print("DONE")
