.. _how_do_i_create_a_neuron_model:

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