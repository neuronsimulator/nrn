.. _parnet:

ParallelNetManager
------------------



.. class:: ParallelNetManager


    Syntax:
        ``pnm = h.ParallelNetManager(ncell)``


    Description:
        Manages the setup and running of a network simulation on a cluster 
        of workstations or any parallel computer that has mpi installed. 
        A version for PVM is also available. This class, implemented 
        in :file:`nrn/share/lib/hoc/netparmpi.hoc`, presents an interface 
        compatible with the NetGUI style of network specification, and is implemented 
        using the :ref:`ParallelNetwork` methods. Those methods are 
        available only if NEURON has been built with the configuration option, 
        --with-mpi. The :file:`netparmpi.hoc` file at last count was only 430 lines long 
        so if you have questions about how it works that are not answered here, 
        take a look. 
         
        Experience with published network models where there is an order of magnitude 
        more segments or cells than machines, suggests that superlinear speedup 
        occurs up to about 20 to 50 machines due to the fact that the parallel 
        machine has much more effective high speed cache memory than a single 
        cpu. Basically, good performance will occur if there is a lot for each 
        machine to do and the amount of effort to simulate each machine's subnet 
        is about equal. If cell granularity causes load balance to be 
        a signficant problem see :meth:`ParallelNetManager.splitcell`. 
        The "lot for each machine to do" is relative to the 
        number of spikes that must be exchanged between machines and how often 
        these exchanges take place. The latter is determined by the minimum 
        delay between a spike event generated on one machine that must be delivered 
        to another machine since that defines the interval that each machine 
        is allowed to integrate before having to share all spikes it generated 
        which are destined for other machines. 
         
        The fundamental requirement for the use of this class is for the 
        programmer to be able to associate a unique global id (gid) for each 
        cell and define the connectivity by means of the source_cell_gid and the 
        target_cell_gid. If the target cell happens to have synapses, we assume 
        they can be found via a local synapse index into the target cell's synapse list. 
        We absolutely must use global indices because it 
        will be the case that when a connection is requested 
        on a machine that either the source or the target cell or both may not 
        actually exist on the machine -- the last case is a no-op. 
         
        The following describes the author's intention as to how this class can 
        be used to construct and simulate a parallel network. 
        It is assumed that every machine 
        executes exactly the same code (though with different data). 
         
        0)  So that the concatenation of all the following fragments will 
            end up being a valid network simulation for a ring of 128 artificial 
            cells where cell i sends a spike to cell i+1, let's start out with 

            .. code::

                from neuron import h
                h.load_file('stdrun.hoc')
                tstop = 1000

            Yes, I know that this example is foolish since there is no computation 
            going on except when a cell receives a spike. I don't expect any benefit 
            from parallelization but it is simple enough to allow me to focus on the process 
            of setup and run instead of cluttering the example with a large cell class. 
         
        1)  load the :file:`netparmpi.hoc` file and create a ParallelNetManager 

            .. code::

                h.load_file("netparmpi.hoc") 
                ncell = 128 
                pnm = h.ParallelNetManager(ncell) 

            If you know the global number of cells put it in. For the non-MPI 
            implementation of ParallelNetManager, ncell is absolutely necessary 
            since that implementation constructs many mapping vectors that allow 
            it to figure out what cell is being talked about when the gid is 
            known. The MPI implementation uses dynamically constructed maps and 
            it is not necessary to know the global number of cells at this time. 
            Note that ncell refers to the global number of cells and NOT the 
            number of cells to be created on this machine. 
         
        2)  Tell the system which gid's are on which machines. 
            The simplest distribution mechanism is :func:`round_robin` 

            .. code::

                pnm.round_robin() 

            which will certainly give good load balance if the number of each 
            cell type to be constructed is an integer multiple of the number 
            of machines. Otherwise specify which gid's are on which machines through 
            the use of :meth:`ParallelNetManager.set_gid2node` . Note that you only 
            HAVE to call \ ``pnm.set_gid2node(gid, myid)`` for the subset of gid's that 
            are supposed to be associated with this machines 
            particular \ ``myid = pnm.pc.id`` but it is usually simpler just to call 
            it for all gid's since the ``set_gid2node`` call is a no-op when the second 
            argument does not match the pc.id. Also, the PVM version REQUIRES that 
            you call the function for all the gid values. 
             
            There are three performance considerations with regard to sprinkling gid 
            values on machines. 
         
            A)  By far the most important is load balance. That is 
                simple if all your cells take the same time to integrate over the same 
                interval. If cells have very different sizes or cpu's end up with 
                very different amounts of work to do so that load balance is a 
                serious problem then :meth:`ParallelNetManager.splitcell` can be used to 
                solve it. 
         
            B)  Of lesser importance but still quite important is to maximize the 
                delay of NetCon's that span machines. This isn't an issue if all your 
                NetCon delays are the same.  The minimum delay across machines defines 
                the maximum step size that each machine can integrate before having 
                to share spikes. In principle, Metis can help with this and C) but don't 
                waste your time unless you have established that communication overhead 
                is your rate limiting step. See :meth:`ParallelNetManager.prstat` and 
                :meth:`ParallelContext.wait_time` . 
             
            C)  I am only guessing that this is less important than B, it is certainly 
                related, but obviously 
                things will be better if you minimize the number of spanning NetCon's. 
                For our ring example it obviously would be best to keep neighboring cells together 
                but the improvement may be too small to measure. 
         
        3)  Now create only the cells that are supposed to be on this machine 
            using :meth:`ParallelNetManager.register_cell`. 

            .. code::

                for i in range(ncell):
                    if pnm.gid_exists(i):
                        pnm.register_cell(i, h.IntFire1())

            Notice how we don't construct a cell if the gid does not exist. 
            You only HAVE to call 
            register_cell for those gid's which are actually owned by this machine and 
            need to send spikes to other machines. 
            If the gid does not exist, then register_cell will call gid_exists for you. 
            Note that 2) and 3) can 
            be combined but it is a serious bug if a gid exists on more than one machine. 
            You can even start connecting 
            as discussed in item 4) but of course a NetCon presupposes the existence 
            of whatever cells it needs on this machine. 
             
            Of course this presupposes that you have 
            already read the files that define your cell classes. 
            We assume your 
            cell classes for "real" cells follow the NetworkReadyCell policy required by 
            the NetGUI tool. That is, each "real" cell type has a synapse list, eg. the 
            first synapse is \ ``cell.synlist.object(0)`` (the programmer will have to 
            make use of those synapse indices when such cells are the target of a NetCon) 
            and each "real" cell type has a connect2target method that constructs 
            a netcon (returns it in the second argument) 
            with that cell as the source and its first argument as the 
            synapse or artificial cell object. 
             
            Artificial cells can either be unwrapped or follow the NetGUI tool policy 
            where they are wrapped in a cell class in which the actual artificial cell 
            is given by the \ ``cell.pp`` field and the cell class also has a 
            connect2target method. 
             
            If you don't know what I've been talking about in the last two paragraphs, 
            use the NetGUI tool on a single machine to construct a toy network consisting 
            of a few real and artificial cells and save it to a hoc file for examination. 
         
        4)  Connect the cells using :meth:`ParallelNetManager.nc_append` 

            .. code::

                for i in range(ncell):
                    pnm.nc_append(i, (i + 1) % ncell, -1, 1.1, 2) 

            Again, it only has to be called if i, or i + 1, or both, are on this machine. 
            It is a no-op if neither are on this machine and usually a no-op if only 
            the source is on this machine since it will only mark the source cell 
            as output cell, once. 
             
            The -1 just refers to the 
            synapse index which should be -1 for artificial cells. 
            The delay is 2 ms and the weight is 1.1 which guarantees 
            that the IntFire1 cell will fire when it receives a spike. 
             
            Our example requires a stimulus and this is not an 
            unreasonable time to stimulate the net. 
            Let's get the ring going by forcing the gid==4 
            cell to fire. 

            .. code::

                # stimulate
                if pnm.gid_exists(4):
                    stim = h.NetStim(0.5)
                    ncstim = h.NetCon(stim, pnm.pc.gid2obj(4)) 
                    ncstim.weight[0] = 1.1 
                    ncstim.delay = 0 
                    stim.number=1 
                    stim.start=1 

            Note the stimulator does not require a gid even though it is an artificial 
            cell because its connections do not span machines. But it does have to be 
            on the machine that has the cell it is connecting to. 
         
        5)  Have the system figure out the minimum spanning NetCon delay so it knows 
            the maximum step size. 

            .. code::

                pnm.set_maxstep(100) # will end up being 2 

         
        6)  Decide what output to collect 

            .. code::

                pnm.want_all_spikes() 

            If you want to record spikes from only a few cells you can use 
            :meth:`ParallelNetManager.spike_record` explicitly. If you want to 
            record range variable trajectories, check that the cell exists with 
            :meth:`ParallelNetManager.gid_exists` and then use :meth:`Vector.record`. 
             
        7)  Initialize and run. 

            .. code::

                import time
                h.stdinit() 
                runtime = time.time() 
                pnm.psolve(tstop) 
                runtime = time.time() - runtime 

         
        8)  Print the results. 

            .. code::

                for spike, i in zip(pnm.spikevec, pnm.idvec):
                    print(spike, i)

            If you save the stdout to a file you can sort the results. A nice idiom 
            is 
            \ ``sort -k 1n,1n -k 2n,2n temp1 > temp`` 
             
            A perhaps more flexible alternative is to separate the master from all the 
            workers somewhere after item 4) and before item 8) using :meth:`ParallelContext.runworker` 
            and then making use of the :meth:`ParallelNetManager.prun` and 
            :meth:`ParallelNetManager.gatherspikes` with the normal ParallelContext control 
            in a master worker framework. 
             
            At any rate, before we quit we have to call it so that the master can 
            tell all the workers to quit. 

            .. code-block::

                pnm.pc.runworker()
                pnm.pc.done()


         

