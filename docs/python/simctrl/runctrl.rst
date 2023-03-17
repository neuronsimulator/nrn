.. _runctrl:


RunControl
----------

A minimal control system for managing a single "Oscilloscope sweep" level 
simulation run. 
     

t
~

Neuron time (ms). The field editor is updated regularly to display the 
value of the global variable :data:`t`. 

dt
~~

Value of the fundamental integration time step, :data:`dt`, 
used by :func:`fadvance`. 
When a value is entered into the field editor it is rounded down 
so that an integral multiple of fadvance's make up a SingleStep 

.. _runcontrol_initrun:

InitRun
~~~~~~~

Initialize states, set t=0, and run the simulation until t == Tstop 
Plotting to graphs constructed from the :ref:`NEURONMainMenu` occurs at 
a rate given by the variable set by the :ref:`Plotsms` value editor. 
It is often convenient to substitute problem specific procedures 
for the default procedures :func:`init` and :func:`advance`. 
The run call chain is 

.. code-block::
    none

        h.run --> h.continuerun --> h.step --> h.advance --> h.fadvance 

The default advance is merely a HOC function that calls :func:`fadvance`. It may be overriden via, e.g.

.. code-block::
    python

    h('proc advance() {nrnpython("myadvance()")}')
    
    def myadvance():
        print(f'h.t = {h.t}')
        h.fadvance()


.. warning:: 

    Multiple presses of the this button without waiting 
    for the previous simulation to finish (or pressing Stop) will 
    execute the run() procedure recursively (probably not what is 
    desired) Press the Stop button to unwrap these recursions. 

.. _runcontrol_init:

Init
~~~~

The default initialize procedure initializes states using 
:func:`finitialize` (v_init) where v_init is displayed in the value editor. 
The init call chain is 

.. code-block::
    none

        h.stdinit --> h.init --> (h.finitialize, h.fcurrent) 

When more complicated initialization is required, use 
:class:`FInitializeHandler` objects or substitute a 
new procedure for the default init procedure; e.g.


.. code-block::
    python

    h('proc init() {finitialize(v_init) nrnpython("myinit()")}')

    def myinit():
        # new code to happen after initialization here
        print('initializing...')
        # only need the following if states have been changed
        if h.cvode.active():
            h.cvode.re_init()
        else:
            h.fcurrent()
        h.frecord_init()


.. seealso::
    :func:`finitialize`, :meth:`CVode.re_init`, :func:`fcurrent`, :func:`frecord_init`, :class:`FInitializeHandler`

.. _runctrl_stop:

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
calls to :func:`fadvance`.

Tstop
~~~~~

Stop time for InitRun 

.. _plotsms:

Plotsms
~~~~~~~

Number of integration steps per millisecond at which plots occur. 
Notice that reducing dt does not by itself increase the number 
of points plotted. If the the step is not an integral multiple of 
dt then dt is rounded down to the nearest integral multiple. 

Quiet
~~~~~

When checked, turns off movies and graph flushing during 
an :ref:`runcontrol_initrun`. Under some circumstances this can speed 
things up very considerably such as when using the :ref:`RunFitter` 
in the presence of a Shape Movie plot under MSWINDOWS. 

RealTime
~~~~~~~~

Running display of computation time. Resolution is 1 second. 
     

