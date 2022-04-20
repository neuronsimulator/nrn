.. _intro_to_gui:

Introduction to the GUI
=======================


Physical System
---------------
An electrolyte-containing vesicle with a lipid bilayer membrane that has no ion channels.

Conceptual Model
~~~~~~~~~~~~~~~~

A capacitor

.. image:: img/bilayer.gif
    :align: center


Simulation
~~~~~~~~~~

Computational implementation of the conceptual model
++++++++++++++++++++++++++++++++++++++++++++++++++++

We're going to use the CellBuilder to make a cell that has surface area = 100 um2 and no ion channels.

Alternatively, we could do this in HOC

.. code:: 
    hoc
    
    create soma
    . . . plus more code . . .

or in Python

.. code::
    python

    from neuron import h

    soma = h.Section(name = 'soma')
    # . . . plus more code . . .

1. 
    **Start in whatever directory you want to use for this exercise**

    Open a terminal or console (users of Windows versions prior to Windows 10 Anniversary Edition may prefer to use the bash shell in the NEURON program group), then change to that directory via e.g.

        ``cd path_to_your_Desktop/course/intro_to_gui``

2. 
    MORE THINGS HERE