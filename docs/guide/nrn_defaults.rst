.. _nrn_defaults:

The appearance of NEURON's graphical windows is controlled by definitions in the file ``$(NEURONHOME)/lib/nrn.defaults`` (under MSWindows it's ``c:\nrnxx\lib\nrn.def`` where ``xx`` is the version number). This is a plain ASCII file that you can edit with a text editor.

What's $(NEURONHOME)?

Start NEURON, and at the oc> prompt type

.. code::
    Python

    neuronhome()

On my Linux box, this tells me that $(NEURONHOME) is

:file:`/usr/local/nrn/share/nrn`

and sure enough, ``nrn.defaults`` is in

.. code::
    Python

    /usr/local/nrn/share/nrn/lib 

How to change the background color used in shape plots and other graphs
------------------

Change the lines

.. code::

   *Scene_background: #ffffff
   *Scene_foreground: #000000


to whatever you like. For example,

.. code::
    Python

   *Scene_background: #000000
   *Scene_foreground: #ffffff

makes the background black and the axes and black traces white.

How create a custom colormap
------------

The lines

.. code::
    Python

   //the color map for pseudocolor plotting for 3-D cells
   *shape_scale_file: $(NEURONHOME)/lib/shape.cm2

tell you which file contains the colormap that will be used (under MSWin it's ``c:\nrnxx\lib\shape.cm2``). The colormap file is read when NEURON is initially launched. If the file doesn't exist, NEURON uses a default scale.

The colormap file is plain ASCII, with one set of RGB values per line. NEURON comes with a couple of different scales in files shape.cm1 and shape.cm2.

Here's another one you might try:

.. code::

    95      0       95      
    111     0       111
    127     0       143
    143     0       127
    159     0       111
    175     0       95
    191     0       79
    207     0       63
    207     31      47
    223     47      47
    239     63      31
    255     79      15
    255     95      7
    255     111     0
    255     127     0
    255     143     0
    255     159     0
    255     175     0
    255     191     0
    255     207     0
    255     223     0
    255     239     0
    255     247     0
    255     255     0
    255     255     255

and yet another:

.. code::

    111     0       111
    143     0       127
    175     0       95
    207     0       63
    223     47      47
    255     79      15
    255     111     0
    255     143     0
    255     175     0
    255     207     0
    255     239     0
    255     255     0
    255     255     200

but you can make up anything you like.

Before you start cooking up your own schemes, you might want to look at the following:

`"How NOT to Lie with Visualization" by Rogowitz & Treinish <https://aip.scitation.org/doi/pdf/10.1063/1.4822401>`_

`"A Rule-based Tool for Assisting Colormap Selection" by Bergman et al. <https://www.researchgate.net/publication/220943601_A_Rule-Based_Tool_for_Assisting_Colormap_Selection>`_



