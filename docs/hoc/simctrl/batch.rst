Running and Saving Batch Jobs
-----------------------------

.. hoc:function:: batch_run


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
        The :hoc:func:`batch_save` command specifies which variable are to be saved.
         

         
         

----



.. hoc:function:: batch_save


    Syntax:
        ``batch_save()``

        ``batch_save(&var, &var, ...)``


    Description:


        ``batch_save()`` 
            starts a new list of variables to save in a :hoc:func:`batch_run` .

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

     
