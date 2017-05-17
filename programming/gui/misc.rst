Style Settings
--------------

.. function:: ivoc_style

    Syntax:
        ``h.ivoc_style("name", "attribute")``

    Description:
        Gives the style attribute to name. Any property listed in the file 
        :file:`$(NEURONHOME)/lib/nrn.defaults` or :file:`$(NEURONHOME)/src/ivoc/ivocmain.c`
        can be changed although it may be that a few of them have already been 
        used by the time the interpreter is invoked. 

    Example:

        .. code-block::
            python

            # 7 decimal places in value field editors. Must be done prior to any panel. 
            h.ivoc_style("*xvalue_format", "%.7g") 
            # large fonts in unix. Takes effect on next panel. 
            h.ivoc_style("*font", "*helvetica-medium-r-normal*--24*") 
            h.ivoc_style("*MenuBar*font", "*helvetica-medium-r-normal*--24*") 
            h.ivoc_style("*MenuItem*font", "*helvetica-medium-r-normal*--24*") 


----

Domain Restrictions for Fields
------------------------------

.. function:: variable_domain

    Syntax:
        ``h.variable_domain("varname", lower_limit, upper_limit)``

    Description:
        Define the domain limits for NEURON variables. Important NEURON 
        variables such as dt, L, diam, and Ra have reasonable default domains. 
        Many parameters defined in model description files also have reasonable domains. 
         
        This function is most useful when a variable makes sense only as a 
        non-negative or positive number. 
         
        One can specify different domains only on a per name basis. Thus there 
        is only one domain specification for L and one for all the instances of 
        IClamp.amp but one can have a different specification 
        for IClamp.amp and VClamp.amp.

    .. note::

        The HOC version also supports a pointer-based specification, but that form does
        not work from Python.

    .. note::

        ``varname`` here would be things like ``t`` or ``diam`` not ``h.t`` or ``h.diam``; i.e.
        omit the ``h.`` prefix.

----

PrintWindowManager Placement
----------------------------

.. function:: pwman_place

    Syntax:
        ``h.pwman_place(left, top)``

        ``h.pwman_place(left, top, 0)``

    Description:
        moves the PrintWindowManager to the indicated location in pixel 
        coordinates where 0,0 is the top left corner of the screen. 
        It is intended that if you build an interface by placing windows 
        near the top and build right then the session you save will 
        be portable to other window managers and other systems with 
        different screen sizes. 
         
        If the third argument is 0, then the window is placed but hidden. 


    .. seealso::
    
        :class:`PWManager`


