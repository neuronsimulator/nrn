.. _project:


.. warning::

    The commands and methods here are outdated and have thus never been updated
    to work with Python; consider directly using a modern version control system
    (e.g. ``git`` or ``hg``) instead. You will likely want to store the version
    identifier with each figure or datafile so you can recreate the data.

Project Management
------------------

RCS control of simulation projects in a single directory. The idea is to 
be able to reproduce a simulation given its version number. 
The version number was printed along with all the neuron windows on the screen 
when the simulation was archived. 
See :ref:`ArchiveAndHardcopy` 

To manage parameter sets instead, use:

.. toctree:: :maxdepth: 1

    mechstan.rst
 
Effective use of this management system requires that the user conform to 
a policy in which no simulation variables are changed except by changing them 
in managed files (listed in the nrnversion file). ie. during a session 
only change variables by editing a hoc file and saving it, or only change 
variables in field editors that actually appear on the screen when a simulation 
is archived. Variable values will be lost if you change them 
by direct command to the interpreter (the command does not appear in any file) 
or if you dismiss a panel after changing one of its field editors (when 
the simulation is archived, only values of field editors actually displayed 
on the screen are saved). User judgment is required to determine if these 
missing variable values will later prevent reproduction of the simulation. 
If this is the case, then when the hardcopy is printed, the user should 
manually enter this information when the log message is requested or 
else make hand written notes on the hardcopy itself. 
 
A policy that seems to work pretty well is to always start a simulation via 

.. code-block::
    none

    	special init.hoc -	# if the simulation uses mod file 

where init.hoc xopens all necessary files to initialize the simulation. 

.. code-block::
    none

    init.hoc 
    //------- 
    xopen("$(NEURONHOME)/lib/hoc/noload.hoc")	// standard run tools 
    xopen("morph.hoc")	// topology, geometry, compartmentalization 
    xopen("memb.hoc")	// membrane properties 
    xopen("param.hoc")	// parameters that are occasionally changed 
    xopen("start.ses")	// will automatically change for every new version 
    //------- 

With this style, whenever an old version is checked out and run, the appearance 
of the screen should match the hardcopy and it should be possible to 
reproduce the archived simulation. 
     

.. warning::
    Reliance on a management policy that requires judgment is regrettable. 
    However making the system failsafe while retaining the generality of the 
    interpreter is probably impossible. Archive strategies consisting solely 
    of checkpoints, 
    eg. coredumps, use a great deal of disk space and don't capture any 
    semantic information about the logical structure of the variables and 
    how they are related. This strategy is low time and high space. 
    Audit trails, saving all the commands and changes, is high time and low space. 
    Its problem is that it captures all the false starts and mistakes. 
    Recapitulating every excruciating detail is probably too confusing. 
    The current system, by developing very low space, very low time logical 
    snapshots, although not failsafe, hopefully will realistically help to 
    solve the very real problems of simulation management. 

Examples
========

The following example illustrates the initialization of an essentially 
empty project. 
 
Here are the files. After initializing the project with these, they 
can be modified to handle arbitrary complexity. These files should be 
in an otherwise empty subdirectory. 
 
init.hoc 

.. code-block::
    none

    xopen("$(NEURONHOME)/lib/hoc/noload.hoc") 
    xopen("morph.hoc") 
    xopen("memb.hoc") 
    xopen("param.hoc") 
    xopen("start.ses") 

morph.hoc 

.. code-block::
    none

    create soma 
    access soma 
    L=5 
    diam=100/(PI*L) 

memb.hoc 

.. code-block::
    none

    insert hh 

param.hoc 

.. code-block::
    none

    gnabar_hh = .120 

start.ses 

.. code-block::
    none

    nrnmainmenu() 

 
The project is initialized with :ref:`prjnrninit` . 
This will create an RCS directory and checkout 
a nrnversion file with the contents: 

.. code-block::
    none

    $Revision: 1.3 $ 
    1.1 init.hoc 
    1.1 memb.hoc 
    1.1 morph.hoc 
    1.1 param.hoc 
    1.1 start.ses 

Note that nrnversion is essentially just a manifest of the files in 
the project. To add a new file to the project one can explicitly check 
the file into RCS with the ci command and insert the appropriate line 
in the nrnversion file. 
 
