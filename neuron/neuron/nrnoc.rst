.. _nrnoc:

         
Functions
---------

These are Neuron-specific functions which you can call from 
the nrnoc interpreter. 
 


.. function:: batch_run


    Syntax:
        ``batch_run(tstop, tstep, "filename")``

        ``batch_run(tstop, tstep, "filename", "comment")``


    Description:
        This command replaces the set of commands: 

        .. code-block::
            none

            while (t < tstop) { 
                    for i=0, tstep/dt { 
                            fadvance() 
                    } 
                    // print results to filename 
            } 

        and produces the most efficient run on any given neuron model.  This 
        command was created specifically for Cray computers in order eliminate 
        the interpreter overhead as the rate limiting simulation step. 
         
        This command will save selected variables, as they are changed in the run, 
        into a file whose name is given as the third argument. 
        The 4th comment argument is placed at the beginning of the file. 
        The :func:`batch_save` command specifies which variable are to be saved. 
         

         
         

----



.. function:: batch_save


    Syntax:
        ``batch_save()``

        ``batch_save(&var, &var, ...)``


    Description:


        ``batch_save()`` 
            starts a new list of variables to save in a :func:`batch_run` . 

        ``batch_save(&var, &var, ...)`` 
            adds pointers to the list of variables to be saved in a ``batch_run``. 
            A pointer to a range variable, eg. "v", must have an explicit 
            arc length, eg. axon.v(.5). 

         

    Example:

        .. code-block::
            none

            batch_save()	//This clears whatever list existed and starts a new 
            		//list of variables to be saved. 
            batch_save(&soma.v(.5), &axon.v(1)) 
            for i=0,2 { 
            	batch_save(&dend[i].v(.3)) 
            } 

        specifies five quantities to be saved from each ``batch_run``. 

         

----



.. function:: initnrn


    Syntax:
        ``initnrn()``


    Description:
        Initialize ``t, dt, clamp_resist``, and ``celsius`` to the values 
        they had when the program was first run. 
         
        Note that in this 
        version ``Ra`` is no longer a global variable but a section variable 
        like *L* and *rallbranch*. Thus ``Ra`` can be different for different 
        sections.  In order to set ``Ra`` to a constant value, use: 
         
        ``forall Ra=...`` 

    .. warning::
        Not very useful. No way to completely restart neuron exect to :func:`quit` and 
        re-load. 


----



.. function:: fadvance


    Syntax:
        ``fadvance()``


    Description:
        Integrate all section equations over the interval :data:`dt` . 
        The value of :data:`t` is incremented by dt. 
        The default method is first order implicit but may be changed to 
        Crank-Nicholson by changing :data:`secondorder` = 2. 
         
        fadvance integrates the equation over the dt step by 
        calling all the BREAKPOINT blocks of models at t+dt/2 twice with 
        v+.001 and v in order to compute the current and conductance to form 
        the matrix conductance*voltage = current. 
        This matrix is then solved for v(t+dt). 
        (if secondorder == 2 the ionic currents are adjusted to be second order 
        correct. If secondorder == 1 the ionic currents are not adjusted but 
        the voltages are second order correct) 
        Lastly the SOLVE statement within the BREAKPOINT block of models is 
        executed with t+dt and the new values of v in order to integrate those 
        states (from new t-.5dt to new t+.5dt). 

         

----



