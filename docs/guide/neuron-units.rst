.. _neuron-units:

What units does NEURON use for current, concentration, etc.?
--------------

If you're using the GUI, you've probably noticed that buttons next to numeric fields generally indicate the units, such as (mV), (nA), (ms) for millivolt, nanoamp, or millisecond.

:ref:`Here's a chart of the units that NEURON uses by default. <units_used_in_neuron>`

If you're writing your own mod files, you can specify what units will be used. For example, you may prefer to work with micromolar or nanomolar concentrations when dealing with intracellular free calcium or other second messengers. You can also define new units. :ref:`See this tutorial <units_tutorial>` to get a better understanding of units in NMODL.

For the terminally curious, here is a copy of the :download:`units.dat <data/units.dat.txt>` file that accompanies one of the popular Linux distributions. Presumably mod file variables should be able to use any of its entries.


