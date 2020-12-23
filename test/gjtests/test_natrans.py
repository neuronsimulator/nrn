from neuron import h
pc = h.ParallelContext()
rank = pc.id()
nhost = pc.nhost()
pc.nthread(4)

# Want to excercise the internal indexing schemes. So need mpi and threads.
# Random order of sgid calls to source_var. Random order and location
# of sgid source . Random order and location of sgid target. An sgid source
# has random number of targets.

ncell = 100
nsrc = 50 # so that number of sgid pointing to _ref_nai
          # multiple sgid may point to same _ref_nai
ntarget = 200 # so sgid on average sends to ntarget/nsrc targets.
ncell=10
nsrc=5
ntarget=10

h('''
begintemplate Cell
public soma
create soma
proc init() {
  soma.diam = 5
  soma.L = 5
  soma {insert na_ion}
}  
endtemplate Cell
''')

def test_natrans():
  gids = [gid for gid in range(rank, ncell, nhost)]  
  cells = []
  for gid in range(rank, ncell, nhost):
    pc.set_gid2node(gid, rank)
    cells.append(h.Cell())
    pc.cell(gid, h.NetCon(cells[-1].soma(.5)._ref_v, None, sec=cells[-1].soma))
 
  r = h.Random()
  r.Random123(1,1,0)

  #nsrc unique random sgids in range(ncell). Sample without replacement.
  sgids = []
  v = list(range(ncell))
  for i in range(nsrc):
    x = int(r.discunif(0, len(v)-1))
    sgids.append(v[x])
    v.pop(x)
  del v
  print(sgids)
  
  for sgid in sgids:
    if pc.gid_exists(sgid) == 3:
      sec = pc.gid2cell(sgid).soma
      pc.source_var(sec(.5)._ref_nai, sgid, sec=sec)

  #ntarget randomly chosen cells are targets for the nsrc sgids
  #ntarget NaTrans are created
  targets = []
  for itar in range(ntarget):
    gid = int(r.discunif(0, ncell-1))
    sgid = sgids[int(r.discunif(0, nsrc-1))]
    if pc.gid_exists(gid) == 3:
      sec = pc.gid2cell(gid).soma
      target = h.NaTrans(sec(.5))
      targets.append(target)
      pc.target_var(target, target._ref_napre, sgid)
      target.sgid = sgid

  cvode = h.CVode()
  cvode.cache_efficient(1)

  pc.set_maxstep(10)
  pc.setup_transfer()
  
  h.dt = .1
  tstop= 1
  tstop=.1
  def run():
    h.finitialize(-65)
    for sgid in sgids:
      if pc.gid_exists(sgid) == 3:
        sec = pc.gid2cell(sgid).soma
        print("finitialize src %d %g" % (sgid, sec(.5).nai))
        sec(.5).nai = float(sgid)/100.
    tars = h.List("NaTrans")
    for tar in tars:
      print("finitialize tar %d %g" % (tar.sgid, tar.napre))
    pc.psolve(tstop)
    for tar in tars:
      x = tar.sgid/100. if tar.sgid >= 0 else 0
      print("%g %g"%(tar.napre, x))
      assert(tar.napre == x)

  run()

  from neuron import coreneuron
  coreneuron.available = True
  if coreneuron.available:
    coreneuron.enable = True
    coreneuron.cell_permute = 0
    run()

  return cells, gids, sgids, targets

if __name__ == '__main__':
  model = test_natrans()