.. function:: finitialize


    Syntax:
        ``finitialize()``

        ``finitialize(v)``


    Description:
        Call the INITIAL block for all mechanisms and point processes 
        inserted in the sections. 
        If the optional argument is present then all voltages of all sections 
        are initialized to *v*. 
        :data:`t` is set to 0. 
         
        The order of principal actions during an finitialize call is:
        
        -   t = 0 
        -   Clear the event queue. 
        -   Random.play values assigned to variables. 
        -   Make sure internal structures needed by integration methods are consistent 
             with the current biophysical spec. 
        -   Vector.play at t=0 values assigned to variables. 
        -   All v = arg if the arg is present. 
        -   Type 0 FInitializeHandler statements executed. 
        -   All mechanism BEFORE INITIAL blocks are called. 
        -   All mechanism INITIAL blocks called. 
               Mechanisms that WRITE concentrations are after ion mechanisms and 
               before mechanisms that READ concentrations. 
        -   LinearMechanism states are initialized 
        -   INITIAL blocks inside NETRECEIVE blocks are called. 
        -   All mechanism AFTER INITIAL blocks are called. 
        -   Type 1 FInitializeHandler statements executed. 
        -   The INITIAL block net_send(0, flag) events are delivered. 
        -   Effectively a call to CVode.re_init or fcurrent(), whichever appropriate. 
        -   Various record functions at t=0. e.g. CVode.record, Vector.record  
        -   Type 2 FInitializeHandler statements executed. 
             


    .. seealso::
        :class:`FInitializeHandler`, :ref:`runcontrol_Init`, :meth:`CVode.re_init`, :func:`fcurrent`, :func:`frecord_init`

         

----



.. function:: frecord_init


    Syntax:
        ``frecord_init()``


    Description:
        Initializes the Vectors which are recording variables. i.e. resize to 0 and 
        append the current values of the variables.  This is done at the end 
        of an :func:`finitialize` call but needs to be done again to complete initialization 
        if the user changes states or assigned variables that are being recorded.. 

    .. seealso::
        :meth:`Vector.record`, :ref:`runcontrol_Init`

         

----



.. function:: fstim


    Syntax:
        ``fstim()``


    Description:
        Consider this obsolete.  Nevertheless, it does work. See the old NEURON reference 
        manual. 

         

----



.. function:: fstimi


    Syntax:
        ``fstimi()``


    Description:
        Obsolete 

         

----



.. function:: fit_praxis


    Syntax:
        ``min = fit_praxis(n, "funname", &x[0])``

        ``min = fit_praxis(n, "funname", Vector)``

        ``min = fit_praxis(..., ..., ..., "after quad statement")``

        ``min = fit_praxis(efun_as_python_callable, hoc_vector)``


    Description:
        This is the principal axis method for minimizing a function. See praxis.c 
        in the scopmath library. 
         


        ``1 <= n < 20`` 
            is the number of parameters to vary (number 
            of arguments to *funname*). 

        *funname* 
            the name of the function to minimize, eg. least square difference between model and data. 
            The funname must take two arguments, the first arg, ``$1``, 
            is the number of elements in second arg vector, ``$&2``. The ith index of the 
            vector is given by ``$&2[i]``. 

        *x* 
            is a double vector of at least length *n*. Prior to the call set 
            it to a guess of the parameter values. On return it contains the 
            values of the args that minimize ``funname()``. 

         
        *funname* may be either 
        an interpreted hoc function or a compiled NMODL function. 
         
        If the variable stoprun is set to 1 during a call to fit_praxis, it will 
        return immediately (when the current call to  funname returns) with 
        a return value and varx values set to the best minimum found so far. Use 
        :func:`stop_praxis` to stop after finishing the current principal axis calculation. 
         
        The fourth argument, if present, specifies a statement to be executed at 
        the end of each principal axis evaluation. 
         
        If the third argument is a Vector, then that style is used to specify 
        the initial starting point and return the final value. However the 
        function is still called with second arg as a pointer into a double array. 
         
        The Python callable form uses a Python Callable as the function to 
        minimize and it must take a single hoc Vector argument specifying the 
        values of the parameters for use in evaluation the function. On entry to 
        fit_praxis the Vector specifies the number of parameters and the 
        parameter starting values. On return the vector contains the values of 
        parameters which generated the least minimum found so far. 
         
        Hoc example: minimize ``(x+y - 5)^2 + 5*((x-y) - 15)^2``

        .. code-block::
            none

            objref vec 
            vec = new Vector(2) // vec.x[0] is x, vec.x[1] is y 
            func efun() {local x, y 
              x = $&2[0]  y = $&2[1] 
              return (x+y - 5)^2 + 5*(x-y - 15)^2 
            } 
            attr_praxis(1e-5, .5, 0) 
            e = fit_praxis(vec.size(), "efun", vec) 
            printf("e=%g x=%g y=%g\n", e, vec.x[0], vec.x[1]) 
             
            objref paxis 
            paxis = new Vector() 
            for i=0, 1 { 
              pval = pval_praxis(i, paxis) 
              printf("%d  %10g      %10g %10g\n", i, pval, paxis.x[0], paxis.x[1]) 
            } 

         
        Python example: 

        .. code-block::
            none

            from neuron import h 
            v = h.Vector(2) 
            def efun(v): 
              return (v.x[0]+v.x[1] - 5)**2 + 5*(v.x[0]-v.x[1] - 15)**2 
            h.attr_praxis(1e-5, .5, 0) 
            e = h.fit_praxis(efun, v) 
            print "e=%g x=%g y=%g\n"%(e, v.x[0], v.x[1]) 


    .. warning::
        Up to version 4.0.1, the arguments to *funname* were an explicit 
        list of *n* arguments. ie ``numarg()==n``. 

    .. seealso::
        :func:`attr_praxis`, :func:`stop_praxis`, :func:`pval_praxis`

         

