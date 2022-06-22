.. _network_builder_tutorials:

Network Builder tutorials 
==============

Using the Network Builder
-----------

This exercise shows how to make model networks of artificial cells, such as pulse generators (network stimulator or "NetStim" cells) and integrate & fire cells, and hybrid nets that contain both artificial cells and cells that have biophysical properties (what some might call "realistic" model cells).

One feature of the Network Builder deserves special mention : it can write a hoc file that specifies the network you built with it. This hoc file shows how to use the various statements that are necessary to construct networks algorithmically. By studying such examples, you can learn how to write your own code to implement much larger nets. Another possible use of the Network Builder is to make microcircuits that can be turned into templates for a more modular approach to construction of large scale models.

:ref:`Introduction to network construction <introduction_to_network_construction>`

Before you start either tutorial, you absolutely should read this.

:ref:`Tutorial 1 : making a network that contains only artificial neurons <tutorial_artificial_neurons>`

This introduces material that you will need to know before moving on to the second tutorial.

:ref:`Tutorial 2 : making a hybrid net that contains both artificial and "real" neurons <tutorial_2_hybrid_nets>`

Before starting this one, make sure you know how to use the CellBuilder (if you haven't already done so, you might want to get the `CellBuilder tutorial <https://nrn.readthedocs.io/en/latest/courses/cellbuilder_overview_and_hints.html?>`_ and work through it).

:download:`This link will get you a pkzipped collection of these html files and images. <data/nets.zip>`





.. toctree::
    :hidden:

    introduction_to_network_construction.rst
    tutorial_artificial_neurons.rst
    tutorial_2_hybrid_nets.rst