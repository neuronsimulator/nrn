
.. _hoc_savstate:

SaveState
---------



.. hoc:class:: SaveState

    The state includes :hoc:data:`t`, the voltage for all segments of all sections,
    and all the STATEs defined in all the membrane and point process 
    mechanisms. With regard to model descriptions, it does not include 
    PARAMETERs, ASSIGNED variables. 
    It always includes 
    values for the ambiguous variable of ions such as 
    cai, ko, or ena. This can be an expensive object in terms of memory 
    storage. 
     
    The state also includes all the outstanding events (external and self) 
    and the weight vectors of all NetCon objects. For model descriptions 
    containing a NET_RECEIVE block, all the ASSIGNED variables are also included 
    in the state (this is because such models often use such variables to 
    store logic state and other values, such as the last event time t0, 
    needed to compute state variables at the next event.) 
     
    The outstanding event delivery times are absolute. 
    When restored, all outstanding 
    events will be cleared and the restored event times and NetCon info 
    will take their place. Note that it is not in general possible to 
    change the value of t in a network simulation since most NET_RECEIVE 
    blocks keep t0 (the last event time) as part of their state. 

    .. versionchanged:: 8.1

        Prior to NEURON 8.1, :class:`SaveState` did not save 
        reaction-diffusion states.

    .. warning::
        The intention is that a save followed by 
        any number of simulation-continue,restore 
        pairs will give the same simulation result (assuming the simulation 
        is deterministic). Given the possibility that simulations can 
        be written to depend on a variety of computer states not saved in this 
        object, this is more an experimental question than an assertion. 
         
        Between a save and a restore, 
        it is important not to create or delete sections, NetCon objects, 
        or point processes. Do not 
        change the number of segments, insert or delete mechanisms, 
        or change the location of point processes. 
         
        Does work with the local variable timestep method if the stdrun system 
        is used since continuerun() uses cvode.solve(tstop) to integrate and 
        this returns with all states at tstop. However, if you advance using 
        fadvance() calls different cells will be at different t values in 
        general and SaveState will be useless. 

    :hoc:class:`BBSaveState` is a more flexible cell centered version of SaveState

----



.. hoc:method:: SaveState.save


    Syntax:
        ``.save()``


    Description:
        t, voltage, state and event values are stored in the object. 

         

----



.. hoc:method:: SaveState.restore


    Syntax:
        ``.restore()``

        ``.restore(1)``


    Description:
        t, voltage, state  and event values are put back in the sections. 
        Between a save and a restore, 
        it is important not to create or delete sections, change 
        the number of segments, insert or delete mechanisms, 
        or change the location or number of point processes. 
        Before restoring states, the object checks for consistency 
        between its own data structure and the section structures. 
         
        If the arg is 1, then the event queue is not cleared and no saved events are 
        put back on the queue. Therefore any Vector.play and/or FInitializeHandler 
        events on the queue after finitialize() are not disturbed. 

         

----



.. hoc:method:: SaveState.fread


    Syntax:
        ``.fread(File)``

        ``.fread(File, close)``


    Description:
        Reads binary state data from a File object into the 
        SaveState object. (See File in ivochelp). This does 
        not change the state of the sections. (That is done with 
        \ ``.restore()``). This function opens the file defined 
        by the File object. On return the file is closed unless 
        the second arg exists and is 0. 
         
        Warning: file format depends on what 
        mechanisms are available in the executable and the order 
        that sections are created (and mechanisms inserted) 
        by the user. Also the order of NetCon, ArtificialCell, 
        PointProcess creation and just about everything else that 
        gets saved in the file. I.e. if you change your simulation 
        setup, old files may become incompatible. 
         
        In a parallel simulation, each host 
        :hoc:meth:`ParallelContext.id` , should
        write an id specific file. Note that the set of files is 
        at least :hoc:meth:`ParallelContext.nhost` specific.

         

----



.. hoc:method:: SaveState.fwrite


    Syntax:
        ``.fwrite(File)``


    Description:
        Opens the file defined by the *File* object, writes saved 
        binary state data to the beginning of the file. 
        On return the file is closed unless the second arg exists 
        and is 1. In that case, extra computer state information 
        may be written to the file, e.g. :hoc:meth:`Random.seq`.

         

----



.. hoc:method:: SaveState.writehoc


    Syntax:
        ``.writehoc(File)``


    Description:
        Writes saved state data as sequence of hoc statements that 
        can be read with \ ``xopen(...)``. Not implemented at this time. 


