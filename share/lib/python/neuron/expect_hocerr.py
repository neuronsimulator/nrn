import sys
from io import StringIO
from neuron import h
pc = h.ParallelContext()
nhost = pc.nhost()

def expect_hocerr(callable, args, sec=None):
  """
  Execute callable(args) and assert that it generated an error.

  If sec is not None, executes callable(args, sec=sec)
  Skips if nhost > 1 as all hoc_execerror end in MPI_ABORT
  Does not work well with nrniv launch since hoc_execerror messages do not
  pass through sys.stderr.
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
    print("expect_hocerr: %s" % errmes)
  if err == 0:
    print("expect_hocerr: no err for %s%s" % (str(callable), str(args)))
  assert(err)