----



.. function:: attr_praxis


    Syntax:
        ``attr_praxis(tolerance, maxstepsize, printmode)``

        ``previous_index = attr_praxis(mcell_ran4_index)``


    Description:
        Set the attributes of the praxis method. This must be called before 
        the first call to :func:`fit_praxis`. 


        tolerance 
            praxis attempt to return f(x) such that if x0 is the true 
            local minimum then ``norm(x-x0) < tolerance`` 

        maxstepsize 
            should be set to about the maximum distance from 
            initial guess to the minimum. 

        printmode=0 
            	no printing 

        printmode=1,2,3 
            more and more verbose 

        The single argument form causes praxis to pick its random numbers from 
        the the mcellran4 generator beginning at the specified index. This 
        allows reproducible fitting. The return value is the previously picked 
        index. (see :func:`mcell_ran4`) 

         

----



.. function:: pval_praxis


    Syntax:
        ``pval = pval_praxis(i)``

        ``pval = pval_praxis(i, &paxis[0])``

        ``pval = pval_praxis(i, Vector)``


    Description:
        Return the ith principal value. If the second argument is present, ``pval_praxis`` also fills 
        the vector with the ith principal axis. 

         

----



.. function:: stop_praxis


    Syntax:
        ``stop_praxis()``

        ``stop_praxis(i)``


    Description:
        Set a flag in the praxis function that will cause it to stop after 
        it finishes the current (or ith subsequent) 
        principal axis calculation. If this function 
        is called before :func:`fit_praxis`, then praxis will do a single 
        (or i) principal axis calculation and then exit. 

         

----



.. function:: fclamp


    Syntax:
        ``fclamp()``


    Description:
        obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process. 

         

----



.. function:: fclampi


    Syntax:
        ``fclampi()``


    Description:
        obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process. 

         

----



.. function:: fclampv


    Syntax:
        ``fclampv()``


    Description:
        obsolete. Use the :class:`VClamp` or :class:`SEClamp` point process. 

         

----



.. function:: prstim


    Syntax:
        ``prstim()``


    Description:
        obsolete. Print the info about ``fstim``, ``fclamp``, and ``fsyn`` 

         

----



