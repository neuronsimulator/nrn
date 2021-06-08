#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import os
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
        print("INFO : Using neuron-nightly Package (Developer Version)")
        package_name = 'neuron-nightly'

    assert package_name in working_set.by_key, "NEURON package not found! Verify PYTHONPATH"
    NRN_PREFIX = os.path.join(working_set.by_key[package_name].location, 'neuron', '.data')
    os.environ["NEURONHOME"] = os.path.join(NRN_PREFIX, 'share/nrn')
    os.environ["NRNHOME"] = NRN_PREFIX
    os.environ["NRN_PYTHONEXE"] = sys.executable
    os.environ["NRNBIN"] = os.path.dirname(__file__)

    _set_default_compiler()
    return os.path.join(NRN_PREFIX, 'bin', exe_name)


def _wrap_executable(output_name):
    """Create a wrapper for an executable in same dir. Requires renaming the original file.
    Executables are typically found under arch_name
    """
    release_dir = os.path.join(os.environ["NEURONHOME"], "demo/release")
    arch_name = next(os.walk(release_dir))[1][0]  # first dir
    file_path = os.path.join(arch_name, output_name)
    shutil.move(file_path, file_path + ".nrn")
    shutil.copy(__file__, file_path)


if __name__ == '__main__':
    exe = _config_exe(os.path.basename(sys.argv[0]))

    if exe.endswith("nrnivmodl"):
        # To create a wrapper for special (so it also gets ENV vars) we intercept nrnivmodl
        subprocess.check_call([exe, *sys.argv[1:]])
        _wrap_executable("special")
        sys.exit(0)

    if exe.endswith("special"):
        exe = os.path.join(sys.argv[0] + '.nrn')  # original special is renamed special.nrn

    os.execv(exe, sys.argv)