----



.. method:: ParallelNetManager.set_gid2node


    Syntax:
        ``pnm.set_gid2node(gid, machine_id)``


    Description:
        When MPI is being used, this is just 
        a wrapper for the ParallelContext version of 
        :meth:`ParallelContext.set_gid2node` . 
         

         

----



.. method:: ParallelNetManager.round_robin


    Syntax:
        ``pnm.round_robin()``


    Description:
        The gid ranging from 0 to ncell-1 
        is assigned to machine ``(gid + 1) % nhost``. There is no good reason 
        anymore for the "+1". :meth:`ParallelContext.nhost` is the number of machines 
        available. 

         

----



.. method:: ParallelNetManager.gid_exists


    Syntax:
        ``result = pnm.gid_exists(gid)``


    Description:
        Returns 1 if the gid exists on this machine, 2 if it exists and has been 
        declared to be an output cell. 0 otherwise. 
        Just a wrapper for :meth:`ParallelContext.gid_exists` when MPI is being used. 

         

----



.. method:: ParallelNetManager.create_cell


    Syntax:
        ``cellobject = pnm.create_cell(gid, "obexpr")``


    Description:
        This is deprecated. Use :meth:`ParallelNetManager.register_cell` . 
         
        If the gid exists on this machine the obexpr is executed in HOC in a statement 
        equivalent to ``pnm.cells.append(obexpr)``. Obexpr should be something like 
        \ ``"new Pyramid()"`` or any function that returns a cell object. Valid 
        "real" cell objects should have a connect2target method and a synlist 
        synapse list field just as the types used by the NetGUI builder. 
        Artificial cell objects can be bare or enclosed in a wrapper class 
        using the pp field. 
         
        Note: the following has been changed so that the source is always 
        an outputcell. 
         
        At the end of this call, \ ``pnm.gid_exists(gid)`` will return either 
        0 or 1 because the cell has not yet been declared to be an outputcell. 
        That will be done when the first connection is requested for which 
        this cell is a source but the target is on another machine. 

         

