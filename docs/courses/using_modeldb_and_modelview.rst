.. _using_modeldb_and_modelview:

Using ModelDB with NEURON
=========================

`https://modeldb.yale.edu/ <https://modeldb.yale.edu/>`_ is the the home page of ModelDB.

We'll be analyzing a couple of models in order to answer these questions:

1.
    What physical system is being represented, and for what purpose?

2.
    What is the representation of the physical system, and how was it implemented?

3.
    What is the user interface, how was it implemented, and how do you use it?

Example: Moore et al. 1983 `modeldb.yale.edu/9852 <https://senselab.med.yale.edu/ModelDB/showmodel.cshtml?model=9852#tabs-1>`_
------------------------------------------------------------------------------------------------------------------------------

    Moore JW, Stockbridge N, Westerfield M (1983)
    On the site of impulse initiation in a neurone.
    J. *Physiol*. 336:301-11 doi: `10.1113/jphysiol.1983.sp014582 <https://pubmed.ncbi.nlm.nih.gov/6308224/>`_

This one doesn't have any mod files, but there's plenty to keep us busy. Begin by downloading the model.

1. What physical system is being represented, and for what purpose?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

ModelDB provides a link to `PubMed <https://pubmed.ncbi.nlm.nih.gov/6308224/>`_ for the abstract. At some point we should also read the paper.

Examine the README file. Does it provide any more clues?

2. What is the representation of the physical system, and how was it implemented?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Load mosinit.hoc 

.. code::
    python

    from neuron import h, gui
    h.load_file('mosinit.hoc')

The PointProcessManager shows a shape plot of the cell.

Using Python to Examine What's in the Model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This model was written in HOC, but we can still use Python to explore it. To see what sections exist and how they are connected, type

.. code::
    python

    h.topology()

and

.. code:: 
    python

    from pprint import pprint  # optional; could use print
    for sec in h.allsec():
        pprint(sec.psection())

at the >>> prompt.

At this point, you might think you'd have to start reading source code in order to get any more information about what's in the model. But the ModelView tool can save you a lot of time and effort, and it has the advantage of telling you exactly what the model specification is. This is a big advantage, since some programs use complex specification and initialization routines that change model structure and parameters "on the fly."

Using Model View to Discover What's in a Model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Select the Model Views tab from the model entry. Under "Load Model Views" you will see that there are a number of figures that this model code can generate. Select "Figure 1A". Two windows will appear.

The one on the left shows a browsable outline of the model's properties.

Plus signs mark the items that are "expandable."

Expand the outline by clicking on "1 cell with morphology". Then peer inside the model by clicking on each of its expandable subitems.

Notice what happens when you get down to

1 cell with :menuselection:`morphology --> root soma --> 6 inserted mechanisms --> hh --> gnabar`

Mouse over the two graphs to explore the relationships between position and conductances gnabar. Do the same for gkbar and gl.

A shortcut for discovering the distributions of spatially inhomogeneous parameters: :menuselection:`Density Mechanisms --> Heterogeneous parameters`

reveals all that are spatially nonuniform. Expand any one to see the values over the model.

Repeat the same exercise using NEURON's ModelView tool (:menuselection:`Tools --> Miscellaneous --> Model View`). How are the views similar? How are they different?

Analyzing the Underlying Code
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Was this model specified by user-written hoc code, or was a CellBuilder used?

Exit the simulation and search the hoc files for create statements.

In the terminal execute

.. code::
    python

    grep create *hoc

(MSWin users: first open a bash shell, then cd to the exercises/modeldb_and_modelview/moore83 directory)

Alternatively you could try Windows Explorer's semi-useful Search function, or open each hoc file with a text editor and search for create.

If no hoc file contains the create keyword, maybe the CellBuilder was used.

Run :file:`mosinit.hoc` again and look for a CellBuilder.

If you don't see one, maybe a Window Group Manager is hiding it.

Click on :menuselection:`NEURON Main Menu --> Window` and look for one or more window names that are missing a red check mark. If you see one, scroll down to it and release the mouse button.

If a CellBuilder pops up, examine its Topology, Subsets, Geometry, and Biophysics pages.

