
.. _hoc_mechstan:

         
MechanismStandard (Parameter Control)
-------------------------------------



.. hoc:class:: MechanismStandard


    Syntax:
        :samp:`ms = new MechanismStandard("{name}")`

        :samp:`ms = new MechanismStandard("{name}", {vartype})`


    Description:
        With no vartype or vartype = 1, this provides 
        storage for parameter values of a membrane mechanism or point process. 
        This class is useful in maintaining a default set of parameters and can 
        be used to specify values for a set of sections. 
         
        *name* is a density mechanism such as ``hh`` or a point process 
        such as :hoc:class:`VClamp`. A ``MechanismStandard`` instance, when created,
        contains default values for all parameters associated with the mechanism. 
         
        In combination with the 
        :hoc:class:`MechanismType` class it is possible to create generic graphical interface
        widgets that are independent of the particular mechanism and parameter names. 
         
        If vartype = 1, 2, or 3, the storage is for PARAMETER, ASSIGNED, or STATE 
        variables respectively. If vartype = 0, the storage is for all three types. 
         
        If vartype = -1, the count and names (and array size) 
        of the GLOBAL variables are accessible, but any other method will 
        generate an error message. 
         

    Example:

        .. code-block::
            none

            objref ms1, ms2 
            ms1 = new MechanismStandard("hh") 
            ms2 = new MechanismStandard("AlphaSynapse") 
            ms2.set("gmax", .3) 
            ms1.panel() 
            ms2.panel() 

         
        The following example prints all the names associated with POINT_PROCESS 
        and SUFFIX mechanisms. 

        .. code-block::
            none

            create soma 
            access soma 
             
            objref ms, mt 
            strdef s, msname 
            proc pname() {local i, j, k 
            	for i=-1,3 { 
            		ms = new MechanismStandard($s1, i) 
            		print "\n", $s1, "  vartype=", i 
            		for j=0, ms.count()-1 { 
            			k = ms.name(s, j) 
            			print j, s, " size=", k 
            		} 
            	} 
            } 
             
            proc ptype() {local i, j 
            	for i=0,1 { 
            		mt = new MechanismType(i) 
            		for j=0, mt.count-1 { 
            			mt.select(j) 
            			mt.selected(msname) 
            print "\n\n", msname, " mechanismtype=", j 
            			pname(msname) 
            		} 
            	} 
            } 
             
            ptype() 
             


    .. seealso::
        :hoc:class:`MechanismType`

         

----



.. hoc:method:: MechanismStandard.panel


    Syntax:
        ``ms.panel()``

        ``ms.panel("string")``


    Description:
        Popup a panel of parameters for this mechanism. It's a good idea to 
        set the default values before generating the panel. 
         
        With no argument the first item in the panel will be the name of the 
        mechanism. Otherwise the string is used as the first item label. 

    .. seealso::
        :hoc:func:`nrnglobalmechmenu`, :hoc:func:`nrnmechmenu`, :hoc:func:`nrnpointmenu`

         

----



.. hoc:method:: MechanismStandard.action


    Syntax:
        ``ms.action("statement")``


    Description:
        action to be executed when any variable is changed in the panel. 
        The hoc variable :hoc:data:`hoc_ac_` is set to the index of the variable (0 to count-1).

    Example:
        forall delete_section() 

        .. code-block::
            none

            create soma, axon, dend[3] 
            forsec "a" insert hh 
            forsec "d" insert pas 
            xpanel("Updated when MechanismStandard is changed") 
            xvalue("dend[0].g_pas") 
            xvalue("dend[1].g_pas") 
            xvalue("dend[2].g_pas") 
            xpanel() 
            objref ms 
            ms = new MechanismStandard("pas") 
            ms.action("change_pas()") 
            ms.panel() 
             
            proc change_pas() { 
            	forall if(ismembrane("pas")) { 
            		ms.out() 
            	} 
            } 


         

----



.. hoc:method:: MechanismStandard.in


    Syntax:
        ``ms.in()``

        :samp:`ms.in({x})`

        :samp:`ms.in({pointprocess})`

        :samp:`ms.in({mechanismstandard})`


    Description:
        copies parameter values into this mechanism standard from ... 


        ``ms.in()`` 
            the mechanism located in first segment of the currently accessed section. 

        :samp:`ms.in({x})` 
            the mechanism located in the segment containing x of the currently accessed section. 
            (Note that x=0 and 1 are considered to lie in the 
            0+ and 1- segments respectively so a proper iteration uses for(x, 0). 
            See :ref:`for <hoc_keyword_for>`.

        :samp:`ms.in({pointprocess})` 
            the point process object 

        :samp:`ms.in({mechanismstandard})` 
            another mechanism standard 

        If the source is not the same type as the standard then nothing happens. 

         

----



.. hoc:method:: MechanismStandard.out


    Syntax:
        ``ms.out()``

        ``ms.out(x)``

        ``ms.out(pointprocess)``

        ``ms.out(mechanismstandard)``


    Description:
        copies parameter values from this mechanism standard to ... 


        ``ms.out()`` 
            the mechanism located in the currently accessed section (all segments). 

        ``ms.out(x)`` 
            the mechanism located in the currently accessed section in the segment 
            containing x.(Note that x=0 and 1 are considered to lie in the 
            0+ and 1- segments respectively) 

        ``ms.out(pointprocess)`` 
            the point process argument 

        ``ms.out(mechanismstandard)`` 
            another mechanism standard 

        If the target is not the same type as the standard then nothing happens. 

         

----



.. hoc:method:: MechanismStandard.set


    Syntax:
        :samp:`ms.set("{varname}", {val} [, {arrayindex}])`


    Description:
        sets the parameter in the standard to *val*. If the variable is 
        an array, then the optional index can be specified. 

         

----



.. hoc:method:: MechanismStandard.get


    Syntax:
        ``val = ms.get("varname" [, arrayindex])``


    Description:
        returns the value of the parameter. If the variable is actually 
        a POINTER and it is nil, then return -1e300. 

         

----



.. hoc:method:: MechanismStandard.save


    Syntax:
        ``.save("name")``


    Description:
        For saving the state of a MechanismStandard to a session file. 
        The name will be the objectvar that the instance gets assigned to 
        when the session file is read. 
        See pointman.hoc for an example of usage. 

         

----



.. hoc:method:: MechanismStandard.count


    Syntax:
        ``cnt = ms.count()``


    Description:
        Returns the number of parameter names of the mechanism 
        represented by the MechanismStandard. 

         

----



.. hoc:method:: MechanismStandard.name


    Syntax:
        ``ms.name(strdef)``

        ``size = ms.name(strdef, i)``


    Description:
        The single arg form assigns the name of the mechanism to the strdef 
        variable. 
         
        When the i parameter is present (i ranges from 0 to ms.count()-1) the 
        strdef parameter gets assigned the ith name of the mechanism represented 
        by the MechanismStandard. In addition the return value is the 
        array size of that parameter (1 for a scalar). 


