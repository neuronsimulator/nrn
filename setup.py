# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

import inspect
import os
import os.path as osp
import platform
import re
import subprocess
import sys
import sysconfig
from distutils.version import LooseVersion

from distutils.cmd import Command
from setuptools import Extension, setup
from setuptools.command.build_ext import build_ext
from setuptools.command.test import test


class lazy_dict(dict):
    """When the value associated to a key is a function, then returns
    the function call instead of the function.
    """

    def __getitem__(self, item):
        value = dict.__getitem__(self, item)
        if inspect.isfunction(value):
            return value()
        return value


def get_sphinx_command():
    """Lazy load of Sphinx distutils command class
    """
    from sphinx.setup_command import BuildDoc

    return BuildDoc


class InstallDoc(Command):
    description = 'Install Sphinx documentation'
    user_options = []
    def initialize_options(self):
        pass
    def finalize_options(self):
        pass

    def run(self):
        self.run_command("test")
        self.run_command("doctest")
        self.run_command("buildhtml")

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = osp.abspath(sourcedir)


class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(["cmake", "--version"])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: "
                + ", ".join(e.name for e in self.extensions)
            )

        cmake_version = LooseVersion(
            re.search(r"version\s*([\d.]+)", out.decode()).group(1)
        )
        if cmake_version < "3.3.0":
            raise RuntimeError("CMake >= 3.3.0 is required")

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = osp.abspath(osp.dirname(self.get_ext_fullpath(ext.name)))
        extdir = osp.join(extdir, ext.name)
        cmake_args = [
            "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=" + extdir,
            "-DPYTHON_EXECUTABLE=" + sys.executable,
        ]

        cfg = "Debug" if self.debug else "Release"
        build_args = ["--config", cfg]

        if platform.system() == "Windows":
            cmake_args += [
                "-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{}={}".format(cfg.upper(), extdir)
            ]
            if sys.maxsize > 2 ** 32:
                cmake_args += ["-A", "x64"]
            build_args += ["--", "/m"]
        else:
            cmake_args += ["-DCMAKE_BUILD_TYPE=" + cfg]
            build_args += ["--", "-j{}".format(max(1, os.cpu_count() - 1))]

        env = os.environ.copy()
        env["CXXFLAGS"] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get("CXXFLAGS", ""), self.distribution.get_version()
        )
        if not osp.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, cwd=self.build_temp, env=env
        )
        subprocess.check_call(
            ["cmake", "--build", "."] + build_args, cwd=self.build_temp
        )

class NMODLTest(test):
    """Custom disutils command that acts like as a replacement
    for the "test" command.

    It first executes the standard "test" command, then runs the
    C++ tests and finally runs the "doctest" to also validate
    code snippets in the sphinx documentation.
    """

    def distutils_dir_name(self, dname):
        """Returns the name of a distutils build directory"""
        dir_name = "{dirname}.{platform}-{version[0]}.{version[1]}"
        return dir_name.format(
            dirname=dname, platform=sysconfig.get_platform(), version=sys.version_info
        )

    def run(self):
        super().run()
        subprocess.check_call(
            [
                "cmake",
                "--build",
                os.path.join("build", self.distutils_dir_name("temp")),
                "--target",
                "test",
            ]
        )


install_requirements = ["jinja2>=2.9", "PyYAML>=3.13", "sympy>=1.3"]

setup(
    name="NMODL",
    version="0.2",
    author="Blue Brain Project",
    author_email="bbp-ou-hpc@groupes.epfl.ch",
    description="NEURON Modelling Language Source-to-Source Compiler Framework",
    long_description="",
    packages=["nmodl"],
    ext_modules=[CMakeExtension("nmodl")],
    cmdclass=lazy_dict(
        build_ext=CMakeBuild,
        test=NMODLTest,
        install_doc=InstallDoc,
        doctest=get_sphinx_command,
        buildhtml=get_sphinx_command,
    ),
    zip_safe=False,
    setup_requires=["nbsphinx>=0.3.2", "m2r", "sphinx-rtd-theme", "sphinx>=2.0"]
    + install_requirements,
    install_requires=install_requirements,
    tests_require=["pytest>=3.7.2"],
)
