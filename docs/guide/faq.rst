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


The classical approach to using NEURON is to specify all three components by writing a program in ``hoc``, NEURON's programming language. You can do this with any editor you prefer, as long as it can save your code to an ASCII text file. Make sure your ``hoc`` files end with the extension ``.hoc``

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

Also, "don't throw all your code into one giant ``hoc`` (or ``ses``) file." Regardless of whether you use ``hoc``, the GUI, or both, it will be much easier to create and revise programs if you keep model specification (the "experimental preparation") separate from instrumentation and control (the "user interface"). You might even put them in separate files, e.g. :file:`model.hoc` might contain the code that specifies the anatomy and biophysics of your model cell or network, and :file:`rig.ses` might specify a RunControl panel and other graphical tools that you use to run simulations, apply stimuli, and display results. Then you create a third file, called :file:`init.hoc`, which contains the following statements :

.. code::
    c++

    load_file("nrngui.hoc") // get NEURON's gui library
    load_file("model.hoc") // the model specification
    load_file("rig.ses") // the instrumentation, control, and user interface

When NEURON executes :file:`init.hoc`, up comes your model and user interface.

This greatly simplifies program development, testing and maintentance. For example, complex models and experimental rigs can be constructed in an incremental manner, so that :file:`init.hoc` grows to contain many :func:`load_file` statements.

