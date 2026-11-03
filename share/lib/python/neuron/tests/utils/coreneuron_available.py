# Check if coreneuron is enabled in the current installation
# and loadable at runtime. This may not be the case if we
# build static library of the coreneuron.

from neuron import config, h


def coreneuron_available():
    if not config.arguments["NRN_ENABLE_CORENEURON"]:
        return False
    # But can it be loaded?
    cvode = h.CVode()
    pc = h.ParallelContext()
    h.finitialize()
    result = 0
    import sys
    from io import StringIO

    original_stderr = sys.stderr
    sys.stderr = StringIO()
    try:
        pc.nrncore_run("--tstop 1 --verbose 0")
        result = 1
    except Exception as e:
        pass
    sys.stderr = original_stderr
    return result
