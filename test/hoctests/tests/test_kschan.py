from neuron import h, gui
from neuron.expect_hocerr import expect_err
from neuron import expect_hocerr
import os

expect_hocerr.quiet = False

from neuron.tests.utils.capture_stdout import capture_stdout
from neuron.tests.utils.checkresult import Chk

# Create a helper for managing reference results
dir_path = os.path.dirname(os.path.realpath(__file__))
chk = Chk(os.path.join(dir_path, "test_kschan.json"))

# Cover KSChan::state_consist(int shift) in nrniv/kschan.cpp

h.load_file("chanbild.hoc")


def test_1():
    cb = h.ChannelBuild()
    cb.khh()  # HH potassium channel
    s = h.Section(name="soma")
    s.insert("khh")  # exists in soma and has one state
    chk("khh inserted", capture_stdout("h.psection()", True))
    # It is not supported (anymore) to change the number of variables
    # of a mechanism while instances of that mechanism are active.
    # In this case the change would be from 1 state to 2 states.
    expect_err("cb.nahh()")  # cb changes name and inserted na_ion before failure
    cb.ks.name("khh")  # change name back
    chk("khh same except for na_ion", capture_stdout("h.psection()", True))
    s.uninsert("khh")
    cb.nahh()  # try again
    s.insert("nahh")
    chk("nahh now", capture_stdout("h.psection()", True))
    chk("cb.ks.pr()", capture_stdout("cb.ks.pr()", True))
    assert cb.ks.ntrans() == 2.0
    assert cb.ks.nstate() == 2.0
    assert cb.ks.ngate() == 2.0
    assert cb.ks.nligand() == 0.0

    expect_err("h.KSState()")  # kss_cons
    expect_err("h.KSGate()")  # ksg_cons
    expect_err("h.KSTrans()")  # kst_cons

    # cover many mechanism runtime interface functions
    h.tstop = 1
    for cvode in [1, 0]:
        h.cvode_active(cvode)
        for cache in [1, 0]:
            h.cvode.cache_efficient(cache)
            h.run()

    del cb, s
    locals()


def mk_khh0():
    # to cover the "shift" fragments. Need a POINT_PROCESS KSChan
    # copy from nrn/share/demo/singhhchan.hoc
    h(
        """
{ ion_register("k", 1) }
objref ks, ksvec, ksgate, ksstates, kstransitions, tobj
{
  ksvec = new Vector()
  ksstates = new List()
  kstransitions = new List()
  ks = new KSChan(1)
}
// khh0 Point Process (Allow Single Channels)
// k ohmic ion current
//     ik (mA/cm2) = khh0.g * (v - ek)*(0.01/area)
{
  ks.name("khh0")
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
{ ks.single(1) }
  """
    )


def test_2():
    mk_khh0()
    s = h.Section(name="soma")
    kchan = h.khh0(s(0.5))
    chk("kchan with single", capture_stdout("h.psection()", True))
    assert kchan.nsingle(10) == 10.0
    assert kchan.nsingle() == 10.0
    expect_err("h.ks.single(0)")
    chk("kchan failed to turn off single", capture_stdout("h.psection()", True))
    del kchan
    locals()
    h.ks.single(0)
    kchan = h.khh0(s(0.5))
    chk("kchan without single", capture_stdout("h.psection()", True))

    # location coverage
    assert kchan.has_loc() == True
    assert kchan.get_loc() == 0.5
    h.pop_section()
    assert kchan.get_segment() == s(0.5)
    kchan.loc(s(1))
    assert kchan.get_segment() == s(1)

    """ bug
  # remove transition and state
  del kchan
  assert h.ks.nstate() == 5
  assert h.ks.ntrans() == 4
  h.ks.remove_transition(0)
  assert h.ks.ntrans() == 3
  h.ks.remove_state(0)
  assert h.ks.nstate() == 4
  """

    del kchan, s
    locals()


if __name__ == "__main__":
    test_1()
    test_2()

    chk.save()
