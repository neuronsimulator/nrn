
.. _hoc_runctrl:


RunControl
----------

A minimal control system for managing a single "Oscilloscope sweep" level 
simulation run. 
     

t
~

Neuron time (ms). The field editor is updated regularly to display the 
value of the global variable :hoc:data:`t`.

dt
~~

Value of the fundamental integration time step, :hoc:data:`dt`,
used by :hoc:func:`fadvance`.
When a value is entered into the field editor it is rounded down 
so that an integral multiple of fadvance's make up a SingleStep 


.. _hoc_runcontrol_initrun:

InitRun
~~~~~~~

Initialize states, set t=0, and run the simulation until t == Tstop 
Plotting to graphs constructed from the :ref:`hoc_NEURONMainMenu` occurs at
a rate given by the variable set by the :ref:`hoc_Plotsms` value editor.
It is often convenient to substitute problem specific procedures 
for the default procedures :hoc:func:`init` and :hoc:func:`advance`.
The run call chain is 

.. code-block::
    none

    		run continuerun step advance fadvance 

The default advance is merely 

.. code-block::
    none

    		proc advance() { 
    			fadvance() 
    		} 

and is a good candidate for substitution by a problem specific 
user routine.

.. warning:: 

    multiple presses of the this button without waiting 
    for the previous simulation to finish (or pressing Stop) will 
    execute the run() procedure recursively (probably not what is 
    desired) Press the Stop button to unwrap these recursions. 


.. _hoc_runcontrol_init:

Init
~~~~

The default initialize procedure initializes states using 
:hoc:func:`finitialize` (v_init) where v_init is displayed in the value editor.
The init call chain is 

.. code-block::
    none

    		stdinit init (finitialize fcurrent) 

When more complicated initialization is required, use 
:hoc:class:`FInitializeHandler` statements or  substitute a
new procedure for the default init procedure: 

.. code-block::
    none

    	proc init() { 
    		finitialize(v_init) 
    		// insert new initialization code here to change states 
    		// If states have been changed then complete 
    		// initialization with 
    	    /*	 
    		if (cvode.active()) { 
    			cvode.re_init() 
    		}else{ 
    			fcurrent() 
    		} 
    		frecord_init() 
    	    */ 
    	} 


.. seealso::
    :hoc:func:`finitialize`, :hoc:meth:`CVode.re_init`, :hoc:func:`fcurrent`, :hoc:func:`frecord_init`, :hoc:class:`FInitializeHandler`


.. _hoc_runctrl_stop:

Stop
~~~~

Stops the simulation at the end of a step. 

Continuetil
~~~~~~~~~~~

Continues integrating until t >= value displayed in value editor. 
Plots occur each step. 

Continuefor
~~~~~~~~~~~

Continues integrating for amount of time displayed in value editor. 
Plots occur each step. 

SingleStep
~~~~~~~~~~

Integrates one step and plots. 
A step is 1/(Plots/ms) milliseconds and consists of 1/dt/(Plots/ms) 
calls to fadvance() 

Tstop
~~~~~

Stop time for InitRun 


.. _hoc_plotsms:

Plotsms
~~~~~~~

Number of integration steps per millisecond at which plots occur. 
Notice that reducing dt does not by itself increase the number 
of points plotted. If the the step is not an integral multiple of 
dt then dt is rounded down to the nearest integral multiple. 

Quiet
~~~~~

When checked, turns off movies and graph flushing during 
an :ref:`hoc_runcontrol_initrun`. Under some circumstances this can speed
things up very considerably such as when using the :ref:`hoc_RunFitter`
in the presence of a Shape Movie plot under MSWINDOWS. 

RealTime
~~~~~~~~

Running display of computation time. Resolution is 1 second. 
     