----



.. method:: ParallelNetManager.register_cell


    Syntax:
        ``pnm.register_cell(gid, cellobject)``


    Description:
        Associate gid and cellobject. If :meth:`ParallelContext.gid_exists` 
        is zero then this procedure calls :meth:`ParallelContext.set_gid2node` 
        If the cell is "real" or encapsulates a point process artificial cell, then 
        the cellobject.connect2target is called. The cellobject is declared to 
        be an :meth:`ParallelContext.outputcell` . 
         
        This method supersedes the create_cell method since it more easily handles 
        cell creation arguments. 

         

----



.. method:: ParallelNetManager.nc_append


    Syntax:
        ``netcon = pnm.nc_append(src_gid, target_gid, synapse_id, weight, delay)``


    Description:
        If the source and target exist on this machine a NetCon is created 
        and added to the pnm.nclist. 
         
        If the target exists and is a real cell 
        the synapse object is \ ``pnm.gid2obj(target_gid).synlist(synapse_id)``. 
         
        If the target exists and is a wrapped artificial cell then the 
        synapse_id should be -1 and the target artificial cell is 
        \ ``pnm.gid2obj(target_gid).pp``. 
        If the target exists and is an ArtificialCell 
        the synapse_id should be -1 and the target artificial cell is 
        \ ``pnm.gid2obj(target_gid)``. Note that 
        the target is an unwrapped artificial cell if 
        :meth:`StringFunctions.is_point_process` returns a non-zero value. 
         
        If the target exists but not the source, the netcon 
        is created via :meth:`ParallelContext.gid_connect` and added to the 
        pnm.nclist. 
         
        If the source exists but not the target, and 
        :meth:`ParallelContext.gid_exists` returns 
        1 (instead of 2) then the cell is marked to be an 
        :meth:`ParallelContext.outputcell` . 
         
        If the source exists and is a real cell or wrapped artificial 
        cell \ ``pnm.gid2obj(src_id).connect2target(synapse_target_object, nc)`` 
        is used to 
        create the NetCon. 
         
        If the source exists and is a artificial cell 
        then the NetCon is created directly. 
         
        If neither the source or target exists, 
        there is nothing to do. 

         

