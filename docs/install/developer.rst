Developer Builds
================

Developer builds for creating Binary and Python wheel distributions, tests, documentation, code coverage.
Each aspect generally has extra dependencies and special instructions.

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
