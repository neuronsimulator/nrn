# Sum of all i_membrane_ should equal sum of all ElectrodeCurrent
# For a demanding test, use a tree with many IClamp and ExpSyn point processes
# sprinkled on zero and non-zero area nodes.

from neuron import h

class Cell():
  def __init__(self, id, nsec):
    self.id = id
    self.secs = [h.Section(name="d"+str(i), cell=self) for i in range(nsec)]
    r = h.Random()
    r.Random123(id, 0, 0)

    # somewhat random tree, d[0] plays role of soma with connections to
    # d[0](0.5) and all others to 1.0
    for i in range(1, nsec):
      iparent = int(r.discunif(0, i-1))
      x = 0.5 if iparent == 0 else 1.0
      self.secs[i].connect(self.secs[iparent](x))

    # uniform L and diam but somewhat random passive g and e
    for i, sec in enumerate(self.secs):
      sec.L = 10 if i > 0 else 5
      sec.diam = 1 if i > 0 else 5
      sec.insert("pas")
      sec.g_pas = 0.0001*r.uniform(1.0, 1.1)
      sec.e_pas = -65*r.uniform(1.0, 1.1)

    # IClamp and ExpSyn at every location (even duplicates) with random
    # parameters (would rather use a Shunt, but ...)
    self.ics = []
    self.syns = []
    self.netcons = []
    self.netstim = h.NetStim()
    self.netstim.number=1
    self.netstim.start=0.0
    for sec in self.secs:
      for seg in sec.allseg():
        ic = h.IClamp(seg)
        ic.delay = 0.1
        ic.dur = 1.0
        ic.amp = .001*r.uniform(1.0, 1.1)
        self.ics.append(ic)

        syn = h.ExpSyn(seg)
        syn.e = -65*r.uniform(1.0, 1.1)
        syn.tau = r.uniform(0.1, 1.0)
        self.syns.append(syn)
        nc = h.NetCon(self.netstim, syn)
        nc.delay = 0.1
        nc.weight[0] = 0.001*r.uniform(1.0, 1.1)        
        self.netcons.append(nc)

  def __str__(self):
    return "Cell"+str(self.id)

def total_imem():
  imem = 0.0
  for sec in h.allsec():
    for seg in sec.allseg():
      if seg.x == 0.0 and sec.parentseg() is not None:
        assert(seg.i_membrane_ == sec.parentseg().i_membrane_)
        continue # don't count twice
      imem += seg.i_membrane_
  #print("total_imem ", imem)
  return imem
	
def total_iclamp(ics):
  icur = 0.0
  for ic in ics:
    icur += ic.i
  #print("total_iclamp ", icur)
  return icur

# verified that events arrived at syn to change g.
def total_syn_g(syns):
  g = 0
  for syn in syns:
    g += syn.g
  #print("total syn g ", g)
  return g

#Remark:
#  The iteration over all segments (including 0 area nodes) without
#  counting 0 area nodes twice is
'''
    for sec in h.allsec():
     for seg in sec.allseg():
       if seg.x == sec.orientation() and sec.parentseg() is not None:
         continue
       # rest of the "for seg..." body
'''
#  Note that sec.parentseg() is needed to count the root and use of
#  sec.trueparentseg() would miss the root node. Also, although an
#  extremely rare edge case, sec.orientation() is needed to match
#  which segment is closest to root.

def test_allseg_unique_iter():
  a = h.Section('a')
  b = h.Section('b')
  c = h.Section('c')
  d = h.Section('d')
  e = h.Section('e')
  f = h.Section('f')
  g = h.Section('g')
  g1 = h.Section('g1')
  g2 = h.Section('g2')
  g3 = h.Section('g3')
  g4 = h.Section('g4')

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

  #h.topology()

  unique_segs = []
  for sec in h.allsec():
    for seg in sec.allseg():
      if seg.x == sec.orientation() and sec.parentseg() is not None:
        continue
      #print ("%s(%g)" % (sec.name(), seg.x))
      unique_segs.append(seg)

  nseg = sum(sec.nseg for sec in h.allsec())
  nsec = sum(1 for _ in h.allsec())
  ncell = 1
  assert(len(unique_segs) == nseg + nsec + ncell)
  # nothing left out and nothing done twice
  for sec in h.allsec():
    sec.v = 100.
  for seg in unique_segs:
    assert(seg.v == 100.) # nothing done twice
    seg.v = 0.0
  for sec in h.allsec():
    for seg in sec.allseg():
      assert(seg.v == 0.0) # nothing left out

# verified that there is i_membrane_ != 0.0 in zero area nodes.
def print_imem():
  print("t=%g" % h.t)
  for sec in h.allsec():
    for seg in sec.allseg():
      if seg.x == 0.0 and sec.parentseg() is not None:
        continue # don't count twice
      print("%s(%g).i_membrane_ = %g" % (sec.name(), seg.x, seg.i_membrane_))

def balanced(ics, tolerance):
  #print_imem() # helped to verify the test is substantive
  bal = abs(total_imem() - total_iclamp(ics))
  if bal > tolerance:
    print("t=%g bal=%g total_imem=%g total_iclamp=%g" % (h.t, bal, total_imem(), total_iclamp(ics)))
  assert(bal <= tolerance)

def run(tstop, ics, tolerance):
  # to get nontrivial initialized i_membrane_, initialize to random voltage.
  r = h.Random()
  r.Random123(0, 1, 0)
  for sec in h.allsec():
    for seg in sec.allseg():
      # don't care if some segments counted twice
      seg.v = -65. + r.uniform(0, 5)
  h.finitialize()
  balanced(ics, tolerance)
  while h.t < 1.0:
    h.fadvance()
    balanced(ics, tolerance)

def test_fastimem():
  cells = [Cell(id, 10) for id in range(2)]
  #h.topology()
  cvode = h.CVode()
  ics = h.List("IClamp")
  syns = h.List("ExpSyn")
  cvode.use_fast_imem(1)
  h.finitialize(-65)
  run(1.0, ics, 1e-13)
  total_syn_g(syns)
  cvode.active(1)
  run(1.0, ics, 1e-12)
  cvode.use_fast_imem(0)

if __name__ == "__main__":
  test_allseg_unique_iter()
  test_fastimem()
