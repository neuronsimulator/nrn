.. _nfunc:

Miscellaneous Menus
-------------------

.. note::

    In Python, GUI menus require one to have previously run:

    .. code-block::
        python

        from neuron import gui
    
    The MATLAB interface does not currently support NEURON's InterViews-based GUI
    tools.

.. function:: nrnglobalmechmenu

    .. tab:: Python
    
    
        Syntax:
            ``n.nrnglobalmechmenu("mechname")``

            ``n = n.nrnglobalmechmenu("mechname", 0)``


        Description:
            pops up panel containing all global variables defined by the 
            mechanism. 
         
            :samp:`n.nrnglobalmechmenu("{mechname}", 0)` returns the number of global variables 
            for this mechanism. Does not pop up a panel. 
         


    .. tab:: HOC


        Syntax:
            ``nrnglobalmechmenu("mechname")``
        
        
            ``n = nrnglobalmechmenu("mechname", 0)``
        
        
        Description:
            pops up panel containing all global variables defined by the 
            mechanism. 
        
        
            :samp:`nrnglobalmechmenu("{mechname}", 0)` returns the number of global variables 
            for this mechanism. Does not pop up a panel. 
        
----



.. function:: nrnmechmenu

    .. tab:: Python
    
        vestigial. does nothing. taken over by :func:`nrnsecmenu` 

    .. tab:: HOC


        vestigial. does nothing. taken over by :func:`nrnsecmenu`
        
----



.. function:: nrnpointmenu

    .. tab:: Python
    
             
    
        Syntax:
            ``n.nrnpointmenu(PointProcessObject)``

            ``n.nrnpointmenu(PointProcessObject, labelstyle)``


        Description:
            Pops up panel containing all variables of the point process. 
            if the point process is relocated to another position then the 
            label will be incorrect and the window should be dismissed and 
            recreated. 
         
            If the second arg exists,  a value of -1 means to not prepend 
            a label. 0 means to use the Object name as the label. 1 (default) 
            means to use the object name along with location when the panel was 
            created. 

         

    .. tab:: HOC


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



.. function:: nrnsecmenu

    .. tab:: Python
    
    
        Syntax:
            ``n.nrnsecmenu(x, vartype, sec=section)``


        Description:
            Pop up a panel containing variables in the ``section``. 
         
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


    .. tab:: HOC


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
        
