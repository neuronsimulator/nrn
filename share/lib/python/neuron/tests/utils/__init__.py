"""
Utilities for writing tests
"""
from contextlib import contextmanager


@contextmanager
def cvode_enabled(enabled):
    """
    Helper for tests to enable/disable CVode, leaving the setting as they found it.

    For example:
    from neuron.tests.utils import cvode_enabled
    with cvode_enabled(True):
        # cvode is enabled here
        some_test_using_cvode()
    # cvode is disabled again
    """
    from neuron import h

    old_status = h.cvode_active()
    h.cvode_active(enabled)
    try:
        yield None
    finally:
        h.cvode_active(old_status)


@contextmanager
def cvode_use_long_double(enabled):
    """
    Helper for tests to enable/disable long double support in CVode, leaving the setting as they found it.
    """
    from neuron import h

    cv = h.CVode()
    old_setting = cv.use_long_double()
    cv.use_long_double(enabled)
    try:
        yield None
    finally:
        cv.use_long_double(old_setting)


@contextmanager
def hh_table_disabled():
    """
    Helper for tests to ensure that the TABLE statement in hh.mod will not be used.
    This is disabled at compile time when CoreNEURON is enabled.
    """
    from neuron import h

    old_value = None
    if hasattr(h, "usetable_hh"):
        old_value = h.usetable_hh
        h.usetable_hh = False
    try:
        yield None
    finally:
        if old_value is not None:
            h.usetable_hh = old_value


@contextmanager
def num_threads(pc, threads=1, parallel=True):
    """
    Helper for tests to change the number of threads, leaving the setting as they found it.

    Keyword arguments:
    threads -- the number of NrnThreads to partition the model into
    parallel -- whether to process NrnThreads in parallel, if threading support was enabled
    """
    old_threads = pc.nthread()
    old_workers = pc.nworker()
    old_parallel = False if old_threads > 1 and old_workers == 0 else True
    assert old_workers == 0 or old_workers == old_threads
    pc.nthread(threads, parallel)
    try:
        yield None
    finally:
        pc.nthread(old_threads, old_parallel)


@contextmanager
def parallel_context():
    """
    Helper for tests to get a ParallelContext instance that cleans up after itself.
    """
    from neuron import h

    pc = h.ParallelContext()
    try:
        yield pc
    finally:
        pc.gid_clear()
