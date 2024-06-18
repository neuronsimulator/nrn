CoreNEURON
##########

`CoreNEURON <https://github.com/neuronsimulator/nrn/blob/master/src/coreneuron>`_ is a compute engine for the NEURON simulator optimised for both memory usage and computational speed on modern CPU/GPU architectures.
Some of its key goals are to:

* Efficiently simulate large network models
* Reduce memory usage
* Support execution on specialised hardware, GPUs
* Support optimisations such as vectorisation and cache-efficient memory layout (e.g. Structure-Of-Arrays)

CoreNEURON is tightly integrated with NEURON, and can transparently simulate a large number of NEURON models.
Certain conditions that must be met are covered in the :ref:`CoreNEURON compatibility` section.

.. bbcorepointer belongs under coreneuron-preconditions.rst, but I haven't figured out the right magic to get it there and in the TOC correctly.
.. toctree::
   :maxdepth: 3

   compatibility.rst
   installation.rst
   running-a-simulation.rst
   examples.rst