Do they agree with the output of ``for sec in h.allsec(): pprint(sec.psection(())`` and/or what you discovered with the Model View tool?

"Extra Credit" Question

Now you know what's in the model cell, and how it was implemented. Suppose you wanted to get a copy of it that you could use in a program of your own. Would you do this by saving a CellBuilder to a new session file, or by using a text editor to copy create, connect, insert etc. statements from one of the hoc files?


3. What is the user interface, how was it implemented, and how do you use it?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

What is that panel with all the buttons?

What happens if you click on one of them?

Click on a different one and see what happens to the string at the top of the panel.

Click on some more and see what happens to the blue dot in the PointProcessManager's shape plot.

Is this one of the standard GUI tools you can bring up with the NEURON Main Menu?

How does it work?

    Hints: look for an xpanel statement in one of the hoc files.

    Read about :func:`xpanel`, :func:`xbutton`, and :func:`xvarlabel` in the help files.

Find the procedures that implement the actions that are caused by clicking on a button.

The last statement in each of these procedures launches a simulation.

What does the very first statement do?

What does the second statement do?

    The remaining statements do one or more of the following:

    change model parameters (e.g. spatial distribution of HH in the dendrite)

    change stimulus parameters (e.g. stimulus location and duration)

    change simulation parameters

Why does the space plot automatically save traces every 0.1 ms?

    Hint: analyze the procedure that actually executes a simulation

    Which hoc file contains this procedure?

What procedure actually changes the stimulus location, duration, and amplitude? Read about PointProcessManager in the help files.


Another example: Mainen and Sejnowski 1996 `modeldb.yale.edu/2488 <https://senselab.med.yale.edu/ModelDB/showmodel.cshtml?model=2488#tabs-1>`_
----------------------------------------------------------------------------------------------------------------------------------------------

    Mainen ZF, Sejnowski TJ (1996). Influence of dendritic structure on firing pattern in model neocortical neurons. *Nature* 382:363-6. doi: `doi.org/10.1038/382363a0 <https://www.nature.com/articles/382363a0#citeas>`_

This one has interesting anatomy and several mod files. Begin by downloading the model from `modeldb.yale.edu/2488 <https://senselab.med.yale.edu/ModelDB/showmodel.cshtml?model=2488#tabs-1>`_

The model archive patdemo.zip has already been downloaded and unzipped. Its contents are in :file:`exercises/modeldb_and_modelview/patdemo`


1. What physical system is being represented, and for what purpose?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

This is ModelDB's link to `PubMed <https://pubmed.ncbi.nlm.nih.gov/8684467/>`_ for the abstract. Another paper to read.

Read the README.txt file. Any more clues here?

2. What is the representation of the physical system, and how was it implemented?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Compile the mod files, then load mosinit.hoc as in the previous exercise.

Four different cell morphologies are available. Select one of them, then click on the Init button to make sure that all model specification and initialization code has been executed. Use Model View to browse the model, and examine the heterogeneous parameters.

Now it's time to discover how this model was created. Where are the files that contain the pt3d statements of these cells?

This program grafts a stylized myelinated axon onto 3d specifications of detailed morphometry.

Where is the hoc code that accomplishes this grafting?

If you load mosinit.hoc and then try to import one of the cell morphologies into the CellBuilder, do you also get the axon?

Length and diameter are scaled in order to compensate for the effect of spines on dendritic surface area. Find the procedure that does this.

What is an alternative way to represent the effect of spines?

nseg is adjusted in each section so that no segment is longer than 50 um. What procedure does this?

Five active currents and one pump mechanism are included. Examine these mod files.

Do they appear to be compatible with CVODE?

Check them with ``modlunit``.

Did you find any inconsistencies?

Do any of these seem likely to affect simulation results?

Are there any other warning messages?

Is there anything that would cause numerical errors?

How might you fix the problems that you found?

3. What is the user interface, how was it implemented, and how do you use it?
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

:file:`mosinit.hoc` brings up a minimal GUI for selecting cells and running simulations.

How did they do that?

4. Reuse one of their cells in a model of your own design
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Import its morphology into a CellBuilder, then save the CellBuilder to a session file and exit the simulation.

Restart ``nrngui`` and load the CellBuilder's session file.

Assign a plausible set and spatial distribution of biophysical properties and save to a session file.

Instrument your new model and run a simulation.

Save the model, with instrumentation, to a session file.




