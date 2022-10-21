NEURON 2021 Online Course
=========================

A free online webinar series was offered in twice-weekly meetings from 2021-06-03 to 2021-08-03.

The first half of the course was presented primarily through the use of the NEURON GUI; the second
half was more centered around using NEURON through Python. These two approaches are complementary
and both are used in most lectures.

Many lectures end with associated exercises that are then discussed in the following Q&A sessions.

For those who prefer, this video series may also be accessed as a 
`YouTube playlist <https://www.youtube.com/watch?v=31ioJKfPw_g&list=PLydMjAmHmOmjIMkJkTnP0CzcdRSFwRLOU>`_.

Basic Concepts and GUI
----------------------

Lecture 2021-06-03
##################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/31ioJKfPw_g?rel=0" frameborder="0" allowfullscreen></iframe></center>

Q & A 2021-06-08
################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/aryIh07hJ_k?rel=0" frameborder="0" allowfullscreen></iframe></center>

Branched cells
--------------

Lecture 2021-06-10
##################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/k-CyQp3ZrLI?rel=0" frameborder="0" allowfullscreen></iframe></center>

Q & A part 1 2021-06-15
#######################

Due to technical difficulties, the Q&A was split into two videos.

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/Gt8J5V1JAFo?rel=0" frameborder="0" allowfullscreen></iframe></center>

Q & A part 2 2021-06-15
#######################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/ZCwVaccDjTE?rel=0" frameborder="0" allowfullscreen></iframe></center>



Adding ion channels
-------------------

Lecture 2021-06-17
##################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/eV5dKD_bZ6E?rel=0" frameborder="0" allowfullscreen></iframe></center>

Q & A 2021-06-22
################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/alr7yqj88Kg?rel=0" frameborder="0" allowfullscreen></iframe></center>


Python scripting
----------------

2021-06-24
##########

Starts out with a GUI demo due to a fire alarm; the main Python lecture
begins around 17:25.

Topics:

- Programmer's reference.
- How to run Python scripts (text editors, terminals, Jupyter notebooks)
- Python overview (displaying results, variables, loops, lists and dictionaries, functions, libraries, string formatting, and getting help)
- loading NEURON as a Python library and loading NEURON libraries
- defining sections and their properties (including connections)
- caution: defaults are generally for squid
- using classes for cell types
- visualizing data (morphology, concentrations, voltage, etc)
- inserting ion channels and other mechanisms and setting their parameters
- recording results
- simulating a model
- synapses and networks
- saving and loading data with pandas


.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/ObSHSQCpkoo?rel=0" frameborder="0" allowfullscreen></iframe></center>

2021-06-29
##########

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/9BqJcKiYePM?rel=0" frameborder="0" allowfullscreen></iframe></center>

2021-07-01
##########

Topics:

- neuron.rxd (intra- and extracellular ion/protein/etc dynamics)
- pandas (file storage and databases... e.g. CSV, excel, sqlite3)
- eFEL (electrophys feature extraction library from the BlueBrain project)

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/sQ0fa1YRBR0?rel=0" frameborder="0" allowfullscreen></iframe></center>

.. _scripting_and_morphologies_20210706:

Scripting and Morphologies 2021-07-06
#####################################

Today we solve the exercises from last class, roughly: (1) examine the role of temperature with the Hodgkin-Huxley kinetics and use a database as part of the parameter sweeps; (2) test morphologies to determine suitability for use in simulation; and (3) simulate a morphologically detailed cell. 

(Note: unlike the other videos in this series, this one was prerecorded due to the CNS conference so there is no student interaction.)

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/yte708tOiVI?rel=0" frameborder="0" allowfullscreen></iframe></center>


Networks and numerical methods
------------------------------

2021-07-08
##########

Topics:

- synapses, spike-triggered transmission, :class:`NetCon`
- artificial spiking cells (:class:`IntFire1`, :class:`IntFire2`, :class:`IntFire4`)
- Forward Euler vs Backward Euler vs Crank-Nicholson; fixed step vs variable step

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/r9dxBS_e_RI?rel=0" frameborder="0" allowfullscreen></iframe></center>

.. _parallel-neuron-sims-2021-07-13:

Parallel NEURON simulations 2021-07-13
######################################

Using NEURON's :class:`ParallelContext` object. Run parallel simulations using e.g. 

.. code::

    mpiexec -n 4 python mymodel.py

Starting about halfway through the next video, we apply these techniques to solving the homework exercises, building a network.

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/aw_e5WyT1AQ?rel=0" frameborder="0" allowfullscreen></iframe></center>


Network exercise solutions and discussion 2021-07-15
####################################################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/pRir3mTVtAw?rel=0" frameborder="0" allowfullscreen></iframe></center>


Parallel simulation conclusion, reproducible randomness, more numerical methods 2021-07-20
#########################################################################################

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/n0ZbUnIbpRE?rel=0" frameborder="0" allowfullscreen></iframe></center>


ModelDB exercises 2021-07-27
----------------------------

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/PBrWJ-S_YnM?rel=0" frameborder="0" allowfullscreen></iframe></center>

Scaling, spines, and reading HOC 2021-07-29
-------------------------------------------

We start with a discussion of the implementation of Mainen & Sejnowski 1996 `https://modeldb.yale.edu/2488 <https://modeldb.yale.edu/2488>`_ and implications for reuse. There was a brief discussion about Python based ion channels and working with density mechanisms. About an hour into the video, we turn to how to read HOC, how to use existing HOC libraries from Python (and vice-versa) , and how to gain insight from HOC code to design your own Python libraries.

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/4a-E5aICaVU?rel=0" frameborder="0" allowfullscreen></iframe></center>

Building GUI interfaces, Initialization, and Circuits 2021-08-03
----------------------------------------------------------------

.. raw:: html

    <center><iframe width="560" height="315" src="https://www.youtube.com/embed/E1K5ytVh08I?rel=0" frameborder="0" allowfullscreen></iframe></center>
