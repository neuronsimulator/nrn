import sys
from io import StringIO
from neuron import h
pc = h.ParallelContext()
nhost = pc.nhost()

quiet = True

def set_quiet(b):
  global quiet
  old = quiet
  quiet = b
  return old

def printerr(e):
  if not quiet:
    print(e)

def checking(s):
  if not quiet:
    print("CHECKING: " + s)

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

def expect_err(stmt, globals, locals):
  """
  expect_err('stmt', globals(), locals())
  stmt is expected to raise an error
  """
  err = 0
  checking(stmt)
  try:
    exec(stmt, globals, locals)
    printerr("expect_err: no err for-- " + stmt)
  except Exception as e:
    err = 1
    printerr(e)
  assert err
