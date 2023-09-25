"""
Utilities for writing tests
"""
from contextlib import contextmanager


@contextmanager
def cvode_enabled(enabled):
    """
    Helper for tests to enable/disable CVode, leaving the setting as they found it.

    Yields an instance of CVode if enabled == True, otherwise None.

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
        yield h.CVode() if enabled else None
    finally:
        h.cvode_active(old_status)


@contextmanager
def cvode_use_global_timestep(cv, enabled):
    """
    Test helper. Ensure that CVode uses the global timestep method.

    This handles a special case, namely that when the model consists of
    cells that are not coupled to each other, and there are multiple
    MPI ranks, the global CVode instance on each rank will proceed
    independently, resulting in per-cell results that depend on the
    number of ranks and the distribution of cells across them.

    For example:

    # some old CVode state active here
    with cvode_enabled(True) as cv, cvode_use_global_timestep(cv,True):
        # global timestep method active here, even if the model is just
        # a set of cells that aren't coupled to each other
    # old settings restored here
    """
    if cv is None:
        # CVode not enabled, do nothing
        yield None
    else:
        from neuron import h

        mpi_enabled = h.ParallelContext().nhost() > 1
        old_use_local_dt = cv.use_local_dt()
        cv.use_local_dt(not enabled)
        use_parallel = enabled and mpi_enabled
        if use_parallel:
            # make sure the global timestep method is really global
            cv.use_parallel()
        try:
            yield None
        finally:
            if use_parallel:
                # toggle use_local_dt to clear the use_parallel setting
                cv.use_local_dt(not old_use_local_dt)
            cv.use_local_dt(old_use_local_dt)


@contextmanager
def cvode_use_long_double(cv, enabled):
    """
    Helper for tests to enable/disable long double support in CVode, leaving the setting as they found it.
    """
    if cv is None:
        # CVode not enabled, do nothing
        yield None
    else:
        old_setting = cv.use_long_double()
        cv.use_long_double(enabled)
        try:
            yield None
        finally:
            cv.use_long_double(old_setting)


@contextmanager
def fast_imem(enabled):
    from neuron import h

    cvode = h.CVode()
    old_setting = cvode.use_fast_imem()
    cvode.use_fast_imem(enabled)
    try:
        yield None
    finally:
        cvode.use_fast_imem(old_setting)


@contextmanager
def hh_table_disabled():
    """
    Helper for tests to ensure that the TABLE statement in hh.mod will not be used.
    This is disabled at compile time when CoreNEURON is enabled.
    """
    from neuron import h

    old_value = None
    if hasattr(h, "usetable_hh"):  # pragma: no cover
        # Hard to cover this in a coverage build that has CoreNEURON enabled
        # and thus has the TABLE statement in hh.mod disabled.
        old_value = h.usetable_hh
        h.usetable_hh = False
    try:
        yield None
    finally:
        if old_value is not None:  # pragma: no cover
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
