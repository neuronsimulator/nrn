.. _introduction_to_network_construction:

Introduction to Network Construction 
============

Regardless of whether you write hoc code or use the Network Builder, creating a network model involves the same three basic steps.

1.
    Define the types of cells

2.
    Create each cell in the network

3.
    Connect the cells 

Networks can have one or both of the following cell types. 

.. list-table:: 
   :header-rows: 0

   * - **Biophysical ("real") cell types**
     - 
        have sections, density mechanisms, and synapses. 
        
        The synapses are PointProcesses with a NET_RECEIVE block that affects membrane current (e.g. ExpSyn).

        Encapsulate in a class. Example:

        .. code::
            python
            
            begintemplate Cell
             public soma, E, I
             create soma
             objref E, I
             proc init() {
               soma insert hh
               soma {
                E = new ExpSyn(0.5)
                I = new ExpSyn(0.5)
                I.e = -80
                }
              }
            endtemplate Cell

   * - **Artifical cell types**
     -
        are PointProcesses with a NET_RECEIVE block that calls net_event

        Examples: IntegrateFire, NetStim


Biophysical ("real") model neurons require numerical integration to advance the solution in time. However, artificial neurons have very simple dynamics, so the time at which the next spike will occur can be computed analytically. Since artificial neurons do not need numerical integration, they are computationally extremely efficient.

It's a good idea to use artificial neurons for prototyping networks for two reasons. First, development time is much shorter because these simple model cells are easy to work with. Second, artificial neurons are "discrete event devices," so simulations executed with variable time steps will run many orders of magnitude faster than when biophysical neurons are included. With just a bit of planning and the use of modular programming, it is possible to come up with network implementations that make it relatively easy to substitute biophysical model cells for artificial cells, and vice versa. For a good example of this, see the NEURON source code for the models described in

Brette R, Rudolph M, Carnevale T, Hines M, Beeman D, Bower JM, Diesmann M, Morrison A, et al. (2007) Simulation of networks of spiking neurons: A review of tools and strategies. J Comp Neurosci 23:349-98.

`Source code available from ModelDB via accession number 83319. <https://senselab.med.yale.edu/ModelDB/ShowModel?model=83319#tabs-1>`_