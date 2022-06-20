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


:ref:`macOS users will find additional information here. <using_neuron_on_the_mac>`

To exit NEURON : type ``quit()`` or ``^D`` ("control D") at the ``oc>`` or ``>>>`` prompt, or use :menuselection:`File --> Quit` in the NEURON Main Menu toolbar.


Installation went smoothly, but every time I bring NEURON up, the interpreter prints this strange message: "jvmdll" not defined in nrn.def JNI_CreateJavaVM returned -1
++++++++++++++++++++++++++++++++++++

You must be running an old version of NEURON. Warnings about Java, such as "Can't create Java VM" or "Info: optional feature is not present" mean that NEURON can't find a Java run-time environment. This is of interest only to individuals who are using Java to develop new tools. NEURON's computational engine, standard GUI library, etc. don't use Java.


What's the best way to learn how to use NEURON?
-----------------------------------------------
First be sure to join `The NEURON Forum <https://www.neuron.yale.edu/phpBB/index.php>`_. 
If you have time, watch our course :ref:`videos <training_videos>`; the material in each course varies, but they generally include a mix of theory and applications.
Then :ref:`read these suggestions <how_to_get_started>`.

How do I create a NEURON model?
-------------------------------

By specifying representations in the computer of the three basic physical components of an actual experiment.

.. list-table:: 
   :header-rows: 1

   * - **Component**
     - **Wet lab**
     - **Compuational Modeling**

   * - Experimental preparation - *What is the biology itself?*
     -
       brain slice, tissue culture etc.
     - 
        specification of what anatomical and biophysical properties to represent in model

   * - Instrumentation - *How will you stimulate it and record results?*
     -
       voltage/current clamp, electrodes, stimulator, recorder etc. 
     - 
        computational representations of clamps, electrodes etc. 

   * - Control - *How do you automate the experimental protocol?*
     -
       programmable pulse generators etc.
     - 
        specification of which variables to monitor and record time step, when to stop, integration method, optimization algorithms


The classical approach to using NEURON is to specify all three components by writing a program in ``hoc``, NEURON's programming language. You can do this with any editor you prefer, as long as it can save your code to an ASCII text file. Make sure your ``hoc`` files end with the ``extension .hoc``

A more recent approach is to use the NEURON Main Menu toolbar's dropdown menus, which allow you to quickly create a wide range of models without having to write any code at all. You can save the GUI's windows and panels to session files that you can use later to recreate what you built (see the FAQ "What is a ses (session) file?").

The most flexible and productive way to work with NEURON is to combine hoc and the GUI in ways that exploit their respective strengths. Don't be afraid of the GUI--noone will accuse you of being a "girly man" if you take advantange of its powerful tools for model specification, instrumentation, and control. In fact, many of the GUI's most useful tools would be extremely difficult and time consuming to try to duplicate by writing your own hoc code.

Be sure to read the FAQ "Help! I'm having a hard time implementing models!"

Help! I'm having a hard time implementing models!
-------------------------------------------------
Here are some general tips about program development.

- Before you write any code, write down an explicit outline of how it should work. Use a "top-down" approach to avoid being overwhelmed at the start by implementational details.

- Successful programming demands an incremental cycle of revision and testing. Start small with something simple that works. Add things one at a time, testing at every step to make sure the new stuff works and didn't break the old stuff.

- Comment your code.

- Use a "modular" programming style. At the most concrete level, this means using lots of short, simple procs and funcs.

Also, "don't throw all your code into one giant ``hoc`` (or ``ses``) file." Regardless of whether you use ``hoc``, the GUI, or both, it will be much easier to create and revise programs if you keep model specification (the "experimental preparation") separate from instrumentation and control (the "user interface"). You might even put them in separate files, e.g. "model.hoc" might contain the code that specifies the anatomy and biophysics of your model cell or network, and "rig.ses" might specify a RunControl panel and other graphical tools that you use to run simulations, apply stimuli, and display results. Then you create a third file, called "init.hoc", which contains the following statements :

.. code::
    Python

    load_file("nrngui.hoc") // get NEURON's gui library
   load_file("model.hoc") // the model specification
   load_file("rig.ses") // the instrumentation, control, and user interface

When NEURON executes ``init.hoc``, up comes your model and user interface.

This greatly simplifies program development, testing and maintentance. For example, complex models and experimental rigs can be constructed in an incremental manner, so that ``init.hoc`` grows to contain many ``load_file`` statements.

- Mine other code (e.g. the Programmers' Reference) for reusable or customizable working examples. "Good programmers imitate great code, great programmers steal great code." But test all code.

Why can't NEURON read the text file (or ``hoc`` file) that I created?
---------------------------------------------------------------------

The Mac, MSWin, and UNIX/Linux versions of NEURON can read ASCII text files created under any of these operating systems. ASCII, which is sometimes called "plain text" or "ANSI text", encodes each character with only 7 bits of each byte. Some text editors offer alternative formats for saving text files, and if you choose one of these you may find that NEURON will not read the file. For example, Notepad under Win2k allows files to be saved as "Unicode text", which will gag NEURON.

How do I print a hard copy of a NEURON window?
----------------------------------------------

Use the Print & File Window Manager (PFWM). Download :download:`printing.pdf <data/printing.pdf>` to learn how.

How do I plot something other than membrane potential?
------------------------------------------------------

Use the :ref:`Plot what? <using_plotwhat_to_specify_a_variable_to_be_plotted>` tool 

How do I save and edit figures?
-------------------------------

The quick and dirty way is to capture screen images as bitmaps. The results are suitable for posting on WWW sites but resolution is generally too low for publication or grant proposals, and editing is a pain. For highest quality, PostScript is best. Use the Print & File Window Manager (PFWM) to save the graphs you want to an Idraw file. This is an encapsulated PostScript format that can be edited by idraw, which comes with the UNIX/Linux version of NEURON. It can also be imported by many draw programs, e.g. CorelDraw. 
 
To learn more, :ref:`see this tutorial <working_with_postscript_and_idraw_figures>` from the NEURON Summer Course.

I've used the NEURON Main Menu to construct and manage models. How can I save what I have done?
-------------------------------------

:ref:`Here's how to save the GUI tools <using_session_files_for_saving_and_retrieving_windows>` you spawned to a session file.

What is a ses (session) file? Can I edit it?
------------

A session file is a plain text file that contains hoc statements that will recreate the windows that were saved to it. It is often quite informative to examine the contents of a ses file, and sometimes it is very useful to change the file's contents with a text editor. :ref:`Read this <>` for more information.

How do I use NEURON's tools for electrotonic analysis?
----------------

See this :ref:`sample lesson <electrotonic_analysis>` from the NEURON Summer Course

Why should I use an odd value for nseg?
---------------------------------------

So there will always be a node at 0.5 (the middle of a section).

Read about this in `NEURON: a Tool for Neuroscientists <https://doi.org/10.1177/107385840100700207>`_ by Hines & Carnevale.

What's a good stretegy for specifying nseg?
-----------------

Probably the easiest and most efficient way is to use what we call the d_lambda rule, which means "set nseg to a value that is a small fraction of the AC length constant at a high frequency."

:download:`Get a copy of "NEURON: a Tool for Neuroscientists" <neurontoolforneuroscientists.pdf>`, which explains how it works.

Read :ref:`how to use the d_lambda rule with your own models<using_the_d_lambda_rule>`.


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
    using_plotwhat_to_specify_a_variable.rst
    working_with_postscript_and_idraw.rst
    using_session_files_for_saving.rst
    using_the_d_lambda_rule.rst