At this point one can run 

.. code-block::
    none

    nrniv init.hoc - 

and see a neuron main menu. Use the menu to generate a graph 
of an action potential. Since there was an RCS directory with a 
nrnversion,v archive when nrnmainmenu() was executed, 
the Miscellaneous menu has an ArchiveAndHardcopy item. 
Pressing this button will archive the current version with the session 
(it is saved in start.ses), request a description of this version and 
print the version number, description, and session windows on the 
printer specified by the ``$PRINT_CMD`` environment variable. 
 
It is recommended that you play with this simple project for a while 
to familiarize yourself with the style before employing it in a serious 
project. Make several different hardcopies. Use :ref:`prjnrnco` to check out 
earlier versions and run them, modify parameters, and make several more 
hardcopies. Note the way branch version numbers are generated and incremented. 

Utilities
=========     

.. _prjnrninit:

prjnrninit
~~~~~~~~~~


Syntax:
    ``$NEURONHOME/bin/prjnrninit``


Description:
    In the current working directory creates an RCS directory and 
    checks in (and out again with locking) 
    all the hoc, ses, mod files as version 1.1. A nrnversion file 
    is created (and archived) which contains a manifest of the files 
    necessary to recreate a simulation. The Revision level of 
    the nrnversion defines the version of the simulation. 


prjnrncmp
~~~~~~~~~


Syntax:
    ``$NEURONHOME/bin/prjnrncmp``


Description:
    If working files consistent with manifest in nrnversion return with 
    exit status 0. Otherwise return with exit status 1 and print the 
    names of the differing files on the standard output. 


.. _prjnrnco:

prjnrnco
~~~~~~~~


Syntax:
    ``$NEURONHOME/bin/prjnrnco version``


Description:
    Checkout the version from the RCS archive. 
     
    Prior to checkout if the working files differ from the archive, the user 
    is asked whether or not to checkout anyway. If "Checkout anyway" is chosen 
    the changes to the previous working files will be lost. 


prjnrnci
~~~~~~~~


Syntax:
    ``$NEURONHOME/bin/prjnrnci``


Description:
    Checkin a new version to the RCS archive. 
     
    The microemacs editor is run so the user can edit a description of the 
    new version. On exit from the editor the user will be asked whether or 
    not to continue checking in the version. 
     
    A new version is checked in even if the working files are unchanged. 
     
    On exit from this command, the working files are locked versions 
    of the newest version. The nrnversion file contains the version number 
    of the simulation (itself) as well as the version numbers of all the 
    working files it needs to reconstruct the simulation. 

.. _prjnrnpr:

prjnrnpr
~~~~~~~~


Syntax:
    ``cat postscriptfile | $NEURONHOME/bin/prjnrnpr``


Description:
    Checks in the working files and 
    sends the postscript file to the command specified in the 
    ``$PRINT_CMD`` (e.g. :command:`lp`) environment variable. 
     
    If the working files are not different from their archived versions 
    the user is asked whether to continue or verify that the simulation 
    can be reproduced. If the latter, a new the simulation is loaded in 
    an xterm window. The user should then try to reproduce the simulation 
    he/she is attempting to checkin. When the xterm goes away the user 
    will be asked whether or not to continue to checkin. If you can't reproduce 
    the simulation or had to change working files to reproduce it, choose "Abort" 
     
    If the working files did differ then :func:`prjnrnci` is run in an :program:`xterm`. 
     
    The last question for the user to answer is whether to leave the working 
    files at the new or old version. The answer depends on whether you envision 
    this simulation as a side branch off the primary version or as 
    additive. 
     
    The log message entered during checkin is added to the postcript stream 
    and sent to ``$PRINT_CMD``.
     
    This command is called by the :ref:`ArchiveAndHardcopy` menu item in the 
    :ref:`NEURONMainMenu` which first saves the session in :file:`start.hoc` and 
    sends the entire session as standard input to this command. 


ivdialog
~~~~~~~~


Syntax:
    ``$NEURONHOME/bin/prjnrnpr "banner" "accept" "cancel"``


Description:
    Pops up a boolean dialog. 
     
    "1" is printed on the standard output if 
    the "accept" button is pressed. 
     
    "0" is printed on the standard output 
    if the "cancel" button is pressed. 


