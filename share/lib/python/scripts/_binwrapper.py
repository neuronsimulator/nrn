#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import os
import sys
from pkg_resources import working_set
from distutils import ccompiler, sysconfig


def _set_default_compiler():
    """Set (dont overwrite) CC/CXX so that apps dont use the build-time ones"""
    cc = ccompiler.new_compiler()
    sysconfig.customize_compiler(cc)
    # xcrun wrapper must bring all args
    if cc.compiler[0] == 'xcrun':
        cc.compiler[0] = sysconfig.get_config_var("CC")
        cc.compiler_cxx[0] = sysconfig.get_config_var("CXX")
    os.environ.setdefault("CC", cc.compiler[0])
    os.environ.setdefault("CXX", cc.compiler_cxx[0])


def _config_exe(exe_name):
    """Sets the environment to run the real executable (returned)"""

    package_name = 'neuron'
    if package_name not in working_set.by_key:
        print ("INFO : Using neuron-nightly Package (Developer Version)")
        package_name = 'neuron-nightly'

    assert package_name in working_set.by_key, "NEURON package not found! Verify PYTHONPATH"
    NRN_PREFIX = os.path.join(working_set.by_key[package_name].location, 'neuron', '.data')
    os.environ["NEURONHOME"] = os.path.join(NRN_PREFIX, 'share/nrn')
    os.environ["NRNHOME"] = NRN_PREFIX
    os.environ["NRNBIN"] = os.path.dirname(__file__)
    os.environ["NRN_PYTHONHOME"] = sys.prefix
    _set_default_compiler()
    return os.path.join(NRN_PREFIX, 'bin', exe_name)


if __name__ == '__main__':
    exe = _config_exe(os.path.basename(sys.argv[0]))
    os.execv(exe, sys.argv)
