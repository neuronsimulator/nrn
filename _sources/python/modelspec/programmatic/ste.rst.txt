.. _ste:

StateTransitionEvent
--------------------

.. class:: StateTransitionEvent

  Syntax:
    ``ste = h.StateTransitionEvent(nstate, [pointprocess])``

  Description:
    A StateTransitionEvent describes a finite state machine which is computed during a simulation run and moves
    instantaneously from one state to another as trigger threshold conditions become true according to
    transitions defined by the set of ``ste.transition`` specifications. Generally, it is the
    case that when a transition occurs, a callback is executed.
    
    `nstate` is the number of states available to the machine and must be > 0 (1 is valid). If a state index, ``istate``,
    is not the destination of a :meth:`StateTransitionEvent.transition`, then the only way to reach
    it is via an interpreter call to :meth:`StateTransitionEvent.state` with arg ``istate``.  If ``istate`` is not
    the source for a transition, then the only exit from it is when a transition enters it and the consequent callback
    executes a :meth:`StateTransitionEvent.state` with arg different from ``istate``.
    
    The ``pointprocess`` arg is needed only if the simulation uses multiple threads or the local variable time
    step method. (an admittedly grotesque requirement to give a hint as to which thread and cell is appropriate for
    all the trigger variables specified by the transitions)
    
  Example:

    .. code-block::
      python
      
      from neuron import h
      h.load_file("stdrun.hoc") # use h.run(), h.cvode, etc
      
      soma = h.Section(name="soma") # empty model not allowed.
      ste = h.StateTransitionEvent(1)

      tnext = h.ref(1)
      
      def fteinit():
        tnext[0] = 1.0 # first transition at 1.0
        ste.state(0)   # initial state

      fih = h.FInitializeHandler(1, fteinit)

      def foo(src): # current state is the destination. arg gives the source
        print(f'{h.t} transition {src} {int(ste.state())} t-tnext = {h.t-tnext[0]}')
        tnext[0] += 1.0 # update for next transition
      
      ste.transition(0, 0, h._ref_t, tnext, (foo, 0))

      print("default dt=0.025 fixed step run")
      h.run()
      
      h.steps_per_ms = 64
      h.dt = 1.0/h.steps_per_ms
      print(f"dt=1/64 fixed step run {h.dt}")
      h.run()

      for i in [1,2]:
        h.cvode.condition_order(i)
        print(f"cvode.condition_order() = {h.cvode.condition_order()}")
        h.cvode_active(True)
        h.run()

    The results of a run are:
    
    .. code-block::
      none
      
      $ nrniv -python temp.py
      NEURON -- VERSION 7.4 (1353:fa0eeb93b0fb) 2015-07-22
      Duke, Yale, and the BlueBrain Project -- Copyright 1984-2015
      See http://www.neuron.yale.edu/neuron/credits
      
      default dt=0.025 fixed step run
      1.025  transition  0 0  t-tnext = 0.025
      2.025  transition  0 0  t-tnext = 0.025
      3.0  transition  0 0  t-tnext = 8.881784197e-15
      4.0  transition  0 0  t-tnext = 2.30926389122e-14
      dt=1/64 fixed step run  0.015625
      1.015625  transition  0 0  t-tnext = 0.015625
      2.015625  transition  0 0  t-tnext = 0.015625
      3.015625  transition  0 0  t-tnext = 0.015625
      4.015625  transition  0 0  t-tnext = 0.015625
      cvode.condition_order() = 1
      3.43225906488  transition  0 0  t-tnext = 2.43225906488
      cvode.condition_order() = 2
      1.0  transition  0 0  t-tnext = -1.11022302463e-16
      2.0  transition  0 0  t-tnext = 0.0
      3.0  transition  0 0  t-tnext = 0.0
      4.0  transition  0 0  t-tnext = 0.0
      5.0  transition  0 0  t-tnext = 0.0
      >>> 

    Note that the dt=0.025 fixed step run exhibits round off errors with respect to repeated addition of dt to t
    when dt is not an exact binary fraction.
    
    Note that when dt is an exact binary fraction (1/64) and the trigger variable exactly equals the trigger
    threshold, that does not constitute (triggervar - triggerthreash > 0) == true and so the transition occurs at
    the end of the next step.
    
    Note that cvode with condition order 1 uses very large time steps with this trivial model. This is not necessarily
    a problem in practice as time steps are generally quite small when states are changing rapidly. However, one
    should consider the benefits of condition order 2.

