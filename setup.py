# ***********************************************************************
# Copyright (C) 2018-2022 Blue Brain Project
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
from find_libpython import find_libpython


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
    # If nbconvert is installed to .eggs on the fly when running setup.py then
    # templates from it will not be found. This is a workaround.
    if 'JUPYTER_PATH' not in os.environ:
        import nbconvert
        os.environ['JUPYTER_PATH'] = os.path.realpath(os.path.join(os.path.dirname(nbconvert.__file__), '..', 'share', 'jupyter'))
        print("Setting JUPYTER_PATH={}".format(os.environ['JUPYTER_PATH']))

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


install_requirements = [
    "PyYAML>=3.13",
    "sympy>=1.3",
    "find_libpython"
]


cmake_args = ["-DPYTHON_EXECUTABLE=" + sys.executable]
if "bdist_wheel" in sys.argv:
    cmake_args.append("-DLINK_AGAINST_PYTHON=FALSE")
    cmake_args.append("-DNMODL_ENABLE_TESTS=FALSE")

# For CI, we want to build separate wheel
package_name = "NMODL"
if "NMODL_NIGHTLY_TAG" in os.environ:
    package_name += os.environ["NMODL_NIGHTLY_TAG"]

# Parse long description from README.md
with open('README.md', 'r', encoding='utf-8') as f:
    long_description = f.read()

setup(
    name=package_name,
    version=__version__,
    author="Blue Brain Project",
    author_email="bbp-ou-hpc@groupes.epfl.ch",
    description="NEURON Modeling Language Source-to-Source Compiler Framework",
    long_description=long_description,
    long_description_content_type='text/markdown',
    packages=["nmodl"],
    scripts=["pywheel/shim/nmodl"],
    include_package_data=True,
    cmake_minimum_required_version="3.15.0",
    cmake_args=cmake_args,
    cmdclass=lazy_dict(
        docs=Docs, doctest=get_sphinx_command, buildhtml=get_sphinx_command,
    ),
    zip_safe=False,
    setup_requires=[
        "jinja2>=2.9.3",
        "jupyter-client",
        "jupyter",
        "myst_parser",
        "mistune<3",  # prevents a version conflict with nbconvert
        "nbconvert",
        "nbsphinx>=0.3.2",
        "pytest>=3.7.2",
        "sphinxcontrib-applehelp<1.0.3",
        "sphinxcontrib-htmlhelp<=2.0.0",
        "sphinx<6",
        "sphinx-rtd-theme",
    ]
    + install_requirements,
    install_requires=install_requirements,
)
