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

import subprocess
import os

from setuptools import Command
from skbuild import setup


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



install_requirements = [
    "PyYAML>=3.13",
    "sympy>=1.3,<1.6",
]

setup(
    name="NMODL",
    version="0.2",
    author="Blue Brain Project",
    author_email="bbp-ou-hpc@groupes.epfl.ch",
    description="NEURON Modelling Language Source-to-Source Compiler Framework",
    long_description="",
    packages=["nmodl"],
    include_package_data=True,
    cmake_minimum_required_version="3.3.0",
    cmake_args=["-DPYTHON_EXECUTABLE=" + sys.executable],
    cmdclass=lazy_dict(
        docs=Docs, doctest=get_sphinx_command, buildhtml=get_sphinx_command,
    ),
    zip_safe=False,
    setup_requires=[
        "jinja2>=2.9.3",
        "jupyter",
        "m2r",
        "mistune<2",  # prevents a version conflict with nbconvert
        "nbconvert<6.0",  # prevents issues with nbsphinx
        "nbsphinx>=0.3.2",
        "pytest>=3.7.2",
        "sphinx-rtd-theme",
        "sphinx>=2.0",
        "sphinx<3.0",  # prevents issue with m2r where m2r uses an old API no more supported with sphinx>=3.0
    ]
    + install_requirements,
    install_requires=install_requirements,
)
