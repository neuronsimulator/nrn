from neuron import h, gui
from neuron.expect_hocerr import expect_err

# Cover KSChan::state_consist(int shift) in nrniv/kschan.cpp

h.load_file("chanbild.hoc")
cb = h.ChannelBuild()
cb.khh()  # HH potassium channel
s = h.Section(name="soma")
s.insert("khh")  # exists in soma and has one state
# Generally one does not modify the name/structure of an inserted channel
h.psection()
# now called nahh and has two states (HH sodium channel). it's not
# supported (anymore) to change the number of variables in a mechanism
# while instances of that mechanism are active
expect_err("cb.nahh()")
h.psection()

# to cover the "shift" fragments. Need a POINT_PROCESS KSChan
# copy from nrn/share/demo/singhhchan.hoc
h(
    """
{objref ks, ksvec, ksgate, ksstates, kstransitions, tobj}
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

kchan = h.khh0(s(0.5))
h.psection()
expect_err("h.ks.single(0)")
h.psection()
h.ks.single(1)
