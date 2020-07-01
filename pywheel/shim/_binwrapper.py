#!/usr/bin/env python
"""
A generic wrapper to access nmodl binaries from a python installation
Please create a softlink with the binary name to be called.
"""

import os
import sys
import stat
from pkg_resources import working_set
from find_libpython import find_libpython

def _config_exe(exe_name):
    """Sets the environment to run the real executable (returned)"""

    package_name = 'nmodl'

    assert package_name in working_set.by_key, "NMODL package not found! Verify PYTHONPATH"
    NMODL_PREFIX = os.path.join(working_set.by_key[package_name].location, 'nmodl')
    NMODL_PREFIX_DATA = os.path.join(NMODL_PREFIX, '.data')
    if sys.platform == "darwin" :
        os.environ["NMODL_WRAPLIB"] = os.path.join(NMODL_PREFIX_DATA, 'libpywrapper.dylib')
    else:
        os.environ["NMODL_WRAPLIB"] = os.path.join(NMODL_PREFIX_DATA, 'libpywrapper.so')

    # find libpython*.so in the system
    os.environ["NMODL_PYLIB"] = find_libpython()

    return os.path.join(NMODL_PREFIX_DATA, exe_name)

if __name__ == '__main__':
    """Set the pointed file as executable"""
    exe = _config_exe(os.path.basename(sys.argv[0]))
    st = os.stat(exe)
    os.chmod(exe, st.st_mode | stat.S_IEXEC)
    os.execv(exe, sys.argv)
