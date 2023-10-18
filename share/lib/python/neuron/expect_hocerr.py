import sys
from io import StringIO
import inspect
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
    err = False
    pyerrmes = False
    try:
        if sec:
            callable(*args, sec=sec)
        else:
            callable(*args)
            printerr("expect_hocerr: no err for %s%s" % (str(callable), str(args)))
    except Exception as e:
        err = True
        errmes = my_stderr.getvalue()
        if errmes:
            errmes = errmes.splitlines()[0]
            errmes = errmes[(errmes.find(":") + 2) :]
            printerr("expect_hocerr: %s" % errmes)
        elif e:
            printerr(e)
    finally:
        sys.stderr = original_stderr
    assert err


def expect_err(stmt):
    """
    expect_err('stmt')
    stmt is expected to raise an error
    """
    here = inspect.currentframe()
    caller = here.f_back
    err = False
    checking(stmt)
    try:
        exec(stmt, caller.f_globals, caller.f_locals)
        printerr("expect_err: no err for-- " + stmt)
    except Exception as e:
        err = True
        printerr(e)
    assert err
