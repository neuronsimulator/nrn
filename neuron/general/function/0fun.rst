.. _0fun:

0fun
----

         


.. function:: ivoc_style


    Syntax:
        ``ivoc_style("name", "attribute")``


    Description:
        Gives the style attribute to name. Any property listed in the file 
        :file:`$(NEURONHOME)/lib/nrn.defaults` or :file:`$(NEURONHOME)/src/ivoc/ivocmain.c`
        can be changed although it may be that a few of them have already been 
        used by the time the interpreter is invoked. 

    Example:

        .. code-block::
            none

            // 7 decimal places in value field editors. Must be done prior to any panel. 
            ivoc_style("*xvalue_format", "%.7g") 
            // large fonts in unix. Takes effect on next panel. 
            ivoc_style("*font", "*helvetica-medium-r-normal*--24*") 
            ivoc_style("*MenuBar*font", "*helvetica-medium-r-normal*--24*") 
            ivoc_style("*MenuItem*font", "*helvetica-medium-r-normal*--24*") 


         

----



.. function:: variable_domain


    Syntax:
        ``variable_domain(&variable, lower_limit, upper_limit)``

        ``variable_domain("varname", lower_limit, upper_limit)``


    Description:
        Define the domain limits for any variable for which one can take its address. 
        At this time only field editors check the variable domain. If one 
        attempts to enter a value into a field editor which is not in the domain, the 
        value will be set to the upper or lower limit. Important NEURON 
        variables such as dt, L, diam, and Ra have reasonable default domains. 
        Many parameters defined in model description files also have reasonable domains. 
         
        This function is most useful when a variable makes sense only as a 
        non-negative or positive number. 
         
        One can specify different domains only on a per name basis. Thus there 
        is only one domain specification for L and one for all the instances of 
        IClamp.amp but one can have a different specification 
        for IClamp.amp and VClamp.amp . 
         

----



.. function:: chdir


    Syntax:
        ``chdir("path")``


    Description:
        Change working directory to the indicated path. Returns 0 if successful 
        otherwise -1, ie the usual unix os shell return style. 

         

----



.. function:: getcwd


    Syntax:
        ``string = getcwd()``


    Description:
        Returns absolute path of current working directory in unix format. 
        The last character of the path name is '/'. 
        Must be less than 
        1000 characters long. 

         

----



.. function:: units


    Syntax:
        ``current_units = units(&variable)``

        ``current_units = units(&variable, "units string")``

        ``"on or off" = units(1 or 0)``

        ``current_units = units("varname", ["units string"])``


    Description:
        When units are on (default on) value editor buttons display the units 
        string (if it exists) along with the normal prompt string. Units for 
        L, diam, Ra, t, etc are built-in and units for membrane mechanism variables 
        are declared in the model description file. See modlunit . 
        Note that units are NOT saved in a session. Therefore, any user defined 
        variables must be given units before retrieving a session that shows them 
        in a panel. 
         
        The units display may be turned off with \ ``units(0)`` or by setting the 
        \ ``*units_on_flag: off`` in the nrn/lib/nrn.defaults file. 
         
        \ ``units(&variable)`` returns the units string for any 
        variable for which an address can be taken. 
         
        \ ``units(&variable, "units string")`` sets the units for the indicated 
        variable. 
         
        If the first arg is a string, it is treated as the name of the variable. 
        This is restricted to hoc variable names of the style, "name", or "classname.name". 
        Apart from the circumstance that the string arg style must be used when 
        executed from Python, a benefit is that it can be used when an instance 
        does not exist (no pointer to a variable of that type). 
        If there are no units specified for the variable name, or the variable 
        name is not defined, the return value is the empty string. 
         

    Example:

        .. code-block::
            none

            units(&t) // built in as "ms" 
            units("t") 
            units("ExpSyn.g") // built in as "uS" 
            x = 1 
            {units(&x, "mA/cm2")}	// declare units for variable x 
            units(&x)		// prints mA/cm2 
            proc p () { 
            	xpanel("Panel") 
            	xvalue("t") 
            	xvalue("prompt for x", "x", 1) 
            	xpanel() 
            } 
            p()		//shows units in panel 
            units(0) 	// turn off units 
            p()		// does not show units in panel 


    .. warning::
        In the Python world, the first arg must be a string as the pointer style will 
        raise an error. 

         

----



.. function:: execerror


    Syntax:
        ``execerror("message1", "message2")``


    Description:
        Raise an error and print the messages. 


