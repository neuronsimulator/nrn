from neuron import h
pc = h.ParallelContext()


h('''
begintemplate Cell
public soma
create soma
endtemplate Cell
''')

def test_natrans():
  cells = [h.Cell() for _ in range(2)]
  for cell in cells:
    s = cell.soma
    s.diam = 5
    s.L = 5
  cells[0].soma.insert("na_ion")    
  nat = h.natrans(cells[1].soma(.5))

  sid = 1
  nairef = cells[0].soma(.5)._ref_nai
  napreref = nat._ref_napre
  pc.source_var(nairef, sid, sec=cells[0].soma)
  pc.target_var(nat, napreref, sid)

  naivec = h.Vector()
  naivec.record(nairef)
  naprevec = h.Vector()
  naprevec.record(napreref)

  naclmpvec = h.Vector()
  naclmpvec.append(10, 10, 20)
  naclmptvec = h.Vector()
  naclmptvec.append(0, .5, .5)
  naclmpvec.play(nairef, naclmptvec, 1)

  cvode = h.CVode()
  cvode.cache_efficient(1)

  pc.set_maxstep(10)
  pc.setup_transfer()
  
  h.dt = .1
  tstop= 1
  h.finitialize(-65)
  pc.psolve(tstop)
  naprevec_std = naprevec.c()
  naprevec.resize(0)
  assert(naivec.eq(naprevec_std)) # nai transfer works for NEURON

  from neuron import coreneuron
  coreneuron.available = True
  if coreneuron.available:
    coreneuron.enable = True
    h.finitialize(-65)
    pc.psolve(tstop)
    assert(naivec.eq(naprevec_std)) # nai transfer works for CoreNEURON

  return cells, nat, naivec, naprevec_std, naprevec

if __name__ == '__main__':
  model = test_natrans()
  print("nai", model[2], "napre", model[3])
  model[2].printf()
  model[3].printf()