----

.. method:: StateTransitionEvent.state

  Syntax:
    ``istate = ste.state()``
    
    ``ste.state(istate)``

  Description:
  With no args, returns the index of the current state. With an arg, sets the current state to the ``istate`` index.
  
  When setting a state, the transitions from the previous state are deactivated and all the transitions leaving the
  ``istate`` index become possible during future time steps.
  
  The user should supply a type 1 :class:`FInitializeHandler` callback to set the initial state index (and perhaps set
  state dependent transition trigger threshold values)
  when a new simulation run begins.
  
----

.. method:: StateTransitionEvent.transition

  Syntax:
    ``ste.transition(isrcstate, ideststate, _ref_triggervar, _ref_triggerthresh, pycallable)``
  
  Description:
    Adds a transition from the ``isrcstate`` of the StateTransitionEvent instance to the ``ideststate``.
    ``Isrcstate`` and ``ideststate`` must be >= 0 and < ``nstate`` (number of states specified in the constructor).
    ``Isrcstate`` == ``ideststate`` is allowed.
    
    A transition occurs when ``triggervar`` becomes greater than ``triggerthresh``. Note: with the fixed step methog a transition does NOT
    occur when it merely becomes equal. Note: a transition does not occur if the isrcstate is entered and triggervar
    is greater than triggerthresh - :data:`float_epsilon`. ie. triggervar must first become not greater than triggervar and then become greater
    for the transition to occur. (The value of float_epsilon is used internally to prevent undesirable multiple events due to round-off error when
    cvode.condition_order is activated and transition destination is the same as source. (Another way of preventing premature firing of state transitions
    is to instead move to a different state and move back via a transition with a slightly higher threshold)
    
    On each time step, the transitions from a source state are checked in the order in which they are created
    and the first true condition
    specifies the transition to be taken. But note a subtlety with regard to the variable step methods 
    with cvode.condition_order(2). Since that
    involves interpolation back to the time at which the threshold crossing actually occurred, the transition with
    the earliest crossing will be the one actually taken.

    The ``triggervar`` may be the NEURON time variable t
    (in this case, pass ``h._ref_t`` for the ``_ref_triggervar`` argument.
    This will work properly with threads and local variable time steps
    as the system will point to the correct thread/cvode instance time. NEURON time as a ``triggerthresh``
    will work correctly
    only for single thread fixed and global variable step methods and otherwise allow a race condition. Note that
    with multiple threads or the local variable time step method. All ``triggervar`` for a given ``ste`` need to be
    in the same thread or cell as was specified by the StateTransitionEvent constructor.
    
    The direction sense of threshold crossing can be reversed by reversing the order of the ``_ref_triggervar`` and ``_ref_triggerthresh`` args.
   
    In Python, the syntax for a triggervar reference is, for example, h._ref_t or sec(.5)._ref_v . A reference to a
    hoc variable is also allowed for a triggerthreash, but if the triggerthresh is a constant, one can declare a Python
    reference with triggerthresh = h.ref(value) and pass that for the ``triggerthresh`` arg.
    One changes its value via the
    ``triggerthresh[0] = ...`` syntax. Since the ste object keeps pointers to these values, it is very important that
    triggerthresh not be destroyed unless the ste instance is also destroyed.
    
    ``statement`` or ``pycallable`` are optional arguments. They are executed when the transition takes place. Note that number of
    distinct def for pycallable for each transition can be reduced by using the syntax for callback with args, ``(pycallable, (arg1, arg2,...))``
    and if a callback arg is a list or dict, it can be changed by the pycallable.
    
  Bugs:
    A time ``triggervar`` is handled the same way as any other range variable such as membrane potential. That is,
    it is compared every time step to its corresponding ``triggerthresh``.
    It would be more efficient in most cases to handle it as a normal time event. Perhaps a time event method will
    be eventually integrated into the StateTransitionEvent class. Note that cvode.event(tevent, callback) is almost
    ok as it is easy to activate the transition when entering the source state. However, one must remember to logically
    deactivate it if a different transition leaving the source state takes place.
    
    Internal pointers to ``Triggervar`` and ``triggerthresh`` do not know if those variables have been destroyed.
    To avoid using freed memory, it is up to the user to avoid this possibility.
    
    That a transition requires a threshold crossing can be occasionally limiting when one wished to check a condition
    and immediately leave a state on entering it. However, the callback can change the current state and that will
    become the activated state on return from the callback.
        
  
