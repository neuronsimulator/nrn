from neuron import h
pc = h.ParallelContext()

def test_units():
  s = h.Section()
  pp = h.UnitsTest(s(.5))
  h.ion_style("na_ion", 1, 2, 1, 1, 0, sec=s)

  h.finitialize(-65)
  R_std = pp.gasconst  
  erev_std = pp.erev
  ghk_std = pp.ghk

  from neuron import coreneuron
  h.CVode().cache_efficient(1)
  coreneuron.enable = True
  pc.set_maxstep(10)
  h.finitialize(-65)
  pc.psolve(h.dt)

  print("\nR %.17g %.17g" %(pp.gasconst, R_std))
  print("erev %.17g %.17g" % (pp.erev, erev_std))
  print("ghk %.17g %.17g" % (pp.ghk, ghk_std))

  assert(R_std == pp.gasconst) # mod2c needs nrnunits.lib.in
  assert(erev_std == pp.erev)
  assert(ghk_std == pp.ghk)

if __name__ == "__main__":
  test_units()
