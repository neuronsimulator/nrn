Developer Builds
================

Developer builds for creating Binary and Python wheel distributions, tests, documentation, code coverage.
Each aspect generally has extra dependencies and special instructions.

**Python versions**

* Source CMake: GIL-enabled CPython **3.10+**. See
  :ref:`NRN_ENABLE_ABI3 <cmake-nrn-enable-abi3>`.
* ``NRN_ENABLE_ABI3=ON`` (default when the default Python is 3.12+, and
  forced for wheels): limited API, ``cp312-abi3`` / ``libnrnpython.abi3``.
  Needs CPython **3.12+**.
* ``NRN_ENABLE_ABI3=OFF`` (default when the default Python is older than
  3.12): versioned ``hoc.cpython-3XY`` and ``libnrnpythonX.Y``.
* Wheels: one ``cp312-abi3`` artifact, tested on 3.12–3.14. Build with
  Python 3.12 (see :doc:`python_wheels`). RxD needs Cython ≥ 3.1.
* Free-threaded CPython is not supported.

CMake option details live in :doc:`../cmake_doc/options`.

**Tests overview**

* In-tree CTest (build directory): enable with
  :ref:`-DNRN_ENABLE_TESTS=ON <cmake-nrn-enable-tests-option>`, then
  ``ctest`` from the main build directory.
* Install / wheel portable suite: target ``test-install`` (same option), or
  configure ``test/foreign`` directly — see that option page and
  ``test/foreign/README.md``.
* Wheel smoke script: ``packaging/python/test_wheels.sh`` (documented under
  :doc:`python_wheels`).
* Coverage workflow: :doc:`code_coverage`.
* Sanitizers / debugging: :doc:`debug`.


.. toctree::
   :maxdepth: 2

   mac_pkg.md
   python_wheels.md
   ci_deps.md
   windows.md
   formatting.md
   code_coverage.md
   debug.md
