import sys
from io import StringIO
from neuron import h
pc = h.ParallelContext()
nhost = pc.nhost()

quiet = True

def set_quiet(b):
  global quiet
  quiet = b

def printerr(e):
  if not quiet:
    print(e)

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

  original_stderr = sys.stderr
  sys.stderr = my_stderr = StringIO()
  err = 0
  pyerrmes = False
  try:
    if sec:
      callable(*args, sec=sec)
    else:  
      callable(*args)
      printerr("expect_hocerr: no err for %s%s" % (str(callable), str(args)))
  except Exception as e:
    err=1
    errmes = my_stderr.getvalue()
    if errmes:
      errmes = errmes.splitlines()[0]
      errmes = errmes[(errmes.find(':')+2):]
      printerr("expect_hocerr: %s" % errmes)
    elif e:
      printerr(e)
  finally:
    sys.stderr = original_stderr
  assert err


