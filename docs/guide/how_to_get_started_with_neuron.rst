.. _how_to_get_started_with_neuron:

How to get started with NEURON
==============================

What is NEURON?
---------------

NEURON is a simulator for neurons and networks of neurons that runs on your local machine,
in the cloud, or on an HPC cluster.

It is built on biophysical models. NEURON simulates neurons using conductance-based equations
introduced by :ref:`Hodgkin and Huxley <Hodgkin-Huxley cable equations>`, whose work
establishes the mathematical foundation for how ion channels drive electrical activity in
model cells.

You can build and simulate models using Python, HOC, and/or NEURON's graphical interface.

Download NEURON
---------------

There are two ways to get NEURON running on your system:

:doc:`Download and run </install/install>` — the standard option for most users. Pre-built
installers are available for Mac, Windows, and Linux.

:doc:`Build from source </install/developer>` — for users who need a custom build, want to
contribute to development, or need to integrate NEURON into a larger software environment.

What can NEURON do?
-------------------

NEURON has been used in hundreds of published studies across computational and experimental
neuroscience. A few examples of what it can do:

- `Mainen and Sejnowski (1996) <https://modeldb.science/2488>`_ demonstrated that the complex
  firing patterns seen in cortical neurons can be reproduced by varying the morphology of a
  single cell model built in NEURON.

- `Migliore et al. (2014) <https://modeldb.science/151681>`_ used NEURON to simulate
  large-scale networks of olfactory neurons, linking network-level activity to perceptual
  phenomena.

- NEURON has also been used to simulate detailed single cell models.
  `Morse et al. (2010) <https://modeldb.science/87284>`_ investigated how early Alzheimer's
  disease affects the electrical excitability of oblique dendrites in hippocampal pyramidal
  neurons. Using a single, anatomically detailed compartmental model of one CA1 pyramidal
  cell, NEURON was used to simulate how amyloid beta blocking A-type potassium channels
  changes the way electrical signals propagate through that cell's dendritic branches.

A full list of publications using NEURON is available on the
:doc:`Publications page </publications-using-neuron>`.

Learning resources
------------------

The resources below are organized by experience level. Within each level, two starting points
are offered depending on your background — choose the one that fits how you think.

Beginner
~~~~~~~~

**New to computational modeling?**

If you want to understand NEURON through a biological lens, before the programming bit kicks
in, use the following resources. These introduce NEURON through its graphical interface and
biological concepts first.

- :doc:`What is NEURON </guide/what_is_neuron>` — guide
- :ref:`Basic Concepts and GUI` — 2021 course video
- :ref:`Using NEURON's GUI to build and simulate cells` — CNS 2022 video
- :doc:`Squid axon model </courses/interactive_modeling>` — exercise
- :doc:`Introduction to the GUI </courses/intro_to_gui>` — exercise

**Comfortable with Python?**

Start here if you have a programming background and want to get something running in code first.

- :doc:`What is NEURON </guide/what_is_neuron>` — guide
- :doc:`Scripting NEURON basics </tutorials/scripting-neuron-basics>` — CNS 2022 video
- :doc:`NEURON scripting exercises </courses/neuron_scripting_exercises>` — exercise
- :doc:`Single compartment model with Hodgkin-Huxley mechanism </courses/using_nmodl_files>` — exercise

Intermediate
~~~~~~~~~~~~

**New to computational modeling?**

These resources introduce more complex cell models, morphology, and ion channel specification
through guided examples and GUI tools.

- :ref:`Branched cells` — 2021 course video
- :ref:`Adding ion channels` — 2021 course video
- :ref:`Using NMODL to add new biophysical mechanisms` — CNS 2022 video
- :doc:`Working with morphometric data and Import3D </courses/using_morphometric_data>` — exercise
- :ref:`How do I work with neuron morphologies?` — guide
- :doc:`Using the CellBuilder GUI </guide/cellbuilder>` — guide

**Comfortable with Python?**

These resources cover networks, numerical methods, morphology in code, and scripting more
complex models.

- :doc:`Using NMODL files </courses/using_nmodl_files>` — exercise
- :ref:`Networks and numerical methods` — 2021 course video
- :ref:`Numerical Methods: accuracy, stability, speed` — CNS 2022 video
- :doc:`Numerical methods exercises </courses/numerical-methods-exercises>` — exercise
- :doc:`HOC exercises </courses/hoc_exercises>` — exercise
- :doc:`Ball and Stick 1: Basic cell </tutorials/ball-and-stick-1>` — Python tutorial
- :doc:`Ball and Stick 2: Build a ring network </tutorials/ball-and-stick-2>` — Python tutorial

Advanced
~~~~~~~~

These resources assume familiarity with both the biological concepts and the scripting
environment.

- :ref:`Reaction-diffusion simulations` — CNS 2022 video
- :doc:`Python RXD tutorials </rxd-tutorials/index>` — tutorial series
- :doc:`MPI and multithreaded parallelization </courses/mpi_parallelization>` — exercise
- :doc:`Using the Neuroscience Gateway for HPC </courses/using_nsg_portal>` — exercise
- :doc:`Ball and Stick 3: Extensible network </tutorials/ball-and-stick-3>` — Python tutorial
- :doc:`Ball and Stick 4: Parallel vs serial mode </tutorials/ball-and-stick-4>` — Python tutorial

Going further
~~~~~~~~~~~~~

The above list covers some of the structured courses, videos, and exercises available in this
documentation. There are additional resources worth knowing about as you go further. You can
work through all materials on the :doc:`Documentation </tutorials/index>` and
:doc:`Courses </courses/exercises2018>` pages.

`ModelDB <https://modeldb.science>`_ is a publicly accessible database of published
computational neuroscience models, many of which were built with NEURON. Once you are
comfortable running your own simulations, ModelDB is a great resource for finding,
downloading, and studying real models from the literature. This helps you see how others have
approached modeling problems similar to your own.

What's next?
------------

In this guide we have covered what NEURON is and what it is built on, and how to download it
and get it running. Whether you are approaching NEURON from a biological background or a
programming one, the learning resources above will take you from a single compartment cell all
the way to large-scale multiscale networks. The tools are well documented, the exercises are
hands-on, and the concepts build on each other naturally the further you go.

Welcome to NEURON. Happy simulating.