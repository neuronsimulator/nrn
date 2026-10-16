try:
    import coverage

    have_coverage = True
except ImportError:
    have_coverage = False
import os
import pytest
import warnings

if __name__ == "__main__":
    args = os.environ.get("NRN_PYTEST_ARGS", "").split()
    # pytest-cov does not play nicely with MPI. Each MPI rank will try to
    # collect coverage files from all ranks and write them to the same file,
    # with predictable race conditions and errors. Try and avoid that by
    # telling each rank to write to a different output file.
    from neuron import h

    pc = h.ParallelContext()
    if have_coverage:
        os.environ["COVERAGE_FILE"] = ".coverage.pynrn." + str(pc.id())
    # Importing neuron just above causes a warning, which seems hard to avoid
    with warnings.catch_warnings():
        if have_coverage:
            warnings.filterwarnings(
                "ignore",
                category=coverage.exceptions.CoverageWarning,
                message=r"Module neuron was previously imported, but not measured \(module-not-measured\)",
            )
        code = pytest.main(args)
    print("run_pytest: exiting with code", code, int(code))
    h.quit(int(code))
