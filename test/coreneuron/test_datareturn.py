# Test of data return covering most of the functionality.

import pytest

from neuron import h
pc = h.ParallelContext()
h.dt = 1.0/32
cvode = h.CVode()

class Cell():
  def __init__(self, gid):
    self.gid = gid
    r = h.Random()
    self.r = r
    r.Random123(gid, 0, 0)
    self.secs = [h.Section(name="s%d"%i, cell = self) for i in range(5)]
    s0 = self.secs[0]

    # random connect to exercise permute
    for i, s in enumerate(self.secs[1:]):
      s.connect(self.secs[int(r.discunif(0, i))])
    
    # hh in first and random passive in rest. 
    s = self.secs[0]
    s.L = 10
    s.diam = 10
    s.insert("hh")
    for s in self.secs[1:]:
      s.nseg = 4
      s.L = 50
      s.diam = 1
      s.insert("pas")
      s.e_pas = -65
      s.g_pas = r.uniform(.0001, .001)

    pc.set_gid2node(gid, pc.id())
    pc.cell(gid, h.NetCon(s0(.5)._ref_v, None, sec=s0))

  def __str__(self):
    return "Cell%d"%self.gid

class Network():
  def __init__(self):
    self.cells = [Cell(i) for i in range(5)]
    cvode.use_fast_imem(1)

  def data(self):
    d = [h.t]
    for sec in h.allsec():
      for seg in sec:
        d.append(seg.v)
        d.append(seg.i_membrane_)
        for mech in seg:
          for var in mech:
            d.append(var[0])

    return d

def test_datareturn():
  from neuron import coreneuron
  coreneuron.enable = False

  model = Network()

  tstop = 5

  def run(tstop):
    pc.set_maxstep(10)
    h.finitialize(-65)
    pc.psolve(tstop)

  run(tstop) # NEURON run
  std = model.data()

  print("CoreNEURON run")
  h.CVode().cache_efficient(1)
  coreneuron.enable = True

  coreneuron.cell_permute = 0
  run(tstop)
  tst = model.data()
  max_unpermuted = h.Vector(std).sub(h.Vector(tst)).abs().max()

  coreneuron.cell_permute = 1
  run(tstop)
  tst = model.data()
  max_permuted = h.Vector(std).sub(h.Vector(tst)).abs().max()

  pc.nthread(2)
  run(tstop)
  tst = model.data()
  max_permuted_thread = h.Vector(std).sub(h.Vector(tst)).abs().max()

  coreneuron.enable = False

  print("max diff unpermuted = %g"% max_unpermuted )
  print("max diff permuted = %g"% max_permuted)
  print("max diff permuted with %d threads = %g"% (pc.nthread(), max_permuted_thread))

  assert(max_unpermuted < 1e-10)
  assert(max_permuted < 1e-10)
  assert(max_permuted_thread < 1e-10)

  if __name__ != "__main__":
    # tear down
    pc.nthread(1)
    pc.gid_clear()
    return None

  return model

if __name__ == "__main__":
  model = test_datareturn()

