.. _hocmech:

HOC-based Mechanisms
--------------------

.. warning::

    The functions on this page create density mechanisms and point processes from
    HOC templates; they have no direct way to use a Python or MATLAB class, but a HOC wrapper
    is possible as is inlining a HOC template inside of a file as shown in the
    example. For faster code, use NMODL (directly or after conversion from NeuroML/LEMS),
    the channel builder, or in NEURON 7.7+ the rxd module to implement mechanisms.


.. function:: make_pointprocess

    See :func:`make_mechanism`.


.. function:: make_mechanism

    .. tab:: Python

        Syntax:

        .. code-block:: python

            n.make_mechanism("suffix", "Template", "parm1 parm2 parm3 ...")
            n.make_pointprocess("Template", "parm1 parm2 parm3 ...")

        Description:
            Installs the HOC (in particular, *not* Python) class called "Template" as a density membrane mechanism 
            called "suffix" or a POINT_PROCESS called "Template". If the last argument exists it must be a space 
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

                from neuron import n
                import math
                n.load_file("stdrun.hoc")

                soma = n.Section("soma")
                soma.L = soma.diam = math.sqrt(100 / math.pi)
                soma.insert(n.hh)

                stim = n.IClamp(soma(0.5))
                stim.dur = 0.1
                stim.amp = 0.3

                # declare a mechanism using HOC
                n('''
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

                n.make_mechanism('max', 'Max')
                soma.insert('max')  # could also do: soma.insert(n.max)
                n.run()

                print(f'V_max = {soma(0.5).max.V}')
         


    .. tab:: HOC

        Syntax:

        .. code-block:: C++

            make_mechanism("suffix", "Template", "parm1 parm2 parm3 ...")
            make_pointprocess("Template", "parm1 parm2 parm3 ...")

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
                C++

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

    .. tab:: MATLAB

        Syntax:

        .. code-block:: matlab

            n.make_mechanism("suffix", "Template", "parm1 parm2 parm3 ...")
            n.make_pointprocess("Template", "parm1 parm2 parm3 ...")

        Description:
            Installs the HOC (in particular, *not* MATLAB) class called "Template" as a density membrane mechanism 
            called "suffix" or a POINT_PROCESS called "Template". If the last argument exists it must be a space 
            separated list of public variables in the Template which are to be 
            treated as PARAMETERs. Public variables not in this list are treated as 
            ASSIGNED variables. The new mechanism is used in exactly the same 
            way as "hh" or mechanism defined by NMODL. Thus, instances are created 
            in each segment with ``section.insert(suffix)`` and after insertion 
            the public names are accessible via the normal range variable notation 
            as in: ``section(x).name_suffix`` (e.g., the ``m`` state variable of the
            ``hh`` mechanism is accessed via ``section(x).m_hh``.
            
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
                matlab

                n = neuron.launch();
                n.load_file("stdrun.hoc");

                soma = n.Section("soma");
                soma.L = sqrt(100 / pi);
                soma.diam = soma.L;
                soma.insert("hh");

                stim = n.IClamp(soma(0.5));
                stim.dur = 0.1;
                stim.amp = 0.3;

                % declare a mechanism using HOC
                n(sprintf([...
                    'begintemplate Max\n' ...
                    '    public V\n' ...
                    '    proc initial() {\n' ...
                    '        V = v($1)\n' ...
                    '    }\n' ...
                    '    proc after_step() {\n' ...
                    '        if (V < v($1)) {\n' ...
                    '            V = v($1)\n' ...
                    '        }\n' ...
                    '    }\n' ...
                    'endtemplate Max\n']));

                n.make_mechanism('max', 'Max');
                soma.insert('max');
                n.run();

                fprintf('V_max = %g\n', soma(0.5).V_max);
         