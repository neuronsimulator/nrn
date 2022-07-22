.. _custom_initialization:

Custom Initialization
=====================

Physical System
---------------

.. image:: img/pyramid.gif

Conceptual Model
----------------

A ball-and-stick approximation to the cell.

.. image:: img/ballstk.gif

The aim of this exercise is to learn how to perform one of the most common types of custom initializaton: initializing to steady state.

We start by making a ball and stick model in which the natural resting potential of the somatic sphere and dendritic cylinder are different. No matter what v_init you choose, default initializaton to a uniform membrane potential results in simulations that show an initial drift of v as the cell settles toward a stable state.

After demonstrating this initial drift, we will implement and test a method for initializing to steady state that leaves membrane potential nonuniform in space but constant in time.

Getting started
~~~~~~~~~~~~~~~

In this exercise you will be creating several files, so you need to be in a working directory for which you have file "write" permission. Start NEURON with exercises/custom_initialization as the working directory.

Computational implementation of the conceptual model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Use the CellBuilder to make a simple ball and stick model that has these properties:

.. list-table:: 
   :header-rows: 1

   * - Soma
     - Anatomy
     - Compartmentalization
     - Biophysics
   * - soma
     -
       length 20 µm
       
       diameter 20 µm
     - ``d_lambda = 0.1``
     - 
       ``Ra = 160`` ohm cm, 
       

       ``Cm = 1`` µf / cm\ :sup:`2`
       
       Hodgkin-Huxley channels
   * - dend
     -
       length 1000 µm
       
       diameter 5 µm
     - ``d_lambda = 0.1``
     - 
       ``Ra = 160`` ohm cm, 
       
       ``Cm = 1`` µf / cm\ :sup:`2`
       
       passive with Rm = 10,000 ohm cm\ :sup:`2`

Turn Continuous Create ON and save your CellBuilder to a session file.

 

Using the computational model
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Bring up a RunControl and a voltage axis graph.

Set Tstop to 40 ms and run a simulation.

Use View = plot to see the time course of somatic membrane potential more clearly.

Add ``dend.v(1)`` to the graph (use Plot what?), then run another simulation.
Use Move Text and View = plot as needed to get a better picture.

Add a space plot and use its Set View to change the y axis range to ``-70 -65``.
Run another simulation and watch how drastically v changes from the initial condition.

Save everything to a session file called :file:`all.ses` (use :menuselection:`File --> save session`) and exit NEURON.

Exercise: initializing to steady state
--------------------------------------

In the same directory, make an :file:`init_ss.py` file with the contents:

.. code:: 
    python

    from neuron import h, gui

    def ss_init(t0=-1e3, dur=1e2, dt=0.025):
        """Initialize to steady state.  
        Executes as part of h.finitialize()
        Appropriate parameters depend on your model
        t0 -- how far to jump back (should be < 0)
        dur -- time allowed to reach steady state
        dt -- initialization time step
        """
        h.t = t0
        # save CVode state to restore; initialization with fixed dt
        old_cvode_state = h.cvode.active()
        h.cvode.active(False)
        h.dt = dt
        while (h.t < t0 + dur): 
            h.fadvance()
        
        # restore cvode active/inactive state if necessary
        h.cvode.active(old_cvode_state)
        h.t = 0
        if h.cvode.active():
            h.cvode.re_init()
        else:
            h.fcurrent()
        h.frecord_init()

    fih = h.FInitializeHandler(ss_init)

    # model specification
    h.load_file('all.ses') # ball and stick model with exptl rig

Now execute :file:`init_ss.py`.

Click on Init & Run and see what happens.

"Special credit" Exercise
-------------------------

Another common initialization is for the initialized model to satisfy a particular criterion. Create an initialization that ensures the resting potential throughout the cell equals ``v_init``.

 

