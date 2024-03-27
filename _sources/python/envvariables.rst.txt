Environment Variables
=====================

With a bash shell, these are set by, e.g. (on ubuntu)

.. code-block:: shell

    extern NEURONHOME=/where/neuron/was/installed/share/nrn

NEURON_MODULE_OPTIONS
--------------------
  The option arguments allowed when nrniv is launched can be passed to
  the neuron module (``import neuron``) when python is launched by using
  the ``NEURON_MODULE_OPTIONS`` environment variable.
  See the output of ``nrniv -h`` for a list of these options. The relevant
  options are:

  .. code-block:: shell

      -NSTACK integer    size of stack (default 1000)
      -NFRAME integer    depth of function call nesting (default 200)
      -nogui             do not send any gui info to screen
      and all InterViews and X11 options

  Note that the special option ``-print-options`` will cause the neuron module
  to print its options (except for that one) on first import. Also,
  ``h.nrnversion(7)`` will return the effective command line indicating the
  options.

  If an arg appears multiple times, only the first will take effect.

  Example:

  .. code-block:: shell

     export NEURON_MODULE_OPTIONS="-nogui -NFRAME 1000 -NSTACK 10000"
     python -c '
     from neuron import h
     h("""
       func add_recurse() {
         if ($1 == 0) { return 0 }
         return $1 + add_recurse($1 - 1)
       }
     """)
     i = 900
     assert(h.add_recurse(i) == i*(i+1)/2)
     assert("-nogui -NFRAME 1000 -NSTACK 10000" in h.nrnversion(7))
     '

  As the environment variable is only used on the first import of NEURON,
  one can set it from within python by using
  ``os.environ[NEURON_MODULE_OPTIONS] = "..."`` as in

  .. code-block:: python

    import sys
    assert('neuron' not in sys.modules)

    import os
    nrn_options = "-nogui -NSTACK 3000 -NFRAME 525"
    os.environ["NEURON_MODULE_OPTIONS"] = nrn_options
    from neuron import h
    assert(nrn_options in h.nrnversion(7))
