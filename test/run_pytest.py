import os
import pytest
import sys

# Raises ValueError if the wrong syntax is used.
# Expect: [python | nrniv -python ...] run_pytest.py -- X Y Z
# Want to pass X Y Z to pytest.
args = sys.argv[sys.argv.index("--") + 1 :]
check_coverage = int(os.environ.get("NRN_PYTEST_ENABLE_COVERAGE", "0")) != 0

print("run_pytest: args={} check_coverage={}".format(args, check_coverage))

if check_coverage:
    try:
        import coverage
    except ImportError:
        print("run_pytest: could not import coverage, skipping it")
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
