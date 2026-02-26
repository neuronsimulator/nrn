tstop variable undefined inside templates
==========================================

By default, variables defined at the top level (outside of templates) are not accessible inside a template. This is why directly using a top-level variable such as ``tstop`` inside a template results in an "undefined variable" error.

To allow a template to access a top-level variable, use the ``external`` keyword inside the template to declare the variable as external.

Example in Hoc:

.. code-block:: hoc

    // Define tstop at top level
    tstop = 100

    begintemplate SE_NetStim
    public pp
    objref pp
    external tstop     // Declare tstop as external to access top-level variable
    proc init() {
      pp = new NetStim()
      pp.interval = $1
      pp.number = $1*tstop*2
      pp.start = $2
      pp.noise = 1
    }
    endtemplate

Example in Python:

.. code-block:: python

    from neuron import h

    h.tstop = 100

    class SE_NetStim:
        def __init__(self, interval, start):
            self.pp = h.NetStim()
            self.interval = interval
            self.start = start
            # Access top-level tstop via h.tstop
            self.pp.interval = interval
            self.pp.number = interval * h.tstop * 2
            self.pp.start = start
            self.pp.noise = 1

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=1724
