import os
import pytest
import sys

if __name__ == "__main__":
    args = os.environ.get("NRN_PYTEST_ARGS", "").split()
    # pytest-cov does not play nicely with MPI. Each MPI rank will try to
    # collect coverage files from all ranks and write them to the same file,
    # with predictable race conditions and errors. Try and avoid that by
    # telling each rank to write to a different output file. TODO: pytest-cov seems to make life harder here, maybe we should use coverage.py directly?
    os.environ["COVERAGE_FILE"] = ".coverage." + str(os.getpid())
    # Run pytests
    code = pytest.main(args)
    print("run_pytest: exiting with code", code, int(code))
    sys.exit(int(code))
