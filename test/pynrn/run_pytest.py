import os
import pytest
import sys

if __name__ == "__main__":
    args = os.environ.get("NRN_PYTEST_ARGS", "").split()
    code = pytest.main(args)
    print("run_pytest: exiting with code", code, int(code))
    sys.exit(int(code))