.. function:: fcurrent


    Syntax:
        ``fcurrent()``


    Description:
        Make all assigned variables (currents, conductances, etc) 
        consistent with the values of the states. Useful in combination 
        with :func:`finitialize`. 

    Example:

        .. code-block::
            none

            create soma 
            access soma 
            insert hh 
            print "default el_hh = ", el_hh 
            // set el_hh so that the steady state is exactly -70 mV 
            finitialize(-70) // sets v to -70 and m,h,n to corresponding steady state values 
             
            fcurrent()	// set all assigned variables consistent with states 
             
            // use current balance: 0 = ina + ik + gl_hh*(v - el_hh)		 
            el_hh = (ina + ik + gl_hh*v)/gl_hh 
             
            print "-70 mV steady state el_hh = ", el_hh 
            fcurrent()	// recalculate currents (il_hh) 


         

----



.. function:: fmatrix


    Syntax:
        ``fmatrix()``

        ``section {value = fmatrix(x, index)}``


    Description:
        No args: print the jacobian matrix for the tree structure in a particularly 
        confusing way. for debugging only. 
         
        With args, return the matrix element associated with the integer index 
        in the row corresponding to the currently accessed 
        section at position x. The index 1...4 is associated with: 
        The coeeficient for the effect of this locations voltage on current balance at the parent location, 
        The coeeficient for the effect of this locations voltage on current balance at this location, 
        The coeeficient for the effect of the parent locations voltage on current balance at this location, 
        The right hand side of the matrix equation for this location. These are the 
        values of NODEA, NODED NODEB, and NODERHS respectively in 
        nrn/src/nrnoc/section.h . The matrix elements are properly setup on return 
        from a call to the :func:`fcurrent` function. For the fixed step method 
        :func:`fadvance` modifies NODED and NODERHS 
        but leaves NODEA and NODEB unchanged. 

         

----



.. function:: issection


    Syntax:
        ``issection("regular expression")``


    Description:
        Return 1 if the currently accessed section matches the regular expression. 
        Return 0 if otherwise. 
         
        Regular expressions are like those of grep except {} are used 
        in place of [] to avoid conflict with indexed sections. Thus 
        a[{8-15}] matches sections a[8] through a[15]. 
        A match always begins from the beginning of a section name. If you 
        don't want to require a match at the beginning use the dot. 
         
        (Note, 
        that ``.`` matches any character and ``*`` matches 0 or more occurrences 
        of the previous character). The interpreter always closes each string with 
        an implicit ``$`` to require a match at the end of the string. If you 
        don't require a match at the end use "``.*``". 

    Example:

        .. code-block::
            none

            create soma, axon, dendrite[3] 
            forall if (issection("s.*")) { 
            	print secname() 
            } 

        will print ``soma`` 

        .. code-block::
            none

            forall if (issection("d.*2]")) { 
            	print secname() 
            } 

        will print ``dendrite[2]`` 

        .. code-block::
            none

            forall if (issection(".*a.*")) { 
            	print secname() 
            } 

        will print all names which contain the letter "a" 

        .. code-block::
            none

            soma 
            axon 


    .. seealso::
        :ref:`ifsec <keyword_ifsec>`, :ref:`forsec <keyword_forsec>`

         

----



.. function:: ismembrane


    Syntax:
        ``ismembrane("mechanism")``


    Description:
        This function returns a 1 if the current membrane contains this 
        (density) mechanism.  This is not for point 
        processes. 
         

    Example:

        .. code-block::
            none

            forall if (ismembrane("hh") && ismembrane("ca_ion")) { 
            	print secname() 
            } 

        will print the names of all the sections which contain both Hodgkin-Huxley and Calcium ions. 

         

----



.. function:: sectionname


    Syntax:
        ``sectionname(strvar)``


    Description:
        The name of the currently accessed section is placed in *strvar*. 
         
        This function is superseded by the easier to use, :func:`secname`. 

         

----



.. function:: secname


    Syntax:
        ``secname()``


    Description:
        Returns the currently accessed section name. Usage is 

        .. code-block::
            none

            		strdef s 
            		s = secname() 

        or 

        .. code-block::
            none

            		print secname() 

        or 

        .. code-block::
            none

            		forall for(x) printf("%s(%g)\n", secname(), x) 


         

----



