#!/usr/bin/env python
"""
A generic wrapper to access nrn binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import os
import shutil
import subprocess
import sys
import warnings
from importlib.metadata import metadata, PackageNotFoundError
from importlib.util import find_spec
from pathlib import Path

from setuptools.command.build_ext import new_compiler
from packaging.version import Version
from sysconfig import get_config_vars, get_config_var
from find_libpython import find_libpython


def _customize_compiler(compiler):
    """Do platform-specific customizations of compilers on unix platforms."""
    if compiler.compiler_type == "unix":
        (cc, cxx, cflags) = get_config_vars("CC", "CXX", "CFLAGS")
        if "CC" in os.environ:
            cc = os.environ["CC"]
        if "CXX" in os.environ:
            cxx = os.environ["CXX"]
        if "CFLAGS" in os.environ:
            cflags = cflags + " " + os.environ["CFLAGS"]
        cc_cmd = cc + " " + cflags
        # We update executables in compiler to take advantage of distutils arg splitting
        compiler.set_executables(compiler=cc_cmd, compiler_cxx=cxx)


def _set_default_compiler():
    """Set (dont overwrite) CC/CXX so that apps dont use the build-time ones"""
    ccompiler = new_compiler()
    _customize_compiler(ccompiler)
    # xcrun wrapper must bring all args
    if ccompiler.compiler[0] == "xcrun":
        ccompiler.compiler[0] = get_config_var("CC")
        ccompiler.compiler_cxx[0] = get_config_var("CXX")
    os.environ.setdefault("CC", ccompiler.compiler[0])
    os.environ.setdefault("CXX", ccompiler.compiler_cxx[0])


def _check_cpp_compiler_version(min_version: str):
    """Check if GCC compiler is >= min supported one, otherwise show warning"""
    try:
        cpp_compiler = os.environ.get("CXX", "")
        version = subprocess.run(
            [cpp_compiler, "--version"],
            stdout=subprocess.PIPE,
        ).stdout.decode("utf-8")
        if "gcc" in version.lower() or "gnu" in version.lower():
            version = subprocess.run(
                [cpp_compiler, "-dumpversion"],
                stdout=subprocess.PIPE,
            ).stdout.decode("utf-8")
            if Version(version) <= Version(min_version):
                warnings.warn(
                    f"Warning: GCC >= {min_version} is required with this version of NEURON"
                    f"but found version {version}",
                )
    except:
        pass


def _config_exe(exe_name):
    """Sets the environment to run the real executable (returned)"""
    try:
        metadata("neuron-nightly")
        print("INFO : Using neuron-nightly Package (Developer Version)")
    except PackageNotFoundError:
        pass

    NRN_PREFIX = str(Path(find_spec("neuron").origin).parent / ".data")

    os.environ["NEURONHOME"] = os.path.join(NRN_PREFIX, "share/nrn")
    os.environ["NRNHOME"] = NRN_PREFIX
    os.environ["CORENRNHOME"] = NRN_PREFIX
    os.environ["NRN_PYTHONEXE"] = sys.executable
    os.environ["CORENRN_PYTHONEXE"] = sys.executable
    os.environ["CORENRN_PERLEXE"] = shutil.which("perl")
    os.environ["NRNBIN"] = os.path.dirname(__file__)

    if "NMODLHOME" not in os.environ:
        os.environ["NMODLHOME"] = NRN_PREFIX
    if "NMODL_PYLIB" not in os.environ:
        os.environ["NMODL_PYLIB"] = find_libpython()

    # nmodl module is inside <prefix>/lib directory
    sys.path.insert(0, os.path.join(NRN_PREFIX, "lib"))
    os.environ["PYTHONPATH"] = ":".join(sys.path)

    _set_default_compiler()
    return os.path.join(NRN_PREFIX, "bin", exe_name)


def _wrap_executable(output_name):
    """Create a wrapper for an executable in same dir. Requires renaming the original file.
    Executables are typically found under arch_name
    """
    release_dir = os.path.join(os.environ["NEURONHOME"], "demo/release")
    arch_name = next(os.walk(release_dir))[1][0]  # first dir
    file_path = os.path.join(arch_name, output_name)
    shutil.move(file_path, file_path + ".nrn")
    shutil.copy(__file__, file_path)


if __name__ == "__main__":
    exe = _config_exe(os.path.basename(sys.argv[0]))

    if exe.endswith("nrnivmodl"):
        # To create a wrapper for special (so it also gets ENV vars) we intercept nrnivmodl
        _check_cpp_compiler_version("10.0")
        subprocess.check_call([exe, *sys.argv[1:]])
        _wrap_executable("special")
        sys.exit(0)

    if exe.endswith("special"):
        exe = os.path.join(
            sys.argv[0] + ".nrn"
        )  # original special is renamed special.nrn

    os.execv(exe, sys.argv)
