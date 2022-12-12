import sys, io, inspect, hashlib


def capture_stdout(stmt, md5=False):
    """
    Some hoc functions print to stdout.
    This provides a way to capture in a string. E.g. for test comparisons.
    e.g.
       s = capture_stdout("h.topology()")
       md5hash = capture_stdout("h.topology()", True)

    unfortunately, does not work when nrniv is launched
    """

    oldstdout = sys.stdout
    sys.stdout = mystdout = io.StringIO()
    here = inspect.currentframe()
    caller = here.f_back
    try:
        exec(stmt, caller.f_globals, caller.f_locals)
    finally:
        sys.stdout = oldstdout
    if md5:
        return hashlib.md5(mystdout.getvalue().encode("utf-8")).hexdigest()
    return mystdout.getvalue()
