.. _project:

.. note:: This document contains some best practices regarding project management.
  Adhering to these principles drastically increases the future value of your
  project to the neuroscience community. Reproducibility, replicability,
  transparency and understandability are key pillars of good science.

##################
Project Management
##################

===============
Version Control
===============

Version control is a key aspect to well-maintained and manageable projects. It tracks
every change made to your code and makes each changeset uniquely identifiable.

Currently the most used version control protocol is `git`. And the most common cloud
providers of `git` servers are GitHub, GitLab and BitBucket. See their respective
documentation and make yourself familiar with creating and managing repositories.

Continuous integration
======================

Continuous integration is an important step to assure the quality of your model over
time. It is a collection of steps that is performed every time you update your code, to
test that all components are working and the quality is up to standards you have set.

Your own software and all of the dependencies that you use (including NEURON) 
continually change, and these changes can introduce errors that weren't there before.
Make sure that you set up a continuous integration pipeline with your version control
provider that executes `testing`_, `coverage`_, `documentation`_ and `formatting`_.

===============
Code Guidelines
===============

Modern Python projects follow several well-described standards. The 
`Python Zen <https://www.python.org/dev/peps/pep-0020/#id2>`_ is a farcical collection of
rules of thumb that can help you assess the clarity of your code. A more
formal specification can be found in `PEP8 <https://www.python.org/dev/peps/pep-0008/>`_

.. _formatting:

Several automatic linters and formatters exist that can automatically format your code
every time you commit to your version control. Some examples include `flake8`, 
`autopep8` and `black`. The latter has been used to format NEURON's Python source code.

A good model can be used and adapted by others with minimal understanding of the code
itself. Your code should be organized as a proper library with modules and functions.
No code should be executed at import time, and no model parameters hidden inside code
or stored globally. Instead, make a ``model(param1, param2, ...)`` function that users
can call to run their own variations of the model, consider organizing it into a class
so that others can inherit from it and override parts. Use a ``__main__.py`` file so
that users can execute your model with ``python -m my_project param1 param2 param3``:

.. code-block:: python

  from my_project import model
  import sys
  
  # Run model with the command line
  # parameters, assuming they are all
  # numerical values
  model(*map(float, sys.argv[1:]))

Alternatively you can provide a richer command line interface using setuptools's
`console_scripts
<https://setuptools.readthedocs.io/en/latest/userguide/entry_point.html#console-scripts>`_
entry point and a command line argument parser such as Python's `argparse
<https://docs.python.org/3/library/argparse.html>`_

===================
Directory structure
===================

It is conventional for version controlled projects and Python projects to reserve the
root folder for project configuration and tooling configuration files. Your source code
is placed inside of a folder that is named after your project, conforming to `PEP8 module
naming <https://www.python.org/dev/peps/pep-0008/#package-and-module-names>`_:

.. code-block::

  my-project-root
  ⌞.git
  ⌞.github
  ⌞docs
  ⌞my_project
    ⌞mod
    ⌞morphologies
    ⌞__init__.py
    ⌞__main__.py
    ⌞other_src.py
  ⌞tests
  ⌞pyproject.toml
  ⌞README.md
  ⌞setup.cfg
  ⌞setup.py
  
The advantages of a good directory structure are manifold: new developers know where to
look for code and can contribute faster, most default settings will just work, other 
tools will find your codebase easy to process, ...

================
Python packaging
================

As touched briefly before, put your source code in a folder named after the project.
You can distribute your model as a ``pip`` installable package so that other researchers
have easy access to it, and your model *just works* on other machines. By adding either a
``setup.cfg`` or ``setup.py`` file you can provide some metadata about the project and
upload it to `PyPI <https://pypi.org>`_ from whence they can be pip installed.

See the `setuptools quickstart guide 
<https://setuptools.readthedocs.io/en/latest/userguide/quickstart.html>`_ for the basics
of getting your package out there. To package NEURON specific files such as mod files
and morphologies you can use the ``include_package_data`` and ``package_data`` keywords:

.. code-block:: python

  from setuptools import setup
  
  setup(
      name="my-project",
      version="1.0.0",
      author="Me",
      email="noreply@uni.com",
      packages=["my_project"],
      include_package_data=True,
      package_data={
          "my_project": [
              "mod/*.mod",
              "morphologies/*"
          ]
      },
  )
  
.. note:: 

  There is a 60MB package size limit on PyPI. Also GitHub limits single files
  to 125MB. Do not include large datasets in your Python package or git repository.
  Find alternative hosting providers for scientific data like `ModelDB
  <https://senselab.med.yale.edu/ModelDB/>`_ or the `EBRAINS Knowledge Graph
  <https://kg.ebrains.eu/>`. Use your ``.gitignore`` file to exclude large files in
  your repository.
  
=============
Documentation
=============

Good code is made understandable to others:

* For users:
  * Write `docstrings <https://www.python.org/dev/peps/pep-0257/>` for the public API.
  * Separate and indicate the `public and private API 
    <https://www.python.org/dev/peps/pep-0008/#designing-for-inheritance>`
    so users know which part of the code was intended for them to use (public), and what 
    will break in unexpected ways if they change it (private).
  * Describe all of the parameters of the model in the module level docstring
* For developers:
  * Write developer docs on the conventions used in your project
  * Leave comments in the source code that explain possibly unclear passages

With these in place you can generate automatic documentation using `sphinx` and host them
on a provider such as ReadTheDocs. Users are familiar with the layout of sphinx docs and
have an intuition on how to navigate around in them.

=======
Testing
=======

There are many frameworks available that will run your test suites for you. Some common
examples are Python's own ``unittest`` module, ``pytest`` and ``tox``. Place tests under
a ``tests`` folder.

Make sure that you test for common mistakes in user input validation and to run a few
computationally light scaled down versions of your model and validate the output. If the
tests pass you'll be sure that all of the essential parts of your model work as expected.

Coverage
--------

Several tools exist, such as `coverage <https://pypi.org/project/coverage/>`_, that will
analyse how much over the code was actually tested during the testing process. You can
send these reports to online service providers such as `Codecov.io <https://codecov.io/`_
to analyse and visualize which parts of your model might still contain errors.

======
NEURON
======

List the versions of NEURON for which your model works as a requirement for your project:

.. code-block:: python

  from setuptools import setup
  
  setup(
      name="my-project",
      version="1.0.0",
      author="Me",
      email="noreply@uni.com",
      packages=["my_project"],
      install_requires=["NEURON>=8.0.0"],
  )

Mechanisms
==========

Mechanisms need to be compiled against the user's installation of NEURON, so don't
include the compiled library in your version control. The mod files can be packaged
along with your Python source code, and your project's README.md best contains
instructions how to compile them.

Another good practice is to include a check for the compiled library's existence. For example, on `x86_64` UNIX platforms, you could:

.. code-block:: python

  import os
  
  this_dir = os.path.dirname(__file__)
  lib = os.path.join(this_dir, "mod", "x86_64", "libnrnmech.so"))
  assert os.path.exists(lib), "Mechanism library not found, please compile it."


Another option is to use community developed tools such as `Glia 
<https://pypi.org/project/nrn-glia/>`_ to manage mod files.

