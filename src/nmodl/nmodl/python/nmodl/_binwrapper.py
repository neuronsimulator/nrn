#!/usr/bin/env python
"""
A generic wrapper to access nmodl binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import os
import stat
import sys

if sys.version_info >= (3, 9):
    from importlib.metadata import metadata, PackageNotFoundError
    from importlib.resources import files
else:
    from importlib_metadata import metadata, PackageNotFoundError
    from importlib_resources import files

from find_libpython import find_libpython


def main():
    """Sets the environment to run the real executable (returned)"""

    try:
        metadata("nmodl-nightly")
        print("INFO : Using nmodl-nightly Package (Developer Version)")
    except PackageNotFoundError:
        pass

    NMODL_PREFIX = files("nmodl")
    NMODL_HOME = NMODL_PREFIX / ".data"
    NMODL_BIN = NMODL_HOME / "bin"

    # add libpython*.so path to environment
    os.environ["NMODL_PYLIB"] = find_libpython()

    # add nmodl home to environment (i.e. necessary for nrnunits.lib)
    os.environ["NMODLHOME"] = str(NMODL_HOME)

    # set PYTHONPATH for embedded python to properly find the nmodl module
    os.environ["PYTHONPATH"] = (
        str(NMODL_PREFIX.parent) + ":" + os.environ.get("PYTHONPATH", "")
    )

    exe = NMODL_BIN / os.path.basename(sys.argv[0])
    st = os.stat(exe)
    os.chmod(exe, st.st_mode | stat.S_IEXEC)
    os.execv(exe, sys.argv)
