.. _dealing_simulations_generate_lot_data:

Dealing with Simulations that Generate a lot of Data 
=================

Many users have asked how to deal with simulations that generate a lot of data that must be saved. The simplest strategy for saving simulation results is to capture them to Vectors during the simulation, and write them to one or more files after the simulation is complete. This works as long as everything fits into available memory.

If memory limits are exceeded, the first thing to do is go back and decide whether you really need to keep *all* of that stuff. For example, when simulating spiking network models, the only data that must be saved are the spike-gid pairs. Given these it is possible to go back after the end of a simulation and reconstruct the details of activity in a subnet--but that's another story. In this document we presume that you have already selected just those variables that are absolutely essential.

The answer to your problem is to break the simulation into shorter segments, each of which is brief enough to avoid memory overrun, and save the results from each segment before advancing to the next. After the last segment has been executed and its results saved, you can assemble the segmented data into a single record, if you like. Here are a couple of simple examples to help you get started.

Comments and suggestions
-------------

As always, these examples illustrate modular code organization, and iterative refinement and testing of code, both of which are useful strategies for productive programming. If you run into keywords that are new to you, or unfamiliar usage of familiar keywords, be sure to look them up in the :ref:`programmer's reference <python_prog_ref>`.