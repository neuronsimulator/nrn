.. _guide:how_to_get_started_with_neuron:

.. _id1:

How to get started with NEURON
===============================

What is NEURON?
---------------

NEURON is a simulator for neurons and networks of neurons that runs on your local machine,
in the cloud, or on an HPC cluster.

It is built on biophysical models. NEURON simulates neurons using conductance-based equations
introduced by `Hodgkin and Huxley <https://www.neuronsimulator.org/en/9.0.1/courses/interactive_modeling.html#hodgkin-huxley-cable-equations>`_,
whose work establishes the mathematical foundation for how ion channels drive electrical
activity in model cells.

You can build and simulate models using Python, HOC, and/or NEURON's graphical interface.

Download NEURON
---------------

There are two ways to get NEURON running on your system:

`Download and run <https://www.neuronsimulator.org/en/9.0.1/install/install.html>`_ — the
standard option for most users. Pre-built installers are available for Mac, Windows, and Linux.

`Build from source <https://www.neuronsimulator.org/en/9.0.1/install/developer.html>`_ — for
users who need a custom build, want to contribute to development, or need to integrate NEURON
into a larger software environment.

What can NEURON do?
-------------------

NEURON has been used in hundreds of published studies across computational and experimental
neuroscience. A few examples of what it can do when fully utilized:

- `Mainen and Sejnowski (1996) <https://doi.org/10.1038/382363a0>`_ demonstrated that the
  complex firing patterns seen in cortical neurons can be reproduced by varying the morphology
  of a single cell model built in NEURON.

- `Migliore et al. (2014) <https://doi.org/10.3389/fncom.2014.00050>`_ used NEURON to simulate
  large-scale networks of olfactory neurons, linking network-level activity to perceptual
  phenomena.

- NEURON has also been used to simulate detailed single cell models.
  `Morse et al. (2010) <https://doi.org/10.3389/fncir.2010.00016>`_ investigated how early
  Alzheimer's disease affects the electrical excitability of oblique dendrites in hippocampal
  pyramidal neurons. Using a single, anatomically detailed compartmental model of one CA1
  pyramidal cell, NEURON was used to simulate how amyloid beta blocking A-type potassium
  channels changes the way electrical signals propagate through that cell's dendritic branches.
  The model source code is available on `ModelDB <https://modeldb.science>`_ under accession
  number 87284.

- An exploration of how neurons can function as logic gates was carried out by
  `Adonias et al. (2020) <https://doi.org/10.1109/TNB.2020.2975942>`_. Here, NEURON was used
  to simulate networks of Hodgkin-Huxley neurons connected in configurations that replicate
  AND and OR gate logic operations.

A full list of publications using NEURON is available on the
`Publications page <https://www.neuronsimulator.org/en/9.0.1/publications-using-neuron.html>`_.

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

- `What is NEURON <https://www.neuronsimulator.org/en/9.0.1/guide/what_is_neuron.html>`_ — guide
- `Basic Concepts and GUI <https://www.neuronsimulator.org/en/9.0.1/videos/neuron-course-2021.html#basic-concepts-and-gui>`_ — 2021 course video
- `Using NEURON's GUI to build and simulate cells <https://www.neuronsimulator.org/en/9.0.1/videos/incfcns-tutorial-2022.html#using-neuron-s-gui-to-build-and-simulate-cells>`_ — CNS 2022 video
- `Squid axon model <https://www.neuronsimulator.org/en/9.0.1/courses/interactive_modeling.html>`_ — exercise
- `Introduction to the GUI <https://www.neuronsimulator.org/en/9.0.1/courses/intro_to_gui.html>`_ — exercise

**Comfortable with Python?**

Start here if you have a programming background and want to get something running in code first.

- `What is NEURON <https://www.neuronsimulator.org/en/9.0.1/guide/what_is_neuron.html>`_ — guide
- `Scripting NEURON basics <https://www.neuronsimulator.org/en/9.0.1/tutorials/scripting-neuron-basics.html>`_ — CNS 2022 video
- `NEURON scripting exercises <https://www.neuronsimulator.org/en/9.0.1/courses/neuron_scripting_exercises.html>`_ — exercise
- `Single compartment model with Hodgkin-Huxley mechanism <https://www.neuronsimulator.org/en/9.0.1/courses/using_nmodl_files.html>`_ — exercise

Intermediate
~~~~~~~~~~~~

**New to computational modeling?**

These resources introduce more complex cell models, morphology, and ion channel specification
through guided examples and GUI tools.