----



.. method:: ParallelNetManager.want_all_spikes


    Syntax:
        ``pnm.want_all_spikes()``


    Description:
        Records all spikes of all cells on this machine into the 
        pnm.spikevec and pnm.idvec Vector objects. The spikevec holds spike times 
        and the idvec holds the corresponding gid values. 

         

----



.. method:: ParallelNetManager.spike_record


    Syntax:
        ``pnm.spike_record(gid)``


    Description:
        Wraps :meth:`ParallelContext.spike_record` but calls it only if 
        :meth:`ParallelContext.gid_exists` is nonzero and records the spikes 
        into the pnm.spikevec and pnm.gidvec Vector objects. 

         
         

----



.. method:: ParallelNetManager.prun


    Syntax:
        ``pnm.prun()``


    Description:
        All the workers and the master are asked to :meth:`ParallelNetManager.pinit` 
        and :meth:`ParallelNetManager.pcontinue` up to tstop. 

         

----



.. method:: ParallelNetManager.psolve


    Syntax:
        ``pnm.psolve(tstop)``


    Description:
        Wraps :meth:`ParallelContext.psolve` . 

         

----



.. method:: ParallelNetManager.pinit


    Syntax:
        ``pnm.pinit()``


    Description:
        All the workers and the master execute a call to 
        :meth:`ParallelContext.set_maxstep` to determine the maximum possible step size 
        and all the workers and the master execute a call to 
        the stdinit() of the 
        standard run system. 

         

----



.. method:: ParallelNetManager.pcontinue


    Syntax:
        ``pnm.pcontinue(tstop)``


    Description:
        All the workers and the master execute a call to :meth:`ParallelContext.psolve` 
        to integrate from the current value of t to the argument value. 

         

----



.. method:: ParallelNetManager.prstat


    Syntax:
        ``pnm.prstat(0)``

        ``pnm.prstat(1)``


    Description:
        Prints a high resolution amount of time all the machines have waited for 
        spike exchange. If some are much higher than others then there is likely 
        a load balance problem. If they are all high relative to the simulation 
        time then spike exchange may be the rate limiting step. 
         
        If the argument is 1, then, in addition to wait time, spike_statistics 
        are printed. The format is 

        .. code-block::
            none

            pc.id wait_time(s) nsendmax nsend nrecv nrecv_useful 
            %d\t  %g\t %d\t %d\t %d\t %d\n 


    .. seealso::
        :meth:`ParallelContext.wait_time`, :meth:`ParallelContext.spike_statistics`

         

----



.. method:: ParallelNetManager.gatherspikes


    Syntax:
        ``pnm.gatherspikes``


    Description:
        All the workers are asked to post their spikevec and idvec Vectors 
        for taking by the master and concatenated to the master's spikevec 
        and idvec Vectors. 

         

----



.. method:: ParallelNetManager.splitcell


    Syntax:
        ``pnm.splitcell(hostcas, hostparent, sec=split_at)``


    Description:
        The cell is split at the section ``split_at`` and that section's 
        parent into two subtrees rooted at the old connection end of ``split_at``
        and the old ``split_at`` connecting point of the parent (latter must be 
        0 or 1). The ``split_at`` subtree will be preserved on the host specified 
        by hostcas and the parent subtree will be destroyed. The parent subtree 
        will be preserved on the host specified by hostparent and the ``split_at`` 
        subtree destroyed. Hostparent must be either ``host_split_at+1`` or ``host_split_at-1``. 
         
        Splitcell works only if NEURON has been configured with the 
        --with-paranrn option. A split cell has exactly the same stability 
        and accuracy properties as if it were on a single machine. Splitcell 
        cannot be used with variable step methods at this time. A cell can 
        be split into only two pieces. 
         
        Splitcell is implemented using the :meth:`ParallelContext.splitcell` method 
        of :class:`ParallelContext`. 

