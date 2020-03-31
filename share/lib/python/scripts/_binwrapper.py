#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Plese create a softlink with the binary name to be called.
"""
import os
import site
import subprocess as sp
import sys
from distutils.ccompiler import new_compiler
from distutils.sysconfig import customize_compiler


def _set_default_compiler():
    """Set (dont overwrite) CC/CXX so that apps dont use the build-time ones"""
    ccompiler = new_compiler()
    customize_compiler(ccompiler)
    os.environ.setdefault("CC", ccompiler.compiler[0])
    os.environ.setdefault("CXX", ccompiler.compiler_cxx[0])


def _launch_command(exe_name):
    NRN_PREFIX = os.path.join(site.getsitepackages()[0], 'neuron', '.data')
    os.environ["NEURONHOME"] = os.path.join(NRN_PREFIX, 'share/nrn')
    os.environ["NRNHOME"] = NRN_PREFIX
    exe_path = os.path.join(NRN_PREFIX, 'bin', exe_name)
    _set_default_compiler()

    return sp.call([exe_path] + sys.argv[1:])


if __name__ == '__main__':
    sys.exit(_launch_command(os.path.basename(sys.argv[0])))
