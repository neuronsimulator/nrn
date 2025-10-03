What does `$o1` mean in hoc code?
=================================

In hoc, `$o1` refers to the first argument of a function or procedure when that argument is an **object**. The prefix `o` indicates that the argument is an object reference. For scalar arguments, the notation is simply `$1`.

Example in hoc:

.. code-block:: hoc

    proc init() {
        netstimA = $o1  // $o1 is the first object argument passed to init()
    }

Equivalent usage in Python (using NEURON's Python interface) typically involves passing objects directly as arguments:

.. code-block:: python

    def init(netstimA):
        # netstimA is the first argument, typically an object
        pass

This notation helps differentiate between object and scalar arguments within hoc functions or procedures.

Original Thread: https://neuron.yale.edu/phpBB/viewtopic.php?t=3418
