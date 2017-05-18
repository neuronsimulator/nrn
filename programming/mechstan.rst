.. _mechstan:

         
MechanismStandard (Parameter Control)
-------------------------------------



.. class:: MechanismStandard


    Syntax:
    
        .. code-block::
            python
            
            ms = h.MechanismStandard(name_str)
            ms = h.MechanismStandard(name_str, vartype)


    Description:
        With no vartype or vartype = 1, this provides 
        storage for parameter values of a membrane mechanism or point process. 
        This class is useful in maintaining a default set of parameters and can 
        be used to specify values for a set of sections. 
         
        *name_str* is a density mechanism such as ``hh`` or a point process 
        such as :class:`VClamp`. A ``MechanismStandard`` instance, when created, 
        contains default values for all parameters associated with the mechanism. 
         
        In combination with the 
        :class:`MechanismType` class it is possible to create generic graphical interface 
        widgets that are independent of the particular mechanism and parameter names. 
         
        If vartype = 1, 2, or 3, the storage is for PARAMETER, ASSIGNED, or STATE 
        variables respectively. If vartype = 0, the storage is for all three types. 
         
        If vartype = -1, the count and names (and array size) 
        of the GLOBAL variables are accessible, but any other method will 
        generate an error message. 
         

    Example:

        .. code-block::
            python
            
            from neuron import h, gui
            ms1 = h.MechanismStandard('hh')
            ms2 = h.MechanismStandard('AlphaSynapse')
            ms2.set('gmax', 0.3)
            ms1.panel()
            ms2.panel()

            objref ms1, ms2 
            ms1 = new MechanismStandard("hh") 
            ms2 = new MechanismStandard("AlphaSynapse") 
            ms2.set("gmax", .3) 
            ms1.panel() 
            ms2.panel() 

        .. image:: ../images/mechanismstandard.png
            :align: center
                     
        The following example prints all the names associated with POINT_PROCESS 
        and SUFFIX mechanisms. 

        .. code-block::
            python

            from neuron import h, gui

            soma = h.Section()
            def pname(msname):
                s = h.ref('')
                for i in xrange(-1, 4):
                    ms = h.MechanismStandard(msname, i)
                    print '\n', msname, '  vartype=%d' % i
                    for j in xrange(int(ms.count())):
                        k = ms.name(s, j)
                        print '%-5d %-20s size=%d' % (j, s[0], k)

            def ptype():
                msname = h.ref('')
                for i in xrange(2):
                    mt = h.MechanismType(i)
                    for j in xrange(int(mt.count())):
                        mt.select(j)
                        mt.selected(msname)
                        print '\n\n', msname[0], ' mechanismtype=%d' % j
                        pname(msname[0])


            ptype() 
             


    .. seealso::
        :class:`MechanismType`

         

----



.. method:: MechanismStandard.panel


    Syntax:
        .. code-block::
            python
            
            ms.panel()
            ms.panel("string")


    Description:
        Popup a panel of parameters for this mechanism. It's a good idea to 
        set the default values before generating the panel. 
         
        With no argument the first item in the panel will be the name of the 
        mechanism. Otherwise the string is used as the first item label. 

    .. seealso::
        :func:`nrnglobalmechmenu`, :func:`nrnmechmenu`, :func:`nrnpointmenu`

         

----



.. method:: MechanismStandard.action


    Syntax:
        .. code-block::
            python
            
            ms.action("hoc_command")


    Description:
        action to be executed when any variable is changed in the panel. 
        The hoc variable :data:`hoc_ac_` is set to the index of the variable (0 to count-1). 

    .. Warning::
       
        Currently only takes a HOC string and does not work with a Python callable.

..    Example:
..        forall delete_section() 

        .. code-block
..            none

..            create soma, axon, dend[3] 
..            forsec "a" insert hh 
..            forsec "d" insert pas 
..            xpanel("Updated when MechanismStandard is changed") 
..            xvalue("dend[0].g_pas") 
..            xvalue("dend[1].g_pas") 
..            xvalue("dend[2].g_pas") 
..            xpanel() 
..            objref ms 
..            ms = new MechanismStandard("pas") 
..            ms.action("change_pas()") 
..            ms.panel() 
             
..            proc change_pas() { 
..            	forall if(ismembrane("pas")) { 
..           		ms.out() 
..            	} 
..            } 


         

----



.. method:: MechanismStandard.in


    Syntax:
        .. code-block::
            python
            
            ms.__getattribute__('in')()
            ms.__getattribute__('in')(x)
            ms.__getattribute__('in')(pointprocess)
            ms.__getattribute__('in')(mechanismstandard)

    Description:
        copies parameter values into this mechanism standard from ... 


        ``ms.__getattribute__('in')()`` 
            the mechanism located in first segment of the currently accessed section. 

        :samp:`ms.__getattribute__('in')({x})` 
            the mechanism located in the segment containing x of the currently accessed section. 
            (Note that x=0 and 1 are considered to lie in the 
            0+ and 1- segments respectively so a proper iteration uses for(x, 0). 
            See :ref:`for <keyword_for>`.

        :samp:`ms.__getattribute__('in')({pointprocess})` 
            the point process object 

        :samp:`ms.__getattribute__('in')({mechanismstandard})` 
            another mechanism standard 

        If the source is not the same type as the standard then nothing happens. 

    .. note::

        In HOC, this function is available via ``ms.in``; unfortunately in Python that is a Syntax Error
        as ``in`` is a Python keyword.

         

----



.. method:: MechanismStandard.out


    Syntax:
        .. code-block::
            python
            
            ms.out()
            ms.out(x)
            ms.out(pointprocess)
            ms.out(mechanismstandard)


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



.. method:: MechanismStandard.set


    Syntax:
        .. code-block::
            python
            
            ms.set('varname', val [, arrayindex])


    Description:
        sets the parameter in the standard to *val*. If the variable is 
        an array, then the optional index can be specified. 

         

----



.. method:: MechanismStandard.get


    Syntax:
        .. code-block::
            python
            
            val = ms.get('varname' [, arrayindex])


    Description:
        returns the value of the parameter. If the variable is actually 
        a POINTER and it is nil, then return -1e300. 

         

----



.. method:: MechanismStandard.save


    Syntax:
        .. code-block::
            python
            
            ms.save('name')


    Description:
        For saving the state of a MechanismStandard to a session file. 
        The name will be the objectvar that the instance gets assigned to 
        when the session file is read. 
        See pointman.hoc for an example of usage. 

         

----



.. method:: MechanismStandard.count


    Syntax:
        .. code-block::
            python
            
            cnt = ms.count()


    Description:
        Returns the number of parameter names of the mechanism 
        represented by the MechanismStandard. 

         

----



.. method:: MechanismStandard.name


    Syntax:
        .. code-block::
            python
            
            ms.name(strref)
            size = ms.name(strref, i)


    Description:
        The single arg form assigns the name of the mechanism to the strref 
        variable. 
         
        When the i parameter is present (i ranges from 0 to ms.count()-1) the 
        strref parameter gets assigned the ith name of the mechanism represented 
        by the MechanismStandard. In addition the return value is the 
        array size of that parameter (1 for a scalar). 


    Example:
    
        .. code-block::
            python
            
            from neuron import h, gui

            ms = h.MechanismStandard('hh')
            name_strref = h.ref('')

            # read the name of the mechanism
            ms.name(name_strref)

            print name_strref[0]    # displays: hh

