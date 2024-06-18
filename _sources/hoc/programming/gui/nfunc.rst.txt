
.. _hoc_nfunc:

Miscellaneous Menus
-------------------



.. hoc:function:: nrnglobalmechmenu


    Syntax:
        ``nrnglobalmechmenu("mechname")``

        ``n = nrnglobalmechmenu("mechname", 0)``


    Description:
        pops up panel containing all global variables defined by the 
        mechanism. 
         
        :samp:`nrnglobalmechmenu("{mechname}", 0)` returns the number of global variables 
        for this mechanism. Does not pop up a panel. 
         


----



.. hoc:function:: nrnmechmenu

        vestigial. does nothing. taken over by :hoc:func:`nrnsecmenu`

----



.. hoc:function:: nrnpointmenu

         

    Syntax:
        ``nrnpointmenu(PointProcessObject)``

        ``nrnpointmenu(PointProcessObject, labelstyle)``


    Description:
        Pops up panel containing all variables of the point process. 
        if the point process is relocated to another position then the 
        label will be incorrect and the window should be dismissed and 
        recreated. 
         
        If the second arg exists,  a value of -1 means to not prepend 
        a label. 0 means to use the Object name as the label. 1 (default) 
        means to use the object name along with location when the panel was 
        created. 

         

----



.. hoc:function:: nrnsecmenu


    Syntax:
        ``nrnsecmenu(x, vartype)``


    Description:
        Pop up a panel containing variables in the currently accessed section. 
         
        0 < x < 1 shows variables at segment containing x 
        changing these variables changes only the values in that segment 
        eg. equivalent to :samp:`section.v(.2) = -65`
         
        x = -1 shows range variables which are constant (same in 
        each segment  of the section). 
        changing these variables makes the variable constant in the 
        entire section. eg. equivalent to section.v = -65. 
        Variables that are not constant get a label to that effect 
        instead of a field editor. 
         
        vartype=1,2,3 shows parameters, assigned, or states respectively. 