- Mine other code (e.g. the Programmers' Reference) for reusable or customizable working examples. "Good programmers imitate great code, great programmers steal great code." But test all code.

Why can't NEURON read the text file (or ``hoc`` file) that I created?
---------------------------------------------------------------------

The Mac, MSWin, and UNIX/Linux versions of NEURON can read ASCII text files created under any of these operating systems. ASCII, which is sometimes called "plain text" or "ANSI text", encodes each character with only 7 bits of each byte. Some text editors offer alternative formats for saving text files, and if you choose one of these you may find that NEURON will not read the file. For example, Notepad under Win2k allows files to be saved as "Unicode text", which will gag NEURON.

How do I print a hard copy of a NEURON window?
----------------------------------------------

Use the Print & File Window Manager (PFWM). View :ref:`Print & File Manager <print_file_manager>` to learn how.

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

:download:`Get a copy of "NEURON: a Tool for Neuroscientists" <data/neurontoolforneuroscientists.pdf>`, which explains how it works.

Read :ref:`how to use the d_lambda rule with your own models <using_the_d_lambda_rule>`.

How do I change the background color used in NEURON's shape plots and other graphs?
-----------------

Edit the :ref:`nrn.defaults <nrn_defaults>` file

How do I change the color scale used in shape plots?
--------------------------

:ref:`Create a file that specifies the desired RGB values. <nrn_defaults>`

I see an error message that says ... procedure too big in ./foo.hoc ...
----------------

There is an upper limit on the size of a procedure that the hoc parser can handle. The workaround is simple. 

Instead of having a single giant procedure, break it into several smaller procedures, and then call these procedures one after another. For example, suppose your procedure is 

.. code::
    c++

    proc buildcell() {
   ... lots of hoc statements ...
   }

just chop it into smaller chunks like this

.. code::
    c++

    proc buildcell_1() {
   ... some hoc statements ...
   }
   proc buildcell_2() {
   ... some more hoc statements ...
   }
   ... etc ...

then execute them with

.. code::
    c++

    buildcell_1()
   buildcell_2()
   ...

How big can a procedure be? I've never tried to find out. Try cutting your big procedure in half and see if that works. If it doesn't, cut the pieces in half and try again. Eventually you'll find a size that works.

Where can I find examples of mod files?
-----------------

See the NMODL tops on :ref:`the "getting started" page. <how_to_get_started_with_neuron>` 


.. _FAQ_how_do_I_compile_mod_files:
How do I compile mod files?
----------------

If the folder with your NEURON scripts (HOC or Python) is the same as the folder with the mod files, open it in a terminal and type ``nrnivmodl``. You can also specify paths; e.g. if all the mod files are in a subfolder called ``mod`` but you're running NEURON from the current folder, type ``nrnivmodl mod``.

Windows and Mac also provide graphical tools that can be used to compile mod files.

I can't get mod files to compile.
-------------------

Go to `The NEURON Forum <https://www.neuron.yale.edu/phpBB/index.php>`_ and check out the "NEURON installation and configuration" discussions for your particular operating system (MacOS, MSWin, UNIX/Linux). For MacOS and UNIX/Linux this problem often means that the software development environment (compilers and associated libraries) is missing or incomplete.

I installed a new version of NEURON, and now I see error messages like this: 'mecanisms fooba needs to be re-translated. its version 5.2 "c" code is incompatible with this neuron version'.
------------------

Compiling NMODL files produces several "intermediate files" whose names end in .o and .c . This error message means that you have some old intermediate files that were produced under the earlier version of NEURON. So just delete all the .o and .c files, then run nrnivmodl (or mknrndll), and the problem should disappear.

Is there a list of functions that are built into NMODL?
--------------------

:ref:`Look here. <nmodls_built_in_functions>`

Is there a list of functions that are built into hoc?
---------------

You'll find them in the `Programmer's Reference <https://nrn.readthedocs.io/en/latest/python/index.html>`_. Also see chapter 11. :ref:`Interpreter - General in the old "Reference Manual." <hoc_chapter_11_old_reference>`

What units does NEURON use for current, concentration, etc.?
--------------

If you're using the GUI, you've probably noticed that buttons next to numeric fields generally indicate the units, such as (mV), (nA), (ms) for millivolt, nanoamp, or millisecond.

:ref:`Here's a chart of the units that NEURON uses by default. <units_used_in_neuron>`

If you're writing your own mod files, you can specify what units will be used. For example, you may prefer to work with micromolar or nanomolar concentrations when dealing with intracellular free calcium or other second messengers. You can also define new units. :ref:`See this tutorial <units_tutorial>` to get a better understanding of units in NMODL.

For the terminally curious, here is a copy of the :download:`units.dat <data/units.dat.txt>` file that accompanies one of the popular Linux distributions. Presumably mod file variables should be able to use any of its entries.


When I type a new value into a numeric field, it doesn't seem to have any effect.
--------------------------------

You seem to be using a very old version of NEURON. If you can't update to the most recent version, try this:

After entering a new value, be sure to click on the button next to the numeric field (or press the Return key) so that the bright yellow warning indicator on the button is turned off. While the yellow indicator is showing, the field editor is still in entry mode and its contents have not yet been assigned to the proper variable in memory.

What is the difference between SEClamp and VClamp, and which should I use?
----------------------------

:class:`SEClamp` is just an ideal voltage source in series with a resistance (Single Electrode Clamp), while :class:`VClamp` is a model of a two electrode voltage clamp with this equivalent circuit:

.. code::

                   tau2
                   gain
                  +-|\____rstim____>to cell
  -amp --'\/`-------|/
                  |
                  |----||---
                  |___    __|-----/|___from cell
                      `'`'        \|
                      tau1 


If the purpose of your model is to study the properties of a cell, use :class:`SEClamp`. If the purpose is to study how instrumentation artefacts affect voltage clamp data, use :class:`VClamp`. 

SEClamp and IClamp just deliver rectangular step waveforms. How can I make them produce an arbitrary waveform, e.g. something that I calculated or recorded from a real cell?
-----------------------

The Vector class's play method can be used to drive any variable with a sequence of values stored in a Vector. For example, you can play a Vector into an IClamp's amp, an SEClamp's amp1, an SEClamp's series resistance rs (e.g. if you have an experimentally measured synaptic conductance time course). To learn how to do this, get :download:`data/vecplay.hoc`, which contains an exercise from one of our 5-day hands-on NEURON courses. Unzip it in an empty directory. This creates a subdirectory called :file:`vectorplay`, where you will find a file called :file:`arbforc.html`

Open this file with your browser and start the exercise.

I just want a current clamp that will deliver a sequence of current pulses at regular intervals. Vector play seems like overkill for this.
---------------

Right you are. Pick up `pulsedistrib.zip <http://www.neuron.yale.edu/neuron/static/docs/repstim/pulsedistrib.zip>`_, and unzip it into an empty directory. This creates a subdirectory called pulsedistrib, which contains :download:`data/Ipulse1.mod`, :download:`data/Ipulse2.mod`, :download:`data/readme.txt`, and :download:`data/test_1_and_2.hoc`. Read :download:`data/readme.txt`, compile the mod files, and then use NEURON to load :download:`data/test_1_and_2.hoc`, which is a simple demo of these two current pulse generators.

pulsedistrib also contains :download:`data/ipulse3.mod`, :download:`data/ipulse3rig.ses`, and :download:`data/test_3.hoc`, which address the next question in this list.

I want a current clamp that will generate a pulse when I send it an event, or that I can use to produce pulses at precalculated times.
-----------------------

Then get `pulsedistrib.zip <http://www.neuron.yale.edu/neuron/static/docs/repstim/pulsedistrib.zip>`_, and unzip it. Inside the pulsedistrib subdirectory you'll find :download:`data/ipulse3.mod`, :download:`data/ipulse3rig.ses`, and :download:`data/test_3.hoc` (and some other files that pertain to the previous question). :download:`data/ipulse3.mod` contains the NMODL code for a current clamp that produces a current pulse when it receives an input event. :download:`data/test_3.hoc` is a simple demo of the Ipulse3 mechanism, and :download:`data/ipulse3rig.ses` is used by :download:`data/test_3.hoc` to create the GUI for a demo of Ipulse3. It uses a :hoc:class:`NetStim` to generate the events that drive the Ipulse3. If you want to drive an Ipulse3 with recorded or precomputed event times, use the VecStim class as described under the topic `Driving a synapse with recorded or precomputed spike events <https://www.neuron.yale.edu/phpBB/viewtopic.php?f=28&t=2117>`_ in the "Hot tips" area of the `NEURON Forum <https://www.neuron.yale.edu/phpBB/>`_.

I have a set of recorded or calculated spike times. How can I use these to drive a postsynaptic mechanism?
------------------------

Assuming that your synaptic mechanism has a ``NET_RECEIVE`` block, so that it is driven by events delivered by a :hoc:class:`NetCon`, I can think of two ways this might be done. Which one to use depends on how many calculated spike times you are dealing with.

If you only have a "few" spikes (up to a few dozen), you could just dump them into the spike queue at the onset of the simulation. Here's how: 

1.
    Create a Vector and load it with the times at which you want to activate the synaptic mechanism.

2.
    Then use an :ref:`finitialize_handler` that stuffs the spike times into the NetCon's event queue by calling the `NetCon class's event() method <https://nrn.readthedocs.io/en/latest/hoc/modelspec/programmatic/network/netcon.html>`_ during initialization.

    For example, if the Vector that holds the event times is syntimes, and the NetCon that drives the synaptic point process is nc, this would work:

For example, if the Vector that holds the event times is syntimes, and the NetCon that drives the synaptic point process is nc, this would work:

.. code::
    c++

    objref fih
    fih = new FInitializeHandler("loadqueue()")
    proc loadqueue() { local ii
        for ii=0,syntimes.size()-1 nc.event(syntimes.x[ii])
    }

Don't forget that these are treated as *delivery* times, i.e. the NetCon's delay will have no effect on the times of synaptic activation. If additional conduction latency is needed, you will have to incorporate it by adding the extra time to the elements of syntimes before the FInitializeHandler is called. 

If you have a lot of spikes then it's best to use an NMODL-defined artificial spiking cell that generates spike events at times that are stored in a Vector (which you fill with data before the simulation). For more information see `Driving a synapse with recorded or precomputed spike events <https://www.neuron.yale.edu/phpBB/viewtopic.php?f=28&t=2117>`_ in the "Hot tips" area of the `NEURON Forum <https://www.neuron.yale.edu/phpBB/>`_.

How can I read data from a binary PClamp file?
--------------------

clampex.zip contains a mod file that defines an object class (ClampExData) whose methods can read PClamp binary files — or at least it could several years ago - plus a sample data file and a hoc file to illustrate usage. If ClampExData doesn't work with the most recent PClamp file formats, at least ``clampex.mod`` is a starting point that you can modify as needed.

How do I exit NEURON? I'm not using the GUI, and when I enter ^D at the oc> prompt it doesn't do anything?
-----------------------------

You seem to be using an older MSWin or MacOS version of NEURON (why not get the most recent version?). Typing the command

quit()

at the oc> prompt works for all versions, new or old, under all OSes. Don't forget the parentheses, because quit() is a function. Oh, and you need to press the Enter or Return key too.


Is there a list of functions that are built into NEURON?
--------------------------------------------------------

For available functions when scripting or interactively using NEURON, see the `Python programmer's reference <../python/index.html>`_ or the `HOC programmer's reference <../hoc/index.html>`_ as appropriate. (Standard Python functions and libraries may be used as well.)

For functions available when defining ion channel mechanisms etc with NMODL, see :ref:`here <nmodls_built_in_functions>`.

.. toctree::
    :hidden:

    using_plotwhat_to_specify_a_variable.rst
    working_with_postscript_and_idraw.rst
    using_session_files_for_saving.rst
    using_the_d_lambda_rule.rst
    nrn_defaults.rst
    how_to_get_started_with_neuron.rst
    nmodls_built_in_functions.rst
    units_tutorial.rst
    finitialize_handler.rst
    using_neuron_on_the_mac.rst


