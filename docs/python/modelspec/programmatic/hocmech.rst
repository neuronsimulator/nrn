.. _hocmech:

HOC-based Mechanisms
--------------------

.. warning::

    The functions on this page create density mechanisms and point processes from
    HOC templates; they have no direct way to use a Python class, but a HOC wrapper
    is possible as is inlining a HOC template inside of a file as shown in the
    example. For faster code, use NMODL (directly or after conversion from NeuroML/LEMS),
    the channel builder, or in NEURON 7.7+ the rxd module to implement mechanisms.


.. function:: make_pointprocess

    See :func:`make_mechanism`.


.. function:: make_mechanism

    Syntax:
        ``h.make_mechanism("suffix", "Template", "parm1 parm2 parm3 ...")``

        ``h.make_pointprocess("Name", "Template", "parm1 parm2 parm3 ...")``

    Description:
        Installs the HOC (in particular, *not* Python) class called "Template" as a density membrane mechanism 
        called "suffix" or a POINT_PROCESS called Name. If the third argument exists it must be a space 
        separated list of public variables in the Template which are to be 
        treated as PARAMETERs. Public variables not in this list are treated as 
        ASSIGNED variables. The new mechanism is used in exactly the same 
        way as "hh" or mechanism defined by NMODL. Thus, instances are created 
        in each segment with ``section.insert(suffix)`` and after insertion 
        the public names are accessible via the normal range variable notation 
        as in: ``section(x).suffix.name``.
         
        At this time the functionality of such interpreter defined membrane 
        mechanisms is a small subset of the functionality of mechanisms described 
        by the model description language. Furthermore, being interpreted, they 
        are much ( more than 100 times) slower than compiled model descriptions. 
        However, it is a very powerful way to 
        watch variables, specify events for delivery as specific times, 
        receive events, and discontinuously (or continuously) modify parameters 
        during a simulation. And it works in the context of all the integration 
        methods, including local variable time steps. The following procedure 
        names within a template, if they exist, are analogous to the 
        indicated block in the model description language. In each case 
        the currently accessed section is set to the location of this instance 
        of the Template and one argument is passed, x, which is the 
        range variable arc position \ ``(0 < x < 1)``. 
         
        INITIAL: \ ``proc initial()`` 
        Called when :func:`finitialize` is executed. 
         
        BREAKPOINT {SOLVE ... METHOD after_cvode}: \ ``proc after_step()`` 
        For the standard staggered time step and global variable time step 
        integration methods, called at every :func:`fadvance` when t = t+dt. 
        For the local variable step method, the instance is called when 
        the individual cell CVode instance finished its solve step. 
        In any case, it is safe to send an event any time after t-dt. 
         

    Example:
        The following example creates and installs a mechanism that watches 
        the membrane potential and keeps track of its maximum value. 

        .. code-block::
            python

            from neuron import h, gui
            import math

            soma = h.Section(name="soma")
            soma.L = soma.diam = math.sqrt(100 / math.pi)
            soma.insert('hh')  # equivalently: soma.insert(h.hh)

            stim = h.IClamp(soma(0.5))
            stim.dur = 0.1
            stim.amp = 0.3

            # declare a mechanism using HOC
            h('''
                begintemplate Max
                    public V

                    proc initial() {
                        V = v($1)
                    }

                    proc after_step() {
                        if (V < v($1)) {
                            V = v($1)
                        }
                    }
                endtemplate Max
            ''')

            h.make_mechanism('max', 'Max')
            soma.insert('max')
            h.run()

            print('V_max = %g' % soma(0.5).max.V)
         