.. function:: psection


    Syntax:
        ``psection()``


    Description:
        Print info about currently accessed section in a format which is executable. 
        (length, parent, diameter, membrane information) 
         

         
         

----



.. data:: secondorder


    Syntax:
        ``secondorder``


    Description:
        This is a global variable which specifies the time integration method. 


        =0 
            default fully implicit backward euler. Very numerically stable. 
            gives steady state in one step when *dt=1e10*. Numerical errors 
            are proportional to :data:`dt`. 

        =1 
            crank-nicholson Can give large (but damped) numerical error 
            oscillations. For small :data:`dt` the numerical errors are proportional 
            to ``dt^2``. Cannot be used with voltage clamps. Ionic currents 
            are first order correct. Channel conductances are second order 
            correct when plotted at ``t+dt/2`` 

        =2 
            crank-nicholson like 1 but in addition Ion currents (*ina*, *ik*, 
            etc) are fixed up so that they are second order correct when 
            plotted at ``t-dt/2`` 


         

----



.. data:: t


    Syntax:
        ``t``


    Description:
        The global time variable. 

         

----



.. data:: dt


    Syntax:
        ``dt``


    Description:
        The integration interval for :func:`fadvance`. 
         
        When using the default implicit integration method (:data:`secondorder` = 0) 
        there is no upper limit on dt for numerical stability and in fact for 
        passive models it is often convenient to use dt=1.9 to obtain the 
        steady state in a single time step. 
         
        dt can be changed by the user at any time during a simulation. However, 
        some inserted mechanisms may use tables which depend on the value of dt 
        which will be automatically recomputed. In this situation, the tables 
        are not useful and should be bypassed by setting the appropriate 
        usetable_suffix global variables to 0. 

         

----



.. data:: clamp_resist


    Syntax:
        ``clamp_resist``


    Description:
        Obsolete, used by fclamp. 

         

----



.. data:: celsius


    Syntax:
        ``celsius = 6.3``


    Description:
        Temperature in degrees centigrade. 
         
        Generally, rate function tables (eg. used by the hh mechanism) 
        depend on temperature and will automatically be re-computed 
        whenever celsius changes. 

         

----



.. data:: stoprun


    Syntax:
        ``stoprun``


    Description:
        A flag which is watched by :func:`fit_praxis`, :class:`CVode`, and other procedures 
        during a run or family of runs. 
        When stoprun==1 they will immediately return without completing 
        normally. This allows safe stopping in the middle of a long run. Procedures 
        that do multiple runs should check stoprun after each run and exit 
        gracefully. The :meth:`RunControl.Stop` of the RunControl GUI sets this variable. 
        It is cleared at the beginning of a run or when continuing a run. 

         
         

----



.. function:: this_section


    Syntax:
        ``this_section(x)``


    Description:
        Return a pointer (coded as a double) to the section which contains location 0 of the 
        currently accessed section. This pointer can be used as the argument 
        to :func:`push_section`. Functions that return pointers coded as doubles 
        are unsafe with 64 bit pointers. This function has been superseded by 
        :class:`SectionRef`. See :meth:`~SectionRef.sec`. 

         

----



.. function:: this_node


    Syntax:
        ``this_node(x)``


    Description:
        Return a pointer (coded as a double) to the segment 
        of the currently accessed section that 
        contains location *x*. If you wish to compute a segment number 
        index where 1 is the first nonzero area segment and nseg is the last 
        nonzero area segment 
        of the currently accessed section corresponding to position x use the 
        hoc function 

        .. code-block::
            none

            func segnum() { 
                    if ($1 <= 0) { 
                            return 0 
                    }else if ($1 >= 1) { 
                            return nseg+1 
                    }else { 
                            return int($1*nseg + .5) 
                    } 
            } 


    .. warning::
        This function is useless and should be removed. 

         

----



