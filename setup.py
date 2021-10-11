# ***********************************************************************
# Copyright (C) 2018-2019 Blue Brain Project
#
# This file is part of NMODL distributed under the terms of the GNU
# Lesser General Public License. See top-level LICENSE file for details.
# ***********************************************************************

import inspect
import os
import subprocess
import sys

from setuptools import Command
from skbuild import setup

"""
A generic wrapper to access nmodl binaries from a python installation
Please create a softlink with the binary name to be called.
"""
import stat
from pkg_resources import working_set
from pywheel.shim.find_libpython import find_libpython


# Main source of the version. Dont rename, used by Cmake
try:
    v = (
        subprocess.run(["git", "describe", "--tags"], stdout=subprocess.PIPE)
        .stdout.strip()
        .decode()
    )
    __version__ = v[: v.rfind("-")].replace("-", ".") if "-" in v else v
    # allow to override version during development/testing
    if "NMODL_WHEEL_VERSION" in os.environ:
        __version__ = os.environ['NMODL_WHEEL_VERSION']
except Exception as e:
    raise RuntimeError("Could not get version from Git repo") from e


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


class Docs(Command):
    description = "Generate & optionally upload documentation to docs server"
    user_options = []
    finalize_options = lambda self: None
    initialize_options = lambda self: None

    def run(self, *args, **kwargs):
        self.run_command("doctest")
        self.run_command("buildhtml")


def _config_exe(exe_name):
    """Sets the environment to run the real executable (returned)"""

    package_name = "nmodl"

    assert (
        package_name in working_set.by_key
    ), "NMODL package not found! Verify PYTHONPATH"
    NMODL_PREFIX = os.path.join(working_set.by_key[package_name].location, "nmodl")
    NMODL_PREFIX_DATA = os.path.join(NMODL_PREFIX, ".data")
    if sys.platform == "darwin":
        os.environ["NMODL_WRAPLIB"] = os.path.join(
            NMODL_PREFIX_DATA, "libpywrapper.dylib"
        )
    else:
        os.environ["NMODL_WRAPLIB"] = os.path.join(NMODL_PREFIX_DATA, "libpywrapper.so")

    # find libpython*.so in the system
    os.environ["NMODL_PYLIB"] = find_libpython()

    return os.path.join(NMODL_PREFIX_DATA, exe_name)


install_requirements = [
    "PyYAML>=3.13",
    "sympy>=1.3,<1.9",
]


cmake_args = ["-DPYTHON_EXECUTABLE=" + sys.executable]
if "bdist_wheel" in sys.argv:
    cmake_args.append("-DLINK_AGAINST_PYTHON=FALSE")

# For CI, we want to build separate wheel
package_name = "NMODL"
if "NMODL_NIGHTLY_TAG" in os.environ:
    package_name += os.environ["NMODL_NIGHTLY_TAG"]

setup(
    name=package_name,
    version=__version__,
    author="Blue Brain Project",
    author_email="bbp-ou-hpc@groupes.epfl.ch",
    description="NEURON Modeling Language Source-to-Source Compiler Framework",
    long_description="",
    packages=["nmodl"],
    scripts=["pywheel/shim/nmodl", "pywheel/shim/find_libpython.py"],
    include_package_data=True,
    cmake_minimum_required_version="3.15.0",
    cmake_args=cmake_args,
    cmdclass=lazy_dict(
        docs=Docs, doctest=get_sphinx_command, buildhtml=get_sphinx_command,
    ),
    zip_safe=False,
    setup_requires=[
        "jinja2>=2.9.3",
        "jupyter-client<7", # try and work around: TypeError in notebooks/nmodl-kinetic-schemes.ipynb: 'coroutine' object is not subscriptable
        "jupyter",
        "m2r",
        "mistune<2",  # prevents a version conflict with nbconvert
        "nbconvert<6.0",  # prevents issues with nbsphinx
        "nbsphinx>=0.3.2",
        "pytest>=3.7.2",
        "sphinx>=2.0",
        "sphinx<3.0",  # prevents issue with m2r where m2r uses an old API no more supported with sphinx>=3.0
        "sphinx-rtd-theme",
    ]
    + install_requirements,
    install_requires=install_requirements,
)
