#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Please create a softlink with the binary name to be called.
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


def _config_exe(exe_name):
    """Sets the environment to run the real executable (returned)"""
    NRN_PREFIX = os.path.join(site.getsitepackages()[0], 'neuron', '.data')
    os.environ["NEURONHOME"] = os.path.join(NRN_PREFIX, 'share/nrn')
    os.environ["NRNHOME"] = NRN_PREFIX
    os.environ["NRNBIN"] = os.path.dirname(__file__)
    _set_default_compiler()
    return os.path.join(NRN_PREFIX, 'bin', exe_name)


if __name__ == '__main__':
    exe = _config_exe(os.path.basename(sys.argv[0]))
    os.execv(exe, sys.argv)
