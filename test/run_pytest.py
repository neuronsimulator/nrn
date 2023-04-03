import os
import pytest

args = os.environ.get("NRN_PYTEST_ARGS", "").split()
check_coverage = int(os.environ.get("NRN_PYTEST_ENABLE_COVERAGE", "0")) != 0

if check_coverage:
    try:
        import coverage
    except ImportError:
        print(
            "run_pytest: could not import coverage despite having been told to check it"
        )
        check_coverage = False

if check_coverage:
    # data_suffix=True should mean that each MPI rank will write to a file with a unique suffix.
    cov = coverage.Coverage(data_suffix=True)
    cov.start()

# Run pytest. Don't want to import NEURON before here if we can help it.
code = pytest.main(args)
if check_coverage:
    cov.stop()  # stop collecting coverage
    cov.save()  # save the .coverage file

# Finalisation stuff
from neuron import h

# Could do cov.combine() here, but is there any point?
print("run_pytest: exiting with code", code, int(code))
# Using NEURON's quit ensures MPI finalisation, in principle.
h.quit(int(code))