.. function:: parent_section


    Syntax:
        ``parent_section(x)``


    Description:
        Return the pointer to the section parent of the segment containing *x*. 
        Because a 64 bit pointer cannot safely be represented as a 
        double this function is deprecated in favor of :meth:`SectionRef.parent`. 

         

----



.. function:: parent_node


    Syntax:
        ``parent_node(x)``


    Description:
        Return the pointer of the parent of the segment containing *x*. 

    .. warning::
        This function is useless and currently returns an error. 

         

----



.. function:: parent_connection


    Syntax:
        ``y = parent_connection()``


    Description:
        Return location on parent that currently accessed section is 
        connected to. (0 <= x <= 1). This is the value, y, used in 

        .. code-block::
            none

                    connect child(x), parent(y) 


         

----



.. function:: section_orientation


    Syntax:
        ``y = section_orientation()``


    Description:
        Return the end (0 or 1) which connects to the parent. This is the 
        value, x, used in 
         

        .. code-block::
            none

                    connect child(x), parent(y) 


         
         

----


Compile Time Options
====================

        The following definitions are found in nrnoc/SRC/options.h and add extra 
        functionality which not everyone may need. The extras come at the cost 
        of larger memory requirements for node and section structures. METHOD3 is too large 
        and obscure to benefit most users. 

        .. code-block::
            none

            #define VECTORIZE	1	/* hope this speeds up simulations on a Cray */ 
            				/* this is no longer optional */ 
             
            #define EXTRACELLULAR	1	/* extracellular membrane mechanism */ 
             
            #define DIAMLIST	1	/* section contains diameter info */ 
            				/* shape plots make use of this */ 
             
            #define EXTRAEQN	0	/* ionic concentrations calculated via 
            				 * jacobian along with v (not implemented) */ 
            #if DIAMLIST 
            #define NTS_SPINE	1	/* A negative diameter in pt3dadd() tags that 
            				 * diamlist location as having a spine. 
            				 * diam3d() still returns the positive diameter 
            				 * spined3d() returns 1 or 0 signifying presence 
            				 * of spine. setSpineArea() tells how much 
            				 * area/spine to add to the segment. */ 
            #endif 
             
            #define METHOD3		1	/* third order spatially correct method */ 
            				/* testing only, not completely implemented */ 
            				/* not working at this time */ 
             
            #if METHOD3 
             spatial_method(i) 
            	no arg, returns current method 
            	i=0 The standard NEURON method with zero area nodes at the ends 
            		of sections. 
            	i=1 conventional method with 1/2 area end nodes 
            	i=2 modified second order method 
            	i=3 third order correct spatial method 
            	Note: i=1-3 don't work under all circumstances. They have been 
            	insufficiently tested and the correctness must be established for 
            	each simulation. 
            #endif 
             
            #if NEMO 
            neuron2nemo("filename") Beginning of translator between John Millers 
            		nemosys program	and NEURON. Probably out of date. 
            nemo2neuron("filename") 
            #endif 


         
         

----



Ion
===



