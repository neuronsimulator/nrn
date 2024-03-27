Running and Saving Batch Jobs
-----------------------------

.. function:: batch_run


    Syntax:
    
        .. code-block::
            python
            
            h.batch_run(tstop, tstep, 'filename')
            h.batch_run(tstop, tstep, 'filename', 'comment')


    Description:
        This command replaces the set of commands: 

        .. code-block::
            python
            
            while h.t < tstop:
                for i in range(int(tstep / dt)):
                    h.fadvance()
                # print results to filename

        and produces the most efficient run on any given neuron model.  This 
        command was created specifically for Cray computers in order eliminate 
        the interpreter overhead as the rate limiting simulation step. 
         
        This command will save selected variables, as they are changed in the run, 
        into a file whose name is given as the third argument. 
        The 4th comment argument is placed at the beginning of the file. 
        The :func:`batch_save` command specifies which variable are to be saved. 
        
        The variables are stored in plain text and separated by spaces (see the example
        output below).

    Example:
    
        The following code creates a single compartment neuron, adds Hodgkin-Huxley
        channels, applies a current clamp, runs, and stores a voltage trace.
    
        .. code-block::
            python
                     
            from neuron import h

            # define a geometry
            soma = h.Section(name="soma")
            soma.L = 10
            soma.diam = 10

            # biophysics: Hodgkin-Huxley channels
            soma.insert(h.hh)

            # add a stimulus
            iclamp = h.IClamp(soma(0.5))
            iclamp.dur = 0.2
            iclamp.delay = 0.3
            iclamp.amp = 0.5

            # define variables to be stored (time and the soma's membrane potential)
            h.batch_save()
            h.batch_save(h._ref_t, soma(0.5)._ref_v)

            # initialize, run, and save
            h.finitialize(-65)
            h.batch_run(2, 0.1, 'hhsim.dat', 'My HH sim')

        The output (the time series of an action potential) is stored in the :file:`hhsim.dat`:
         
        .. code-block::
            none

            My HH sim
            batch_run from t = 0 to 2 in steps of 0.1 with dt = 0.025
             0 -65
             0.1 -64.9971
             0.2 -64.9943
             0.3 -64.9917
             0.4 -49.6876
             0.5 -34.9008
             0.6 -33.426
             0.7 -25.5015
             0.8 -7.00019
             0.9 20.989
             1 38.2226
             1.1 40.9284
             1.2 39.1047
             1.3 35.8921
             1.4 31.8901
             1.5 27.3462
             1.6 22.4496
             1.7 17.3559
             1.8 12.1873
             1.9 7.0331
             2 1.9538
            
    .. seealso::
    
        :meth:`Vector.record`

----



.. function:: batch_save


    Syntax:
    
        .. code-block::
            python
            
            h.batch_save()
            h.batch_save(varref1, varref2, ...)


    Description:


        ``h.batch_save()`` 
            starts a new list of variables to save in a :func:`batch_run` . 

        ``h.batch_save(varref1, varref2, ...)`` 
            adds pointers to the list of variables to be saved in a ``batch_run``. 
         

    Example:

        .. code-block::
            python

            h.batch_save()    # This clears whatever list existed and starts a new 
            		          # list of variables to be saved. 
            h.batch_save(soma(0.5)._ref_v, axon(1)._ref_v)
            for i in range(3):
                h.batch_save(dend[i](0.3)._ref_v)

        specifies five quantities to be saved from each :func:`batch_run`. 

     
