"""Count _cxx_compile calls when the process-level cache is absent."""
import atexit
import sys


def pytest_configure(config):
    import neuron.rxd.rxd as m

    if hasattr(m, "_cxx_compile_cache"):
        return
    orig = m._cxx_compile
    n = {"n": 0}

    def wrap(formula):
        n["n"] += 1
        return orig(formula)

    m._cxx_compile = wrap
    atexit.register(
        lambda: print(f"RxD JIT (no cache): {n['n']} compiles, 0 hits", file=sys.stderr)
    )
