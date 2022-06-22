.. _tutorial_2_hybrid_nets:

Tutorial 2 : Making Hybrid Nets
=========

Outline
-------

:ref:`Example of a hybrid network <example_hybrid_network>`

:ref:`Step 1. Define the types of cells <step_1_define_type_of_cell>`

    A. We need a class of cells that can supply afferent spikes.

    B. We need a "motoneuron" or "M cell" class.

        1. :ref:`Specifying the anatomical and biophysical properties of the M cell class <step_1_define_type_of_cell>`

        2. :ref:`Specifying what kinds of synapses can be attached to an M cell <step_1_define_type_of_cell2>`

        3. Placing synapses on the M cell

Step 2. Create each cell in the network

    A. We need a Network Builder

    B. We need an instance of each of our three cell types.

Step 3. Connect the cells.

    A. Network architecture

    B. Parameters

Run a simulation and plot the input and output spike trains

.. toctree::
    :hidden:

    example_hybrid_network.rst
    step_1_define_type_of_cell.rst
    step_1_define_type_of_cell2.rst