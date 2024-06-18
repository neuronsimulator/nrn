.. _cell_builder:

Using the Cell Builder GUI
==========================

These tutorials show how to use the CellBuilder, a powerful and convenient tool for constructing and managing models of individual neurons. The CellBuilder is a "graphical form for specifying the properties of a model cell." It breaks the job of model specification into a sequence of tasks :

1. Setting up model topology (branching pattern).
2. Grouping sections with shared properties into subsets.
3. Assigning geometric properties (length, diameter) to subsets or individual sections, and specifying a discretization strategy (i.e. how to set nseg).
4. Assigning biophysical properties (Ra, cm, ion channels, buffers, pumps, etc.) to subsets or individual sections.

These are the same things we would have to do, and pretty much the same sequence we'd follow, in order to define a model by writing hoc code. However, the CellBuilder helps us keep it all nice and organized, and eliminates a lot of error-prone typgin  . . . ptyngi  . . . *typing*.

.. toctree::
   :maxdepth: 2
   :caption: The tutorials:

   cellbuilder1
   cellbuilder2
   cellbuilder3

.. You might also be interested in our video on :ref:`???` from the 2021 NEURON course webinar.

