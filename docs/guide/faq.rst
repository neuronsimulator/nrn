Frequently asked questions (FAQ)
================================




Now that I've installed NEURON, how do I run it?
------------------------------------------------

To start NEURON and bring up the NEURON Main Menu toolbar (which you can use to build new models and load existing ones) :

* UNIX/Linux : type ``nrngui`` or ``nrngui -python`` at the system prompt. (The ``-python`` flag will give you a Python prompt instead of a HOC prompt.)
* OS X : double click on the nrngui icon in the folder where you installed NEURON.
* MSWin : double click on the nrngui icon in the NEURON Program Group (or in the desktop NEURON folder).

To start NEURON from python and bring up the NEURON Main Menu, launch "python" then type

.. code:: python

    from neuron import h, gui

To make NEURON read a file called ``foo.hoc`` when it starts :

* UNIX/Linux : type nrngui foo.hoc at the system prompt. This also works for ses files.
* OS X : drag and drop foo.hoc onto the nrngui icon. This also works for ses files.
* MSWin : use Windows Explorer (not Internet Explorer) to navigate to the directory where foo.hoc is located, and then double click on foo.hoc . This does not work for ses files.


.. todo: macOS users will find additional information here.

To exit NEURON : type ``quit()`` or ``^D`` ("control D") at the ``oc>`` or ``>>>`` prompt, or use :menuselection:`File --> Quit` in the NEURON Main Menu toolbar.

..

    Installation went smoothly, but every time I bring NEURON up, the interpreter prints this strange message: "jvmdll" not defined in nrn.def JNI_CreateJavaVM returned -1
    -----------------------------------------------------------------------------------------------------------------------------------------------------------------------

    You must be running an old version of NEURON. Warnings about Java, such as "Can't create Java VM" or "Info: optional feature is not present" mean that NEURON can't find a Java run-time environment. This is of interest only to individuals who are using Java to develop new tools. NEURON's computational engine, standard GUI library, etc. don't use Java.

What's the best way to learn how to use NEURON?
-----------------------------------------------
First be sure to join `The NEURON Forum <https://www.neuron.yale.edu/phpBB/index.php>`_. 
If you have time, watch our course :ref:`videos <training_videos>`; the material in each course varies, but they generally include a mix of theory and applications.
Then :ref:`read these suggestions <how_to_get_started>`.



What is a ses (session) file? Can I edit it?
--------------------------------------------

A session file is a plain text file that contains hoc statements that will recreate the windows that were saved to it. It is often quite informative to examine the contents of a ses file, and sometimes it is very useful to change the file's contents with a text editor. :ref:`Read this <saveses>` for more information.

Why should I use an odd value for nseg?
---------------------------------------

So there will always be a node at 0.5 (the middle of a section).

Read about this in `NEURON: a Tool for Neuroscientists <https://doi.org/10.1177/107385840100700207>`_ by Hines & Carnevale.

What units does NEURON use for current, concentration, etc?
-----------------------------------------------------------

If you're using the GUI, you've probably noticed that buttons next to numeric fields generally indicate the units, such as (mV), (nA), (ms) for millivolt, nanoamp, or millisecond.

:ref:`Here's a chart of the units that NEURON uses by default <units>` along with information on validating units and defining new units in NMODL.

Is there a list of functions that are built into NEURON?
--------------------------------------------------------

For available functions when scripting or interactively using NEURON, see the `Python programmer's reference <../python/index.html>`_ or the `HOC programmer's reference <../hoc/index.html>`_ as appropriate. (Standard Python functions and libraries may be used as well.)

For functions available when defining ion channel mechanisms etc with NMODL, see :ref:`here <nmodlfunc>`.

.. toctree::
    :hidden:

    nmodlfunc

