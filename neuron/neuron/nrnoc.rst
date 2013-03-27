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


