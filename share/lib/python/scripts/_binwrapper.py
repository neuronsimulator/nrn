#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import os
import platform
import shutil
import subprocess
import sys
from pkg_resources import working_set
from distutils.ccompiler import new_compiler
from distutils.sysconfig import customize_compiler, get_config_var


def _set_default_compiler():
    """Set (dont overwrite) CC/CXX so that apps dont use the build-time ones"""
    ccompiler = new_compiler()
    customize_compiler(ccompiler)
    # xcrun wrapper must bring all args
    if ccompiler.compiler[0] == 'xcrun':
        ccompiler.compiler[0] = get_config_var("CC")
        ccompiler.compiler_cxx[0] = get_config_var("CXX")
    os.environ.setdefault("CC", ccompiler.compiler[0])
    os.environ.setdefault("CXX", ccompiler.compiler_cxx[0])


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
    os.environ["NRN_PYTHONEXE"] = sys.executable
    os.environ["NRNBIN"] = os.path.dirname(__file__)

    _set_default_compiler()
    return os.path.join(NRN_PREFIX, 'bin', exe_name)


if __name__ == '__main__':
    exe = _config_exe(os.path.basename(sys.argv[0]))

    # special is a now a full-on binary. therefore, we've wrapped around special
    # in order to still be able to configure NEURON environment variables (run above)
    if exe.endswith('special'):
        # point executable to `special.nrn`
        exe = os.path.join(sys.argv[0]+'.nrn')

    # wrap around special for nrnivmodl, since special is now a full-on binary
    #   special >> special.nrn
    #   nrniv > special
    if exe.endswith('nrnivmodl'):
        # use subprocess since execv returns
        subprocess.check_call([exe, *sys.argv[1:]])
        print('Wrapping special binary')
        shutil.move(os.path.join(platform.machine(), 'special'), os.path.join(platform.machine(), 'special.nrn'))
        shutil.copy(shutil.which('nrniv'), os.path.join(platform.machine(), 'special'))
    else:    
        os.execv(exe, sys.argv)
