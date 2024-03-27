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

        3. :ref:`Placing synapses on the M cell <step_1_define_type_of_cell3>`

:ref:`Step 2. Create each cell in the network <step_2_create_each_cell>`

    A. We need a Network Builder

    B. We need an instance of each of our three cell types.

:ref:`Step 3. Connect the cells. <step_3_connect_the_cells>`

    A. Network architecture

    B. :ref:`Parameters <step_3_connect_the_cells_continued>`

:ref:`Run a simulation and plot the input and output spike trains <run_simulation_plot_input_output2>`

.. toctree::
    :hidden:

    example_hybrid_network.rst
    step_1_define_type_of_cell.rst
    step_1_define_type_of_cell2.rst
    step_1_define_type_of_cell3.rst
    step_2_create_each_cell.rst
    step_3_connect_the_cells.rst
    step_3_connect_the_cells_continued.rst
    run_simulation_plot_input_output2.rst