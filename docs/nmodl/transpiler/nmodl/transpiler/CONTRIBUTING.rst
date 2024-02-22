Contributing to the NMODL Framework
===================================

We would love for you to contribute to the NMODL Framework and help make
it better than it is today. As a contributor, here are the guidelines we
would like you to follow: - `Question or Problem? <#question>`__ -
`Issues and Bugs <#issue>`__ - `Feature Requests <#feature>`__ -
`Submission Guidelines <#submit>`__ - `Development
Conventions <#devconv>`__

 Got a Question?
----------------

Please do not hesitate to raise an issue on `github project
page <https://github.com/BlueBrain/nmodl>`__.

 Found a Bug?
-------------

If you find a bug in the source code, you can help us by `submitting an
issue <#submit-issue>`__ to our `GitHub
Repository <https://github.com/BlueBrain/nmodl>`__. Even better, you can
`submit a Pull Request <#submit-pr>`__ with a fix.

 Missing a Feature?
-------------------

You can *request* a new feature by `submitting an
issue <#submit-issue>`__ to our GitHub Repository. If you would like to
*implement* a new feature, please submit an issue with a proposal for
your work first, to be sure that we can use it.

Please consider what kind of change it is:

-  For a **Major Feature**, first open an issue and outline your
   proposal so that it can be discussed. This will also allow us to
   better coordinate our efforts, prevent duplication of work, and help
   you to craft the change so that it is successfully accepted into the
   project.
-  **Small Features** can be crafted and directly `submitted as a Pull
   Request <#submit-pr>`__.

 Submission Guidelines
----------------------

 Submitting an Issue
~~~~~~~~~~~~~~~~~~~~

Before you submit an issue, please search the issue tracker, maybe an
issue for your problem already exists and the discussion might inform
you of workarounds readily available.

We want to fix all the issues as soon as possible, but before fixing a
bug we need to reproduce and confirm it. In order to reproduce bugs we
will need as much information as possible, and preferably a sample MOD
file or Python example.

 Submitting a Pull Request (PR)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

When you wish to contribute to the code base, please consider the
following guidelines:

-  Make a `fork <https://guides.github.com/activities/forking/>`__ of
   this repository.

-  Make your changes in your fork, in a new git branch:

   .. code:: shell

      git checkout -b my-fix-branch master

-  Create your patch, **including appropriate test cases**.

-  Enable ``NMODL_TEST_FORMATTING`` CMake variable to ensure that your
   change follows the coding conventions of this project when running
   the tests. The formatting utility can also be used directly:

   -  to format CMake and C++ files:
      ``cmake/hpc-coding-conventions/bin/format``
   -  to format only the C++ files:
      ``cmake/hpc-coding-conventions/bin/format --lang c++``
   -  to format a subset of files or directories:
      ``cmake/hpc-coding-conventions/bin/format src/codegen/ src/main.cpp``
   -  to check the formatting of CMake files:
      ``cmake/hpc-coding-conventions/bin/format --dry-run --lang cmake``

-  Run the full test suite, and ensure that all tests pass.

-  Commit your changes using a descriptive commit message.

   .. code:: shell

      git commit -a

-  Push your branch to GitHub:

   .. code:: shell

      git push origin my-fix-branch

-  In GitHub, send a Pull Request to the ``master`` branch of the
   upstream repository of the relevant component.

-  If we suggest changes then:

   -  Make the required updates.

   -  Re-run the test suites to ensure tests are still passing.

   -  Rebase your branch and force push to your GitHub repository (this
      will update your Pull Request):

      .. code:: shell

          git rebase master -i
          git push -f

That’s it! Thank you for your contribution!

After your pull request is merged
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

After your pull request is merged, you can safely delete your branch and
pull the changes from the main (upstream) repository:

-  Delete the remote branch on GitHub either through the GitHub web UI
   or your local shell as follows:

   .. code:: shell

      git push origin --delete my-fix-branch

-  Check out the master branch:

   .. code:: shell

      git checkout master -f

-  Delete the local branch:

   .. code:: shell

      git branch -D my-fix-branch

-  Update your master with the latest upstream version:

   .. code:: shell

      git pull --ff upstream master

 Development Conventions
------------------------

Formatting
~~~~~~~~~~

Run the HPC coding conventions formatter to format all source files:

.. code:: bash

   cmake/hpc-coding-conventions/bin/format

The HPC coding conventions formatter installs any dependencies into a Python
virtual environment.


Validate the Python package
~~~~~~~~~~~~~~~~~~~~~~~~~~~

You may run the Python test-suites if your contribution has an impact on
the Python API:

1. setup a sandbox environment with either *virtualenv*, *pyenv*, or
   *pipenv*. For instance with *virtualenv*:
   ``python -m venv .venv && source .venv/bin/activate``
2. build the Python package with the command: ``python setup.py build``
3. install *pytest* Python package: ``pip install pytest``
4. execute the unit-tests: ``pytest``

Memory Leaks and clang-tidy
~~~~~~~~~~~~~~~~~~~~~~~~~~~

If you want to test for memory leaks, do :

::

   valgrind --leak-check=full --track-origins=yes  ./bin/nmodl_lexer

Or using CTest as:

::

   ctest -T memcheck

If you want to enable ``clang-tidy`` checks with CMake, make sure to
have ``CMake >= 3.15`` and use following cmake option:

::

   cmake .. -DENABLE_CLANG_TIDY=ON