.. function:: ion_style


    Syntax:
        ``oldstyle = ion_style("name_ion", c_style, e_style, einit, eadvance, cinit)``

        ``oldstyle = ion_style("name_ion")``


    Description:
        In the currently accessed section, 
        force the named ion (eg. na_ion, k_ion, ca_ion, etc) to handle 
        reversal potential and concentrations according to the indicated 
        styles. 
        You will not often need this function since the 
        style chosen automatically on a per section basis should be 
        appropriate to the set of mechanisms inserted in each section. 
         
        Warning: if other mechanisms are inserted subsequent to a call 
        of this function, the style will be "promoted" according to 
        the rules associated with adding the used ions to the style 
        previously in effect. 
         
        The oldstyle value is previous internal setting of 
        c_style + 4*cinit +  8*e_style + 32*einit + 64*eadvance. 
         


        c_style: 0, 1, 2, 3. 
            Concentrations respectively treated as UNUSED, 
            PARAMETER, ASSIGNED, or STATE variables.  Determines which panel (if 
            any) will show the concentrations. 

        e_style: 0, 1, 2, 3. 
            Reversal potential respectively treated as 
            UNUSED, PARAMETER, ASSIGNED, or STATE variable. 

        einit: 0 or 1. 
            If 1 then reversal potential computed by Nernst equation 
            on call to ``finitialize()`` using values of concentrations. 

        eadvance: 0 or 1. 
            If 1 then reversal potential computed every call to 
            ``fadvance()`` using the values of the concentrations. 

        cinit: 0 or 1. 
            If 1 then a call to finitialize() sets the concentrations 
            to the values of the global initial concentrations. eg. ``nai`` set to 
            ``nai0_na_ion`` and ``nao`` set to ``nao0_na_ion``. 

         
        The automatic style is chosen based on how the set of mechanisms that 
        have been inserted in a section use the ion. Note that the precedence is 
        WRITE > READ > unused in the USEION statement; so if one mechanism 
        READ's  cai/cao and another mechanism WRITE's them then WRITE takes precedence 
        in the following table. For compactness, the table assumes the ca ion. 
        Each table entry identifies the equivalent parameters to the ion_style 
        function. 

            ==========    =========   =========   =========
            cai/cao ->    unused      read        write 
            ==========    =========   =========   =========
            eca unused    0,0,0,0,0   1,0,0,0,0   3,0,0,0,1 
            eca read      0,1,0,0,0   1,2,1,0,0   3,2,1,1,1 
            eca write     0,2,0,0,0   1,2,0,0,0   3,2,0,0,1 
            ==========    =========   =========   =========

        For example suppose one has inserted a mechanism that READ's eca, 
        a mechanism that READ's cai, cao and a mechanism that WRITE's cai, cao 
        Then, since WRITE takes precedence over READ in the above table, 
        ``cai/cao`` would appear in the STATE variable panel (first arg is 3), 
        ``eca`` would appear in the ASSIGNED variable panel (second arg is 2), 
        ``eca`` would be calculated on a call to finitialize (third arg is 1), 
        ``eca`` would be calculated on every call to fadvance (fourth arg is 1), 
        ``cai/cao`` would be initialized (on finitialize) to the global variables 
        ``cai0_ca_ion`` and ``cao0_ca_ion`` respectively. (note that this takes place just 
        before the calculation of ``eca``). 

         

----



.. function:: ghk


    Syntax:
        ``ghk(v, ci, co, charge)``


    Description:
        Return the Goldman-Hodgkin-Katz current (normalized to unit permeability). 
        Use the present value of celsius. 

        .. code-block::
            none

            mA/cm2 = (permeability in cm/s)*ghk(mV, mM, mM, valence) 


         

----



.. function:: nernst


    Syntax:
        ``nernst(ci, co, charge)``

        ``nernst("ena" or "nai" or "nao", [x])``


    Description:


        ``nernst(ci, co, charge)`` 
            returns nernst potential. Utilizes the present value of celsius. 

        ``nernst("ena" or "nai" or "nao", [x])`` 
            calculates ``nao/nai = exp(z*ena/RTF)`` for the ionic variable 
            named in the string. 

        Celsius, valence, and the other two ionic variables are taken from their 
        values at the currently accessed section at position x (.5 default). 
        A hoc error is printed if the ionic species does not exist at this location. 

         

----



.. function:: ion_register


    Syntax:
        ``type = ion_register("name", charge)``


    Description:
        Create a new ion type with mechanism name, "name_ion", and associated 
        variables: iname, nameo, namei, ename, diname_dv. 
        If any of these names already 
        exists and name_ion is not already an ion, the function returns -1, 
        otherwise it returns the mechanism type index. If name_ion is already 
        an ion the charge is ignored but the type index is returned. 


----



.. function:: ion_charge


    Syntax:
        ``charge = ion_charge("name_ion")``


    Description:
        Return the charge for the indicated ion mechanism. An error message is 
        printed if name_ion is not an ion mechanism. 


