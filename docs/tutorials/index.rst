
Python Tutorials
=================

These tutorials teach you how to build NEURON models in Python, progressing
from the basics to a complete network simulation. If you are new to NEURON,
start with the **Scripting Basics** tutorial, then work through the
**Ball-and-Stick** series.

.. grid:: 1 1 2 2
   :gutter: 3

   .. grid-item-card:: Python & NEURON Basics
      :link: pythontutorial
      :link-type: doc

      A quick introduction for Python users new to NEURON: importing the
      module, creating sections, and running a simulation.

   .. grid-item-card:: Scripting NEURON Basics
      :link: scripting-neuron-basics
      :link-type: doc

      How to run Python scripts with NEURON, access HOC from Python, and
      use NEURON's core objects.

.. toctree::
   :maxdepth: 1
   :caption: Basic Tutorials

   pythontutorial
   scripting-neuron-basics

Ball-and-Stick Tutorial
-----------------------

Build a biophysically detailed neuron model step by step --- from a single
compartment to a multi-cell ring network:

1. **Single compartment** --- create a soma with Hodgkin--Huxley channels
2. **Add dendrite** --- attach a dendrite and observe signal propagation
3. **Instrumentation** --- record, plot, and analyze results
4. **Ring network** --- connect multiple cells into a network

.. toctree::
   :maxdepth: 2
   :caption: Ball-and-Stick Tutorial

   ball-and-stick-1
   ball-and-stick-2
   ball-and-stick-3
   ball-and-stick-4


