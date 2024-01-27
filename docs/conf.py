# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
import os
import sys
import subprocess

# Make translators & domains available for include
sys.path.insert(0, os.path.abspath("./translators"))
sys.path.insert(0, os.path.abspath("./domains"))

import html2  # responsible for creating jump tables in python and hoc documentation
import hocdomain  # Sphinx HOC domain (hacked from the Python domain via docs/generate_hocdomain.py)

# -- Project information -----------------------------------------------------

project = "NEURON"
copyright = "2022, Duke, Yale and the Blue Brain Project"
author = "Michael Hines"

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.autosectionlabel",
    "myst_parser",
    "sphinx.ext.mathjax",
    "nbsphinx",
    "sphinx_design",
]

source_suffix = {
    ".rst": "restructuredtext",
    ".txt": "markdown",
    ".md": "markdown",
}


def setup(app):
    """Setup connect events to the sitemap builder"""
    app.set_translator("html", html2.HTMLTranslator)

    # Set-up HOC domain
    hocdomain.setup(app)


myst_heading_anchors = 3

# Add any paths that contain templates here, relative to this directory.
templates_path = ["_templates"]

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = ["_build", "Thumbs.db", ".DS_Store", "python/venv"]

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = "sphinx_rtd_theme"

# Sphinx expects the master doc to be contents
master_doc = "index"

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named "default.css" will overwrite the builtin "default.css".
html_static_path = ["_static"]

# Extra html content that is generated. i.e. doxygen
html_extra_path = ["_generated"]

html_css_files = [
    "custom.css",
]

# We never execute the notebooks via nbsphinx (for faster local iterations)
# notebooks are executed thanks to the `notebooks` CMake target
nbsphinx_execute = "never"

# Force mathjax@v2 due to plotly requirement
# https://www.sphinx-doc.org/en/master/usage/extensions/math.html#module-sphinx.ext.mathjax
mathjax_path = (
    "https://cdn.jsdelivr.net/npm/mathjax@2/MathJax.js?config=TeX-AMS-MML_HTMLorMML"
)
mathjax2_config = {
    "tex2jax": {
        "inlineMath": [["$", "$"], ["\\(", "\\)"]],
        "processEscapes": True,
        "ignoreClass": "document",
        "processClass": "math|output_area",
    }
}

if os.environ.get("READTHEDOCS"):
    # Get RTD build version ('latest' for master and actual version for tags)
    # Use alias PKGVER to avoid mixin' with sphinx and wasting lots of time on debugging that
    from packaging import version as PKGVER

    rtd_ver = PKGVER.parse(os.environ.get("READTHEDOCS_VERSION"))

    # Install neuron accordingly (nightly for master, otherwise incoming version)
    # Note that neuron wheel must be published a priori.
    subprocess.run(
        "pip install neuron{}".format(
            "=={}".format(rtd_ver.base_version)
            if isinstance(rtd_ver, PKGVER.Version)
            else "-nightly"
        ),
        shell=True,
        check=True,
    )

    # Execute & convert notebooks + doxygen
    subprocess.run("cd .. && python setup.py docs", check=True, shell=True)
