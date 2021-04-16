#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import os
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
    
    # nrniv -python on macos does not end up with vevn site-packages in sys.path (embedded python aspects?)
    # so we are manually adding it, also making sure to respect the existing $PYTHONPATH 
    if sys.version < '3' and sys.platform == 'darwin':
        import site
        ppath = os.environ['PYTHONPATH'] if 'PYTHONPATH' in os.environ else ''
        for s in site.getsitepackages():
            ppath += ':{}'.format(s)
        os.environ['PYTHONPATH'] = ppath
    
    _set_default_compiler()
    return os.path.join(NRN_PREFIX, 'bin', exe_name)


if __name__ == '__main__':
    exe = _config_exe(os.path.basename(sys.argv[0]))
    os.execv(exe, sys.argv)