- `Branched cells <https://www.neuronsimulator.org/en/9.0.1/videos/neuron-course-2021.html#branched-cells>`_ — 2021 course video
- `Adding ion channels <https://www.neuronsimulator.org/en/9.0.1/videos/neuron-course-2021.html#adding-ion-channels>`_ — 2021 course video
- `Using NMODL to add new biophysical mechanisms <https://www.neuronsimulator.org/en/9.0.1/videos/incfcns-tutorial-2022.html#using-nmodl-to-add-new-biophysical-mechanisms>`_ — CNS 2022 video
- `Working with morphometric data and Import3D <https://www.neuronsimulator.org/en/9.0.1/courses/using_morphometric_data.html>`_ — exercise
- `How to work with neuron morphologies <https://www.neuronsimulator.org/en/9.0.1/guide/bio_faq.html#how-do-i-work-with-neuron-morphologies>`_ — guide
- `Using the CellBuilder GUI <https://www.neuronsimulator.org/en/9.0.1/guide/cellbuilder.html>`_ — guide

**Comfortable with Python?**

These resources cover networks, numerical methods, morphology in code, and scripting more
complex models.

- `Using NMODL files <https://www.neuronsimulator.org/en/9.0.1/courses/using_nmodl_files.html>`_ — exercise
- `Networks and numerical methods <https://www.neuronsimulator.org/en/9.0.1/videos/neuron-course-2021.html#networks-and-numerical-methods>`_ — 2021 course video
- `Numerical methods: accuracy, stability, speed <https://www.neuronsimulator.org/en/9.0.1/videos/incfcns-tutorial-2022.html#numerical-methods-accuracy-stability-speed>`_ — CNS 2022 video
- `Numerical methods exercises <https://www.neuronsimulator.org/en/9.0.1/courses/numerical-methods-exercises.html>`_ — exercise
- `HOC exercises <https://www.neuronsimulator.org/en/9.0.1/courses/hoc_exercises.html>`_ — exercise
- `Ball and Stick 1: Basic cell <https://www.neuronsimulator.org/en/9.0.1/tutorials/ball-and-stick-1.html>`_ — Python tutorial
- `Ball and Stick 2: Build a ring network <https://www.neuronsimulator.org/en/9.0.1/tutorials/ball-and-stick-2.html>`_ — Python tutorial

Advanced
~~~~~~~~

These resources assume familiarity with both the biological concepts and the scripting
environment.

- `Reaction-diffusion simulations <https://www.neuronsimulator.org/en/9.0.1/videos/incfcns-tutorial-2022.html#reaction-diffusion-simulations>`_ — CNS 2022 video
- `Python RXD tutorials <https://www.neuronsimulator.org/en/9.0.1/rxd-tutorials/index.html>`_ — tutorial series
- `MPI and multithreaded parallelization <https://www.neuronsimulator.org/en/9.0.1/courses/mpi_parallelization.html>`_ — exercise
- `Using the Neuroscience Gateway for HPC <https://www.neuronsimulator.org/en/9.0.1/courses/using_nsg_portal.html>`_ — exercise
- `Ball and Stick 3: Extensible network <https://www.neuronsimulator.org/en/9.0.1/tutorials/ball-and-stick-3.html>`_ — Python tutorial
- `Ball and Stick 4: Parallel vs serial mode <https://www.neuronsimulator.org/en/9.0.1/tutorials/ball-and-stick-4.html>`_ — Python tutorial

Going further
~~~~~~~~~~~~~

The above list covers some of the structured courses, videos, and exercises available in this
documentation. There are additional resources worth knowing about as you go further. You can
work through all materials on the
`Documentation <https://www.neuronsimulator.org/en/9.0.1/tutorials/index.html>`_ and
`Courses <https://www.neuronsimulator.org/en/9.0.1/courses/exercises2018.html>`_ pages.

`ModelDB <https://modeldb.science>`_ is a publicly accessible database of published
computational neuroscience models, many of which were built with NEURON. Once you are
comfortable running your own simulations, ModelDB is a great resource for finding, downloading,
and studying real models from the literature. This helps you see how others have approached
modeling problems similar to your own.

What's next?
------------

In this guide we have covered what NEURON is and what it is built on, and how to download it
and get it running. Whether you are approaching NEURON from a biological background or a
programming one, the learning resources above will take you from a single compartment cell all
the way to large-scale multiscale networks. The tools are well documented, the exercises are
hands-on, and the concepts build on each other naturally the further you go.

Welcome to NEURON. Happy simulating.