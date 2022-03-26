
.. _hoc_hocmech:

HOC-based Mechanisms
--------------------

         


.. hoc:function:: hocmech


    Syntax:
        ``make_mechanism("suffix", "Template", "parm1 parm2 parm3 ...")``
        ``make_pointprocess("Name", "Template", "parm1 parm2 parm3 ...")``

    Description:
        Installs the hoc class called "Template" as a density membrane mechanism 
        called "suffix" or a POINT_PROCESS called Name. If the third argument exists it must be a space 
        separated list of public variables in the Template which are to be 
        treated as PARAMETERs. Public variables not in this list are treated as 
        ASSIGNED variables. The new mechanism is used in exactly the same 
        way as "hh" or mechanism defined by NMODL. Thus, instances are created 
        in each segment with \ ``section insert suffix`` and after insertion 
        the public names are accessible via the normal range variable notation 
        as in: \ ``section.name_suffix(x)`` 
         
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
        Called when :hoc:func:`finitialize` is executed.
         
        BREAKPOINT {SOLVE ... METHOD after_cvode}: \ ``proc after_step()`` 
        For the standard staggered time step and global variable time step 
        integration methods, called at every :hoc:func:`fadvance` when t = t+dt.
        For the local variable step method, the instance is called when 
        the individual cell CVode instance finished its solve step. 
        In any case, it is safe to send an event any time after t-dt. 
         

    Example:
        The following example creates and installs a mechanism that watches 
        the membrane potential and keeps track of its maximum value. 

        .. code-block::
            none

            load_file("noload.hoc") 
             
            create soma 
            access soma 
            { L = diam = sqrt(100/PI) insert hh} 
             
            objref stim 
            stim = new IClamp(.5) 
            {stim.dur = .1  stim.amp = .3 } 

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
             
             
            make_mechanism("max", "Max") 
            insert max 
            run() 
            print "V_max=", soma.V_max(.5) 
             


         

