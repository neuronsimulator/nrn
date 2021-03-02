# ParallelContext transfer functionality tests.
# This uses nonsense models and values to verify transfer takes place correctly.

import sys
from io import StringIO

from neuron import h
#h.nrnmpi_init()

pc = h.ParallelContext()
rank = pc.id()
nhost = pc.nhost()

if nhost > 1:
  if rank == 0:
    print("nhost > 1 so calls to expect_error will return without testing.")

def expect_error(callable, args, sec=None):
  """
  Execute callable(args) and assert that it generated an error.

  If sec is not None, executes callable(args, sec=sec)
  """

  if nhost > 1:
    return

  old_stderr = sys.stderr
  sys.stderr = my_stderr = StringIO()
  err = 0
  try:
    if sec:
      callable(*args, sec=sec)
    else:  
      callable(*args)
  except:
    err=1
  errmes =  my_stderr.getvalue()
  sys.stderr = old_stderr
  if errmes:
    errmes = errmes.splitlines()[0]
    errmes = errmes[(errmes.find(':')+2):]
    print("expect_error: %s" % errmes)
  if err == 0:
    print("expect_error: no err for %s%s" % (str(callable), str(args)))
  assert(err)

# Cell with enough nonsense stuff to exercise transfer possibilities.
class Cell():
  def __init__(self):
    self.soma = h.Section(name="soma", cell=self)
    self.soma.insert("na_ion")    # can use nai as transfer source
    # can use POINT_PROCESS range variable as targets
    self.ic = h.IClamp(self.soma(.5))
    self.vc = h.SEClamp(self.soma(.5))
    self.vc.rs = 1e9 # no voltage clamp current

def run():
  pc.setup_transfer()
  h.finitialize()
  h.fadvance()

def teardown():
  pc.gid_clear()

def mkmodel(ncell):
  gids = list(range(rank, ncell, nhost))
  cells = [Cell() for _ in gids]
  nclist = []
  for i, gid in enumerate(gids):
    pc.set_gid2node(gid, rank)
    pc.cell(gid, h.NetCon(cells[i].soma(.5)._ref_v, None, sec=cells[i].soma))
  return (gids, cells)


def test_partrans():
  # no transfer targets or sources.
  model = mkmodel(4)
  run()

  # invalid source or target sid.
  if 0 in model[0]:
    cell = model[1][0]
    s = cell.soma
    expect_error(pc.source_var, (s(.5)._ref_v, -1), sec=s)
    expect_error(pc.target_var, (cell.ic, cell.ic._ref_amp, -1))

  # target with no source.
  if pc.gid_exists(1):
    cell = pc.gid2cell(1)
    pc.target_var(cell.ic, cell.ic._ref_amp, 1)
  expect_error(run, ())
  teardown()
  model = None
  model = mkmodel(4)

  # source with no target (not an error).
  if pc.gid_exists(1):
    cell = pc.gid2cell(1)
    pc.source_var(cell.soma(.5)._ref_v, 1, sec=cell.soma)
  run()
  teardown()
  model = None

  # round robin transfer v to ic.amp and vc.amp1, nai to vc.amp2
  ncell = 5
  model = mkmodel(ncell)
  def transfer1(model):
    for i, gid in enumerate(model[0]):
      cell = model[1][i]
      s = cell.soma
      srcsid = gid
      tarsid = (gid+1)%ncell
      pc.source_var(s(.5)._ref_v, srcsid, sec=s)
      pc.source_var(s(.5)._ref_nai, srcsid+ncell, sec=s)
      pc.target_var(cell.ic, cell.ic._ref_amp, tarsid)
      pc.target_var(cell.ic, cell.vc._ref_amp1, tarsid)
      pc.target_var(cell.vc, cell.vc._ref_amp2, tarsid+ncell)
  transfer1(model)
  def init_values(model):
    for i, gid in enumerate(model[0]):
      c = model[1][i]
      c.soma(.5).v = gid
      c.soma(.5).nai = gid+ncell
      c.ic.amp = 0
      c.vc.amp1 = 0
      c.vc.amp2 = 0
  init_values(model)
  run()
  def check_values(model):
    values = {}
    for i, gid in enumerate(model[0]):
      c = model[1][i]
      vi = c.soma(.5).v
      if (h.ismembrane("extracellular", sec = c.soma)):
        vi += c.soma(.5).vext[0]
      values[gid] = {'v':vi, 'nai':c.soma(.5).nai, 'amp':c.ic.amp, 'amp1':c.vc.amp1, 'amp2':c.vc.amp2}
    x = pc.py_gather(values, 0)
    if rank == 0:
      values = {}
      for v in x:
        values.update(v)
      for gid in range(len(values)):
        print(gid, values[gid])
      ncell = len(values)
      for gid in values:
        v1 = values[gid]
        v2 = values[(gid+ncell-1)%ncell]
        assert(v1['v'] == v2['amp'])
        assert(v1['v'] == v2['amp1'])
        assert(v1['nai'] == v2['amp2'])

  check_values(model)

  # impedance error (number of gap junction not equal to number of pc.transfer_var)
  imp = h.Impedance()
  if rank == 0:
    imp.loc(model[1][0].soma(.5))
  expect_error(imp.compute, (1, 1))
  del imp

  #CoreNEURON gap file generation
  teardown()
  model = None
  model = mkmodel(ncell)
  transfer1(model)
  cvode = h.CVode()
  cvode.cache_efficient(1)
  h.finitialize(-65)
  pc.nrncore_write("tmp")

  # threads
  pc.nthread(2)
  init_values(model)
  run()
  check_values(model)
  pc.nthread(1)

  # extracellular means use v = vm+vext[0]
  for cell in model[1]:
    cell.soma.insert("extracellular")
  init_values(model)
  run()
  check_values(model)

  teardown()

if __name__ == "__main__":
  test_partrans()
  pc.barrier()
  h.quit()
