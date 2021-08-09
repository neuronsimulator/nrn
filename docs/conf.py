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

# Translators
sys.path.insert(0, os.path.abspath("./translators"))
import html2

# -- Project information -----------------------------------------------------

project = "NEURON"
copyright = "2021, Duke, Yale and the Blue Brain Project"
author = "Michael Hines"

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    "sphinx.ext.autodoc",
    "sphinx.ext.autosummary",
    "sphinx.ext.autosectionlabel",
    "recommonmark",
    "sphinx.ext.mathjax",
]

source_suffix = {
    ".rst": "restructuredtext",
    ".txt": "markdown",
    ".md": "markdown",
}


def setup(app):
    """Setup connect events to the sitemap builder"""
    app.set_translator("html", html2.HTMLTranslator)


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

nbsphinx_allow_errors = True

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
