.. _parcon:

ParallelContext
---------------

A video tutorial of parallelization in NEURON from the 2021 NEURON
summer webinar series is available :ref:`here<parallel-neuron-sims-2021-07-13>`.

.. toctree::
    :hidden:

    lyttonmpi.rst


.. class:: ParallelContext


    Syntax:
        ``pc = h.ParallelContext()``

        ``pc = h.ParallelContext(nhost)``


    Description:
        "Embarrassingly" parallel computations using a Bulletin board style 
        analogous to LINDA. (But see the :ref:`ParallelNetwork`, 
        :class:`ParallelNetManager` and :ref:`ParallelTransfer` discussions. 
        Also see :ref:`SubWorld` for a way to simultaneously use 
        the bulletin board and network simulations involving global identifiers.) 
        Useful when doing weeks or months worth of 
        simulation runs each taking more than a second and where not much 
        communication is required.  Eg.  parameter sensitivity, and some forms 
        of optimization.  The underlying strategy is to keep all machines in a 
        PVM or :ref:`ParallelContext_MPI` 
        virtual machine (eg.  workstation cluster) as busy as possible by 
        distinguishing between hosts (cpu's) and tasks.  A task started by a 
        host stays on that host til it finishes.  The code that a host is 
        executing may submit other tasks and while waiting for them to finish 
        that host may start other tasks, perhaps one it is waiting for. 
        Early tasks tend to get done first 
        through the use of a tree shaped priority scheme.  We try to set things 
        up so that any cpu can execute any task.  The performance is good when 
        there are always tasks to perform.  In this case, cpu's never are 
        waiting for other cpu's to finish results but constantly take a task 
        from the bulletin board and put the result back onto the bulletin board. 
        Communication overhead is not bad if each task takes a second or more. 
         
        When using the Bulletin board with Python, the methods 
        :meth:`submit`, :meth:`context`, :meth:`pack`, and :meth:`post` 
        have been augmented and :meth:`pyret` and :meth:`upkpyobj` have been introduced 
        to allow a more Pythonic style. I.e. The executable 
        string for submit and context may be replaced by a Python callable that 
        returns a Python Object (retrieved with pyret), the args to submit, context, pack, and post 
        may be Python Objects, and a bulletin board message value which is a Python 
        Object may be retrieved with upkpyobj. At the end of the 
        following hoc parallelization and discussion the same example is 
        repeated as a Python parallelization. The only restriction is that any 
        python object arguments or return values must be pickleable (see 
        http://docs.python.org/library/pickle.html. As of this writing, hoc 
        objects are not pickleable.) 
         
        The simplest form of parallelization of a loop 
        from the users point of view is 

        .. code-block::
            python

            from neuron import h
            
	    # importing MPI or h.nrnmpi_init() must come before the first instantiation of ParallelContext()
	    h.nrnmpi_init()
                        
            pc = h.ParallelContext()

            def f(x):
                """a function with no context that changes except its argument"""
                return x * x

            pc.runworker() # master returns immediately, workers in an
                           # infinite loop running jobs from bulletin board

            s = 0
            if pc.nhost() == 1:          # use the serial form
                for i in range(20):
                    s += f(i)
            else:                        # use the bulleting board form
                for i in range(20):      # scatter processes
                    pc.submit(f, i)      # any context needed by f had better be
                                         # the same on all hosts
                while pc.working():      # gather results
                    s += pc.pyret()      # the return value for the executed function

            print(s)
            pc.done()                    # tell workers to quit

         
        Several things need to be highlighted: 
         
        If a given task submits other tasks, only those child tasks 
        will be gathered by the working loop for that given task. 
        At this time the system groups tasks according to the parent task 
        and the pc instance is not used. See :meth:`ParallelContext.submit` for 
        further discussion of this limitation. The safe strategy is always to 
        use the idiom: 

        .. code-block::
            none

            for i in range(n):
                pc.submit(...)          # scatter a set of tasks 
            while pc.working():         # gather them all 

         
        Earlier submitted tasks tend to complete before later submitted tasks, even 
        if they submit tasks themselves. Ie, A submitted 
        task has the same general priority as the parent task 
        and the specific priority of tasks with the same parent 
        is in submission order. 
        A free cpu always works on the 
        next unexecuted task with highest priority. 
         
        Each task manages a separate group of submissions 
        whose results are returned only to that task. Therefore you can 
        submit tasks which themselves submit tasks. 
         
        The ``pc.working()`` call checks to see if a result is ready. If so it returns 
        the unique system generated task id (a positive integer) 
        and the return value of the task 
        function is accessed via 
        the ``pc.pyret()`` function. The arguments to the function executed by the 
        submit call are also available. If all submissions have been computed and all 
        results have been returned, ``pc.working()`` returns 0. If results are 
        pending, working executes tasks from ANY ParallelContext until a 
        result is ready. This last feature keeps cpus busy but places stringent 
        requirements on how the user changes global context without 
        introducing bugs. See the discussion in :meth:`ParallelContext.working` . 
         
        ParallelContext.working may not return results in the order of 
        submission. 
         
        Python code subsequent to pc.runworker() is executed only by the 
        master since that call returns immediately if the process is 
        the master and otherwise starts an infinite loop on each worker 
        which requests and executes submit tasks from ANY ParallelContext 
        instance. This is the standard way to seed the bulletin board with 
        submissions. Note that workers may also execute tasks that themselves 
        cause submissions. If subsidiary tasks call ``pc.runworker()``, the call 
        returns immediately. Otherwise the task 
        it is working on would never complete! 
        The pc.runworker() function is also called for each worker after all code files 
        are read in and executed. 
         
        The basic organization of a simulation is: 

        .. code-block::
            python

            # setup which is exactly the same on every machine. 
            # ie declaration of all functions, procedures, setup of neurons 
             
            pc.runworker() # to start the execute loop if this machine is a worker 
             
            # the master scatters tasks onto the bulletin board and gathers results 
             
            pc.done() 

        Issues having to do with context can become quite complex. Context 
        transfer from one machine to another should be as small as possible. 
        Don't fall into the trap of a context transfer which takes longer 
        than the computation itself. Remember, you can do thousands of 
        c statements in the time it takes to transfer a few doubles. 
        Also, with a single cpu, it is often the case that statements 
        can be moved out of an innermost loop, but can't be in a parallel 
        computation. eg. 

        .. code-block::
            python

            # pretend g is a Vector assigned earlier to conductances to test 
            for i in range(20): 
                for sec in h.allsec():
                    sec.gnabar_hh = g[i]
                for j in range(5):
                    stim.amp = s[j]
                    h.run()

        ie we only need to set gnabar_hh 20 times. But the first pass at 
        parallelization would look like: 

        .. code-block::
            python

            def single_run(i, j):
                for sec in h.allsec():
                    sec.gnabar_hh = g[i]
                stim.amp = s[j]
                h.run()

            for i in range(1, 20):
               for j in range(5):
                  pc.submit(single_run, i, j) 
               
            
            while pc.working(): pass
            

        and we take the hit of repeated evaluation of gnabar_hh.
        A run must be quite lengthy to amortize this overhead. 

        To run under MPI, be sure to include the ``h.nrnmpi_init()`` and then
        launch your script via, e.g. ``mpiexec -n 4 python myscript.py``. NEURON
        also supports running via the PVM (parallel virtual machine), but the launch
        setup is different. If you do not have mpi4py and you have not exported
        the NEURON_INIT_MPI=1 environment variable then you can use the
        h.nrnmpi_init() method as long as that is executed prior to the first
        instantiation of ParallelContext.

        The exact same Python files should exist in the same relative locations 
        on all host machines. 

    .. warning::
        Not much checking for correctness or help in finding common bugs. 

        The best sanity test of a working mpi environment is testmpi.py

        .. code-block::
            python

            from neuron import h
            h.nrnmpi_init()

            pc = h.ParallelContext()
            print (f"I am {pc.id()} of {pc.nhost()}")

            pc.barrier()
            h.quit()            
         
        which gives ( the output lines are in indeterminate order)

        .. code-block::

            mpiexec -n 3 python testmpi.py
            numprocs=3
            I am 0 of 3
            I am 1 of 3
            I am 2 of 3

----



.. method:: ParallelContext.nhost


    Syntax:
        ``n = pc.nhost()``


    Description:
        Returns number of host neuron processes (master + workers). 
        If MPI (or PVM) is not being used then nhost == 1 and all ParallelContext 
        methods still work properly. 

        .. code-block::
            python

            import math

            if pc.nhost() == 1:
               for i in range(20):
                  print(i, math.sin(i))
               
            else: 
               for i in range(20):
                  pc.submit(i, math.sin, i) 
               
             
               while pc.working():
                  print(pc.userid(), pc.pyret())

    .. note::

        Prior to NEURON 7.6, this function returned a value of type ``float``;
        in more recent versions of NEURON, the return is an ``int``.
               
            


         

----



.. method:: ParallelContext.id


    Syntax:
        ``myid = pc.id()``


    Description:
        The ihost index which ranges from 0 to pc.nhost()-1 . Otherwise 
        it is 0. The master machine always has an pc.id() == 0. 

    .. warning::
        For MPI, the pc.id() is the rank from 
        MPI_Comm_rank.  For PVM the pc.id() is the order that the HELLO message was 
        received by the master. 

    .. note::

        Prior to NEURON 7.6, this function returned a value of type ``float``;
        in more recent versions of NEURON, the return is an ``int``.

----

.. method:: ParallelContext.mpiabort_on_error


    Syntax:
        ``oldflag = pc.mpiabort_on_error(newflag)``

    Description:
        Normally, when running with MPI, a hoc error causes a call to MPI_Abort
        so that all processes can be notified to exit cleanly without
        any processes hanging. (e.g. waiting for a communicator
        collective to complete). The call to MPI_Abort can be avoided by
        calling this method with newflag = 0. This occurs automatically
        when :func:`execute1` is used.  The method returns the previous
        value of the flag.


.. method:: ParallelContext.submit


    Syntax:

        ``pc.submit(python_callable, arg1, ...)``

        ``pc.submit(userid, ..as above..)``


    Description:
        Submits statement for execution by any host. Submit returns the userid not the 
        system generated global id of the task. 
        However when the task is executed, the :data:`hoc_ac_` variable 
        is set to this unique id (positive integer) of the task. 
        This unique id is returned by :meth:`ParallelContext.working` . 
         
        If the first argument to submit is a non-negative integer 
        then args are not saved and when the id for this 
        task is returned by :meth:`ParallelContext.working`, 
        that non-negative integer can be retrieved with 
        :meth:`ParallelContext.userid` 
         
        If there is no explicit userid, then the args (after the function name) 
        are saved locally and can be unpacked when the corresponding working 
        call returns. A local userid (unique only for this ParallelContext) 
        is generated and returned by the submit call and is also retrieved with 
        :meth:`ParallelContext.userid` when the corresponding working call returns. 
        This is very useful in associating a particular parameter vector with 
        its return value and avoids the necessity of explicitly saving them 
        or posting them. If they are not needed and you do not wish to 
        pay the overhead of storage, supply an explicit userid. 
        Unpacking args must be done in the same order and have the same 
        type as the args of the "function_name". They do not have to be unpacked. 
        Saving args is time efficient since it does not imply extra communication 
        with the server. 

        Arguments may be any pickleable objects (The NEURON :class:`Vector` is
        pickleable, and  most built-in Python objects and user-defined
        classes are pickleable). The callable is executed on some indeterminate MPI
        host. The return value is a Python object and may be retrieved with
        :meth:`pyret`.  Python object arguments 
        may be retrieved with :func:`upkpyobj`. 

    .. seealso::
        :meth:`ParallelContext.working`,
        :meth:`ParallelContext.retval`, :meth:`ParallelContext.userid`,
        :meth:`ParallelContext.pyret`

    .. warning::
        submit does not return the system generated unique id of the task but 
        either the first arg (must be a positive integer to be a userid) or 
        a locally (in this ParallelContext) generated userid which starts at 1. 
         
        A task should gather the results of all the tasks it submits before 
        scattering other tasks even if scattering with different ParallelContext 
        instances. This is because results are grouped by parent task id's 
        instead of (parent task id, pc instance). Thus the following idiom 
        needs extra user defined info to distinguish between pc1 and pc2 task 
        results. 

        .. code-block::
            python

            for i in range(10):
                pc1.submit(...) 
            for i in range(10):
                pc2.submit(...) 
            for i in range(10):
                pc1.working() ...
            for i in range(10):
                pc2.working() ...

        since pc1.working() may get a result from a pc2 submission 
        If this behavior is at all inconvenient, I will change the semantics 
        so that pc1 results only are gathered by pc1.working calls and by no 
        others. 
         
        Searching for the proper object context (pc.submit(object, ...) on the 
        host executing the submitted task is linear in the 
        number of objects of that type. 

         

----



.. method:: ParallelContext.working


    Syntax:
        ``id = pc.working()``


    Description:
        Returns 0 if there are no pending submissions which were 
        submitted by the current task. 
        (see bug below with regard to the distinction between the current 
        task and a ParallelContext instance). Returns the id of a previous pc.submit 
        which has completed 
        and whose results from that computation are ready for retrieval. 
         
        While there are pending submissions and results are not ready, pending 
        submissions from any ParallelContext from any host are calculated. 
        Note that returns of completed submissions are not necessarily in the 
        order that they were made by pc.submit. 

        .. code-block::
            python

            while True:
                id = pc.working()
                if id == 0: break
                # gather results of previous pc.submit calls
                print(id, pc.pyret())
        
        Note that if the submission did not have an explicit userid then 
        all the arguments of the executed function may be unpacked. 
         
        It is essential to emphasize that when 
        a task calls pc.working, while it is waiting for a result, it may 
        execute any number of other tasks and unless care is taken to 
        understand the meaning of "task context" and guarantee that 
        context after the working call is the same as the context before the 
        working call, SUBTLE ERRORS WILL HAPPEN more or less frequently 
        and indeterminately. For example consider the following: 

        .. code-block::
            python

            def f():
               ... write some values to some global variables ... 
               pc.submit(g, ...) 
               # when g is executed on another host it will not in general 
               # see the same global variable values you set above. 
               pc.working() # get back result of execution of g(...) 
               # now the global variables may be different than what you 
               # set above. And not because g changes them but perhaps 
               # because the host executing this task started executing 
               # another task that called f which then wrote DIFFERENT values 
               # to these global variables. 

        I only know one way around this problem. Perhaps there are other and 
        better ways. 

        .. code-block::
            python

            def f(): 
               id = hoc_ac_
               # write some values to some global variables ... 
               pc.post(id, the, global, variables) 
               pc.submit(g, ...) 
               pc.working() 
               pc.take(id) 
               # unpack the info back into the global variables 



    .. seealso::
        :meth:`ParallelContext.submit`,
        :meth:`ParallelContext.retval`, :meth:`ParallelContext.userid`,
        :meth:`ParallelContext.pyret`

    .. warning::
        Submissions are grouped according to parent task id and not by 
        parallel context instance. If suggested by actual experience, the 
        grouping will be according to the pair (parent task id, parallel 
        context instance). Confusion arises only in the case where a task 
        submits jobs  with one pc and fails to gather them before 
        submitting another group of jobs with another pc. See the bugs section 
        of :meth:`ParallelContext.submit` 

         

----



.. method:: ParallelContext.retval


    Syntax:
        ``scalar = pc.retval()``


    Description:
        The return value of the function executed by the task gathered by the 
        last :meth:`ParallelContext.working` call. 
        If the statement form of the submit is used then the return value 
        is the value of :data:`hoc_ac_` when the statement completes on the executing host. 

    .. warning::

        Use :meth:`ParallelContext.pyret` for tasks submitted as Python callables; do not use
        ``pc.retval()`` which only works for tasks submitted as HOC strings.
         

----



.. method:: ParallelContext.pyret


    Syntax:
        ``python_object = pc.pyret()``


    Description:
        If a task is submitted defined as a Python callable then the return 
        value can be any Python object and can only be retrieved with pyret(). 
        This function can only be called once for the task result gathered 
        by the last :meth:`ParallelContext.working` call. 

         

----



.. method:: ParallelContext.userid


    Syntax:
        ``scalar = pc.userid()``


    Description:
        The return value of the corresponding submit call. 
        The value of the userid is either the 
        first argument (if it was a non-negative integer) 
        of the submit call or else it is a positive integer unique only to 
        this ParallelContext. 
         
        See :meth:`ParallelContext.submit` with regard to retrieving the original 
        arguments of the submit call corresponding to the working return. 
         
        Can be useful in organizing results according to an index defined during 
        submission. 
         

         

----



.. method:: ParallelContext.runworker


    Syntax:
        ``pc.runworker()``


    Description:
        The master host returns immediately. Worker hosts start an 
        infinite loop of requesting tasks for execution. 
         
        The basic style is that the master and each host execute the 
        same code up til the pc.runworker call and that code sets up 
        all the context that is required to be identical on all hosts so 
        that any host can run any task whenever the host requests something 
        todo. The latter takes place in the runworker loop and when a task 
        is waiting for a result in a :meth:`ParallelContext.working` call. 
        Many parallel processing bugs 
        are due to inconsistent context among hosts and those bugs 
        can be VERY subtle. Tasks should not change the context required 
        by other tasks without extreme caution. The only way I know how 
        to do this safely 
        is to store and retrieve a copy of 
        the authoritative context on the bulletin board. See 
        :meth:`ParallelContext.working` for further discussion in this regard. 
         
        The runworker method is called automatically for each worker after 
        all files have been read in and executed --- i.e. if the user never 
        calls it explicitly from Python. Otherwise the workers would exit since 
        the standard input is at the end of file for workers. 
        This is useful in those cases where 
        the only distinction between master and workers is that code 
        executed from the gui or console. 

         

----



.. method:: ParallelContext.done


    Syntax:
        ``pc.done()``


    Description:
        Sends the QUIT message to all worker hosts. Those NEURON processes then 
        exit. The master waits til all worker output has been transferred to 
        the master host. 

         

----



.. method:: ParallelContext.context


    Syntax:

        ``pc.context(python_callable, arg1, ...)``

        ``pc.context(userid, ..as above..)``


    Description:
        The arguments have the same semantics as those of :meth:`ParallelContext.submit`. 
        The function or statement is executed on every worker host 
        but is not executed on the master. pc.context can only be 
        called by the master. The workers will execute the context statement 
        when they are idle or have completed their current task. 
        It probably only makes sense for the python_callable to return None. 
         
        There is no return in the 
        sense that :meth:`ParallelContext.working` does not return when one 
        of these tasks completes. 


    .. warning::
        It is not clear if it would be useful to generalize 
        the semantics to 
        the case of executing on every host except the 
        host that executed the pc.context call. 
        (strictly, the host would execute the task 
        when it requests something to do. 
        i.e. in a working loop or in a worker's infinite work loop.) 
        The simplest and safest use of this method is if it is called by the master 
        when all workers are idle. 
         
        This method was introduced in an attempt to get a parallel 
        multiple run fitter which worked in an interactive gui setting. 
        As such it increases safety but is not bulletproof since 
        there is no guarantee that the user doesn't change a global 
        variable that is not part of the fitter. It is also difficult 
        to write safe code that invariably makes all the relevant worker 
        context identical to the master.  An example of a common bug 
        is to remove a parameter from the parameter list and then 
        call save_context(). Sure enough, the multiple run fitters 
        on all the workers will no longer use that parameter, but 
        the global variables that depend on the parameter may be 
        different on different hosts and they will now stay different! 
        One fix is to call save_context() before the removal of the 
        parameter from the list and save_context() after its removal. 
        But the inefficiency is upsetting. We need a better automatic 
        mirroring method. 

         

----



.. method:: ParallelContext.post


    Syntax:
        ``pc.post(key)``

        ``pc.post(key, ...)``


    Description:
        Post the message with the address key, (key may be a string or scalar), 
        and a body consisting of any number of :meth:`ParallelContext.pack` calls since 
        the last post, and any number of arguments of type scalar, Vector, strdef 
        or Python object. 
         
        Later unpacking of the message body must be done in the same order as 
        this posting sequence. 

    .. seealso::
        :meth:`ParallelContext.pack`

         

----



.. method:: ParallelContext.take


    Syntax:
        ``pc.take(key)``

        ``pc.take(key, ...)``


    Description:
        Takes the message with key from the bulletin board. If the key does 
        not exist then the call blocks. Two processes can never take the same 
        message (unless someone posts it twice). The key may be a string or scalar. 
        Unpacking the message must take place in the same order as the packing 
        and must be complete before the next bulletin board operation. 
        (at which time remaining message info will be discarded) 
        It is not required to unpack the entire message, but later items cannot 
        be retrieved without unpacking earlier items first. Optional arguments 
        get the first unpacked values. Scalar, Vectors, and strdef may be 
        unpacked. Scalar arguments must be pointers to 
        a variable. eg  ``_ref_x``. Unpacked Vectors will be resized to the 
        correct size of the vector item of the message. 
        To unpack Python objects, :func:`upkpyobj` must be used. 

    .. seealso::
        :meth:`ParallelContext.upkstr`, :meth:`ParallelContext.upkscalar`,
        :meth:`ParallelContext.upkvec`, :meth:`ParallelContext.upkpyobj`

         

----



.. method:: ParallelContext.look


    Syntax:
        ``boolean = pc.look(key)``

        ``boolean = pc.look(key, ...)``


    Description:
        Like :meth:`ParallelContext.take` but does not block or remove message 
        from bulletin board. Returns 1 if the key exists, 0 if the key does 
        not exist on the bulletin board. The message associated with the 
        key (if the key exists) is available for unpacking each time 
        pc.look returns 1. 

    .. seealso::
        :meth:`ParallelContext.look_take`, :meth:`ParallelContext.take`

         

----



.. method:: ParallelContext.look_take


    Syntax:
        ``boolean = pc.look_take(key, ...)``


    Description:
        Like :meth:`ParallelContext.take` but does not block. The message is 
        removed from the bulletin board and two processes will never receive 
        this message. Returns 1 if the key exists, 0 if the key does not 
        exist on the bulletin board. If the key exists, the message can 
        be unpacked. 
         
        Note that a look followed by a take is *NOT* equivalent to look_take. 
        It can easily occur that another task might take the message between 
        the look and take and the latter will then block until some other 
        process posts a message with the same key. 

    .. seealso::
        :meth:`ParallelContext.take`, :meth:`ParallelContext.look`

         

----



.. method:: ParallelContext.pack


    Syntax:
        ``pc.pack(...)``


    Description:
        Append arguments consisting of scalars, Vectors, strdefs, 
        and pickleable Python objects into a message body 
        for a subsequent post. 

    .. seealso::
        :meth:`ParallelContext.post`

         

----



.. method:: ParallelContext.unpack


    Syntax:
        ``pc.unpack(...)``


    Description:
        Extract items from the last message retrieved with 
        take, look, or look_take. The type and sequence of items retrieved must 
        agree with the order in which the message was constructed with post 
        and pack. 
        Note that scalar items must be retrieved with pointer syntax as in 
        ``soma(0.3).hh._ref_gnabar`` 
        To unpack Python objects, :meth:`upkpyobj` must be used. 

    .. seealso::
        :meth:`ParallelContext.upkscalar`
        :meth:`ParallelContext.upkvec`, :meth:`ParallelContext.upkstr`
        :meth:`ParallelContext.upkpyobj`

         

----



.. method:: ParallelContext.upkscalar


    Syntax:
        ``x = pc.upkscalar()``


    Description:
        Return the scalar item which must be the next item in the unpacking 
        sequence of the message retrieved by the previous take, look, or look_take. 

         

----



.. method:: ParallelContext.upkstr


    Syntax:
        ``str = pc.upkstr(str)``


    Description:
        Copy the next item in the unpacking 
        sequence into str and return that strdef. 

    .. note::

        ``str`` here is a ``strdef`` not a Python string. One may be created via e.g. ``s = h.ref('')``; the stored string
        can then be accessed via ``s[0]``.

         

----



.. method:: ParallelContext.upkvec


    Syntax:
        ``vec = pc.upkvec()``

        ``vec = pc.upkvec(vecsrc)``


    Description:
        Copy the next item in the unpacking 
        sequence into vecsrc (if that arg exists, it will be resized if necessary). 
        If the arg does not exist return a new :class:`Vector`. 

         

----



.. method:: ParallelContext.upkpyobj


    Syntax:
        ``python_object = pc.upkpyobj()``


    Description:
        Return a reference to the (copied via pickling/unpickling) 
        Python object which must be the next item in the unpacking 
        sequence of the message retrieved by the previous take, look, or look_take. 

         

----



.. method:: ParallelContext.time


    Syntax:
        ``st = pc.time()``


    Description:
        Returns a high resolution elapsed wall clock time on the processor 
        (units of seconds) since an arbitrary time in the past. 
        Normal usage is 

        .. code-block::
            python

            st = pc.time() 
            ... 
            print(pc.time() - st)


    .. warning::
        A wrapper for MPI_Wtime when MPI is used. When PVM is used, the return 
        value is :samp:`clock_t times(struct tms {*buf})/100`. 

         

----



.. method:: ParallelContext.wait_time


    Syntax:
        ``total = pc.wait_time()``


    Description:
        The amount of time (seconds) 
        on a worker spent waiting for a message from the master. For the master, 
        it is the amount of time in the pc.take calls that was spent waiting. 
         
        To determine the time spent exchanging spikes during a simulation, use 
        the idiom: 

        .. code-block::
            python

            wait = pc.wait_time() 
            pc.psolve(tstop) 
            wait = pc.wait_time() - wait 


         

----



.. method:: ParallelContext.step_time


    Syntax:
        ``total = pc.step_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent integrating equations, checking thresholds, and delivering 
        events. It is essentially pc.integ_time + pc.event_time. 

         

----



.. method:: ParallelContext.send_time


    Syntax:
        ``total = pc.send_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent directing source gid spikes arriving on the target gid 
        to the proper PreSyn. 

         

----



.. method:: ParallelContext.event_time


    Syntax:
        ``total = pc.event_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent checking thresholds and delivering spikes. Note that 
        pc.event_time() + pc.send_time() will include all spike related time but 
        NOT the time spent exchanging spikes between cpus. 
        (Currently only for fixed step) 

         

----



.. method:: ParallelContext.integ_time


    Syntax:
        ``total = pc.integ_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent integrating equations. (currently only for fixed step) 

         

----



.. method:: ParallelContext.vtransfer_time


    Syntax:
        ``transfer_exchange_time = pc.vtransfer_time()``

        ``splitcell_exchange_time = pc.vtransfer_time(1)``

        ``reducedtree_computation_time = pc.vtransfer_time(2)``


    Description:
        The amount of time (seconds) 
        spent transferring and waiting for voltages or matrix elements. 
        The :func:`integ_time` is reduced by transfer and splitcell exchange times. 
         
        splitcell_exchange_time includes the reducedtree_computation_time. 
         
        reducedtree_computation_time refers to the extra time used by the 
        :meth:`ParallelContext.multisplit` backbone_style 1 and 2 methods between 
        send and receive of matrix information. This amount is also included 
        in the splitcell_exchange_time. 

         

----



.. method:: ParallelContext.mech_time


    Syntax:
        ``pc.mech_time()``

        ``mechanism_time = pc.mech_time(i)``


    Description:
        With no args initializes the mechanism time to 0. The next run will 
        record the computation time for BREAKPOINT and SOLVE statements of each 
        mechanism used in thread 0. When the index arg is present, the computation 
        time taken by the mechanism with that index is returned. 
        The index value is the internal 
        mechanism type index, not the index of the MechanismType. 

    .. seealso::
        :meth:`MechanismType.internal_type`


----


Implementation Notes
~~~~~~~~~~~~~~~~~~~~


Description:
    Some of these notes are PVM specific. 
     
    With the following information you may be encouraged to provide 
    a more efficient implementation. You may also see enough information 
    here to decide that this implementation is about as good as can be 
    expected in the context of your problem. 
     
    The master NEURON process contains the server for the bulletin board system. 
    Communication between normal Python  code executing on the master NEURON 
    process and the 
    server is direct with no overhead except packing and unpacking 
    messages and manipulating the send and receive buffers with pvm commands. 
    The reason I put the server into the master process is twofold. 
    1) While the master is number crunching, client messages are still 
    promptly dealt with. I noticed that when neuron was cpu bound, a separate 
    server process did not respond to requests for about a tenth of a second. 
    2) No context switching between master process and server. 
    If pvm is not running, a local implementation of the server is used 
    which has even less overhead than pvm packing and unpacking. 
     
    Clients (worker processes) communicate with the bulletin board server 
    (in the master machine) with pvm commands pvm_send and pvm_recv. 
    The master process is notified of asynchronous events via the SIGPOLL 
    signal. Unfortunately this is often early since a pvm message often 
    consists of several of these asynchronous events and my experience 
    so far is that (pvm_probe(-1,-1) > 0) is not always true even after 
    the last of this burst of signals. Also SIGPOLL is not available 
    except under UNIX. However SIGPOLL is only useful on the master 
    process and should not affect performance with regard to whether a 
    client is working under Win95, NT, or Linux. So even with SIGPOLL 
    there must be software polling on the server and this takes place 
    on the next execute() call in the interpreter. (an execute call 
    takes place when the body of every for loop, if statement, or 
    function/procedure call is executed.) In the absence of a SIGPOLL 
    signal this software polling takes place every POLLDELAY=20 
    executions. Of course this is too seldom in the case of 
    fadvance calls with a very large model, and too often in the case 
    of for i=1,100000 x+=i. Things are generally ok if the 
    message at the end of a run says that the amount of time spent 
    waiting for something to do is small compared to the amount of time 
    spent doing things. Perhaps a timer would help. 
     
    The bulletin board server consists of several lists implemented with 
    the STL (Standard Template Library) which makes for reasonably fast 
    lookup of keys. ie searching is not proportional to the size of the 
    list but proportional to the log of the list size. 
     
    Posts go into the message list ordered by key (string order). 
    They stay there until taken with look_take or take. 
    Submissions go into a work list ordered by id and a todo list of id's 
    by priority. When a host requests something to do, the highest priority 
    (first in the list) id is taken off the todo list. When done, the id goes 
    onto a results list ordered by parent id. When working is called 
    and a results list has an id with the right parent id, the 
    id is removed from the results list and the (id, message) pair 
    is removed from the work list. 
     
    If args are saved (no explicit userid in the submit call), they are 
    stored locally and become the active buffer on the corresponding 
    working return. The saving is in an STL map associated with userid. 
    The data itself is not copied but neither is it released until 
    the next usage of the receive buffer after the working call returns. 

         

----



.. _ParallelContext_MPI:

MPI
~~~

Description:
    If MPI is already installed, lucky you. You should ask the installer 
    for help. 
     
    Here is how I got it going on a 24 cpu beowulf cluster and 
    a dual processor Mac OSX G5. The cluster consisted of 12 dual processor 
    nodes named node0 to node11 and a master. From the outside world you 
    could only login to the master using ssh and from there to any of the nodes 
    you also had to use ssh. For a second opinion see 
    :doc:`Bill Lytton's notes on installing MPI <lyttonmpi>`.
     
    1) Figure out how to login to a worker without typing a password. 
     
    ie. do not go on unless you can 
    \ ``ssh node1`` or \ ``rsh node1``. If the former works then you must 
    \ ``export RSHCOMMAND=ssh`` before building the MPICH version of MPI since 
    that information is compiled into one of the files. It's too late to set 
    it after MPICH has been built. 
     
    On the Beowulf cluster master I did: 
    \ ``ssh-keygen -t rsa`` 
    and just hit return three times (once to use the default file location 
    and twice to specify and confirm an empty password). 
    Then I did a 
    \ ``cd $HOME/.ssh`` and copied the id_rsa.pub file to authorized_keys. 
    Now I could login to any node without using a password. 
     
    On the OSX machine I did the same thing but had to also check the 
    SystemPreferences/Internet&Network Sharing/Services/RemoteLogin box. 
     
    2) install MPI 
     
    I use http://www-unix.mcs.anl.gov/mpi/mpich/downloads/mpich.tar.gz 
    which on extraction ended up in $HOME/mpich-1.2.7. I built on 
    osx with 

    .. code-block::
        none

        export RSHCOMMAND=ssh 
        ./configure --prefix=`pwd`/powerpc --with-device=ch_p4 
        make 
        make install 

    and the same way on the beowulf cluster but with i686 instead of powerpc. 
    I then added $HOME/mpich-1.2.7/powerpc/bin to my PATH because the 
    NEURON configuration process will need to find mpicc and mpicxx 
    and we will eventually be using mpirun. 
     
    Note: some systems may have a 
    different implementation of MPI already installed and in that 
    implementation the c++ compiler 
    may be called mpic++. If that is in your path, then you will need to 
    go to $HOME/mpich-1.2.7/powerpc/bin and 
    \ ``ln -s mpicxx mpic++``. This will prevent NEURON's configure from becoming 
    confused and deciding to use mpicc from one MPI version and mpic++ from another! 
    ie. configure looks first for mpic++ and only if it does not find it does 
    it try mpicxx. 
     
    You can gain some confidence if you go to mpich-1.2.7/examples/basic and 
    test with 

    .. code-block::
        none

        make hello++ 
        mpirun -np 2 hello++ 

    If this fails on the mac, you may need a machine file with the proper 
    name that is indicated at the end of the $HOME/.ssh/authorized_keys file. 
    In my case, since ssh-keygen called my machine Michael-Hines-Computer-2.local 
    I have to use 

    .. code-block::
        none

        {mpirun -machinefile $HOME/mpifile -np 2 hello++ 

    where $HOME/mpifile has the single line 

    .. code-block::
        none

        Michael-Hines-Computer-2.local 

     
    3) build NEURON using the --with-paranrn argument. 
     
    On the beowulf my neuron 
    sources were in $HOME/neuron/nrn and interviews was installed in 
    $HOME/neuron/iv and I decided to build in a separate object directory called 
    $HOME/neuron/mpi-gcc2.96 so I created the latter directory, cd'd to it 
    and used 

    .. code-block::
        none

        ../nrn/configure --prefix=`pwd` --srcdir=../nrn --with-paranrn 

    On the mac, I created a $HOME/neuron/withmpi directory and configured with 

    .. code-block::
        none

        ../nrn/configure --prefix=`pwd` --srcdir=../nrn --with-paranrn \ 
        --enable-carbon --with-iv=/Applications/NEURON-5.8/iv 


     
    4) test by going to $HOME/neuron/nrn/src/parallel and trying 

    .. code-block::
        none

        mpirun -np 2  ~/neuron/withmpi/i686/bin/nrniv -mpi test0.hoc 

    You should get an output similar to 

    .. code-block::
        none

        nrnmpi_init(): numprocs=2 myid=0 
        NEURON -- Version 5.8 2005-8-22 19:58:19 Main (52) 
        by John W. Moore, Michael Hines, and Ted Carnevale 
        Duke and Yale University -- Copyright 1984-2005 
         
        loading membrane mechanisms from i686/.libs/libnrnmech.so 
        Additional mechanisms from files 
         
        hello from id 0 on NeuronDev 
         
                0 
        bbs_msg_cnt_=0 bbs_poll_cnt_=6667 bbs_poll_=93 
                0 
        hello from id 1 on NeuronDev 
         
        [hines@NeuronDev parallel]$ 
         

     
    5) If your machine is a cluster, list the machine names in a file 
    (on the beowulf cluster $HOME/mpi32 has the contents 

    .. code-block::
        none

        node0 
        ... 
        node11 

    ) 
    and I use the mpirun command 

    .. code-block::
        none

        mpirun -machinefile $HOME/mpi32 -np 24 \ 
            /home/hines/neuron/mpi*6/i686/bin/nrniv -mpi test0.hoc 

    On my mac, for some bizarre reason known only to the tiger creators, 
    the mpirun requires a machinefile with the line 

    .. code-block::
        none

        Michael-Hines-Computer-2.local 


         

----



.. method:: ParallelContext.mpi_init


    Syntax:
        ``h.nrnmpi_init()``


    Description:
        Initializes MPI if it has not already been initialized; mpi4py can
	also be used to intialize MPI.
        Only required if:

        launched python and mpi4py not used and NEURON_INIT_MPI=1
        environment varialble has not been exported.

        launched nrniv without -mpi argument.

        Since ``nrnmpi_init`` turns off gui for all ranks > 0, do not ``from neuron import gui``
        beforehand.

        The mpi_init method name was removed from ParallelContext and replaced
        with the HocTopLevelInterpreter method nrnmpi_init() because MPI
        must be initialized prior to the first instantiation of ParallelContext.

         
----



.. method:: ParallelContext.barrier


    Syntax:
        ``waittime = pc.barrier()``


    Description:
        Does an MPI_Barrier and returns the wait time at the barrier.  Execution 
        resumes only after all process reach this statement. 

         

----



.. method:: ParallelContext.allreduce


    Syntax:
        ``result = pc.allreduce(value, type)``

        ``pc.allreduce(src_dest_vector, type)``


    Description:
        Type is 1, 2, or 3 and the every host gets a 
        result as sum over all value, maximum 
        value, or minimum value respectively 
         
        If the first arg is a :class:`Vector` the reduce is done element-wise. ie 
        min of each rank's v[0] returned in each rank's v[0], etc. Note that 
        each vector must have the same size. 

         

----



.. method:: ParallelContext.allgather


    Syntax:
        ``pc.allgather(value, result_vector)``


    Description:
        Every host gets the value from every other host. The value from a host id 
        is in the id'th element of the vector. The vector is resized to size 
        pc.nhost. 

         

----



.. method:: ParallelContext.alltoall


    Syntax:
        ``pc.alltoall(vsrc, vcnts, vdest)``


    Description:
        Analogous to MPI_Alltoallv(...). vcnts must be of size pc.nhost and 
        vcnts.sum must equal the size of vsrc. 
        For host i, vcnts[j] elements of 
        vsrc are sent to host j beginning at the index vcnts.sum(0,j-1). 
        On host j, those elements are put into vdest beginning at the location 
        after the elements received from hosts 0 to i-1. 
        The vdest is resized to the number of elements received. 
        Note that vcnts are generally different for different hosts. If you need 
        to know how many came from what host, use the idiom 
        \ ``pc.alltoall(vcnts, one, vdest)`` where one is a vector filled with 1. 

        .. code-block::
            python

            # assume vsrc is a sorted Vector with elements ranging from 0 to tstop 
            # then the following is a parallel sort such that vdest is sorted on 
            # host i and for i < j, all the elements of vdest on host i are < 
            # than all the elements on host j. 
            vsrc.sort()
            cnts = h.Vector(pc.nhost()) 
            j = 0 
            for i in range(pc.nhost()):
              x = (i + 1) * tvl 
              k = 0 
              while j < s.size(): 
                if s[j] < x:
                  j += 1 
                  k += 1 
                else:
                  break 

              cnts[i] = k 

            pc.alltoall(vsrc, cnts, vdest)  


         
----



.. method:: ParallelContext.py_alltoall


    Syntax:
        ``destlist = pc.py_alltoall(srclist)``


    Description:
        Analogous to MPI_Alltoallv(...).
	The srclist must be a Python list of nhost pickleable Python objects.
        (Items with value None are allowed).
        The ith object is communicated to the ith host. the return value is
        a Python list of nhost items where the ith item was communicated
        by the ith host. This is a collective operation, so all hosts must
        participate.

        An optional second integer argument > 0 specifies the initial source
        pickle buffer size in bytes. The default size is 100k bytes. The size
        will grow by approximately doubling when needed.

        If the optional second argument is -1, then no transfers will be made
        and return value will be (src_buffer_size, dest_buffer_size) of the
        pickle buffers which would be needed for sending and receiving.

    Example:

        .. code-block::
            python

            from neuron import h
            h.nrnmpi_init()
            pc = h.ParallelContext()
            nhost = pc.nhost()
            rank = pc.id()

            #Keep host output from being intermingled.
            #Not always completely successful.
            import sys
            def serialize():
                for r in range(nhost):
                    pc.barrier()
                    if r == rank:
                        yield r
                        sys.stdout.flush()
                pc.barrier()

            data = [(rank, i) for i in range(nhost)]

            if rank == 0:
                print('source data')
            for r in serialize():
                print(rank, data)

            data = pc.py_alltoall(data)

            if rank == 0:
                print('destination data')

            for r in serialize():
                print(rank, data)

            pc.runworker()
            pc.done()
            h.quit()

        .. code-block::
            none

            $ mpirun -n 4 python parcon.py 
            numprocs=4
            source data
            0 [(0, 0), (0, 1), (0, 2), (0, 3)]
            1 [(1, 0), (1, 1), (1, 2), (1, 3)]
            2 [(2, 0), (2, 1), (2, 2), (2, 3)]
            3 [(3, 0), (3, 1), (3, 2), (3, 3)]
            destination data
            0 [(0, 0), (1, 0), (2, 0), (3, 0)]
            1 [(0, 1), (1, 1), (2, 1), (3, 1)]
            2 [(0, 2), (1, 2), (2, 2), (3, 2)]
            3 [(0, 3), (1, 3), (2, 3), (3, 3)]


----



.. method:: ParallelContext.py_allgather


    Syntax:
        ``destlist = pc.py_allgather(srcitem)``

    Description:
        Each rank sends its srcitem to all other ranks. All ranks assemble the
        arriving objects into an nhost size list such that the i'th element
        came from the i'th rank.
        The destlist is the same on every rank.
        The srcitem may be any pickleable Python object including None, Bool, int, h.Vector,
        etc. and will appear in the destination list as that type. This method can
        only be called from the python interpreter and cannot be called from HOC.
        All ranks (or all ranks in a subworld) must participate in this MPI collective.
        
        pc.py_allgather uses less memory and is faster than the equivalent
        ``destlist = pc.py_alltoall([srcitem]*nhost)``
        
 
    Example:

        .. code-block::
          python
          
          from neuron import h
          pc = h.ParallelContext()
          nhost = pc.nhost()
          rank = pc.id()
          
          src = rank
          dest = pc.py_allgather(src) 

          def pr(label, val):
            from time import sleep
            sleep(0.1) # try to avoid mixing different pr output
            print(f"{rank}: {label}: {val}")

          pr("allgather src", src)  
          pr("allgather dest", dest)

          src = [src]*nhost
          dest = pc.py_alltoall(src)        
          pr("alltoall src", src)   
          pr("alltoall dest", dest) 

          pc.barrier()
          h.quit()

       .. code-block::
          none

          $ mpiexec -n 4 nrniv -python -mpi test.py
          numprocs=4
          NEURON -- VERSION 7.6.4-4-gcd480afb master (cd480afb) 2019-01-04
          Duke, Yale, and the BlueBrain Project -- Copyright 1984-2018
          See http://neuron.yale.edu/neuron/credits

          0: allgather src: 0
          1: allgather src: 1
          2: allgather src: 2
          3: allgather src: 3
          0: allgather dest: [0, 1, 2, 3]
          1: allgather dest: [0, 1, 2, 3]
          2: allgather dest: [0, 1, 2, 3]
          3: allgather dest: [0, 1, 2, 3]
          2: alltoall src: [2, 2, 2, 2]
          0: alltoall src: [0, 0, 0, 0]
          1: alltoall src: [1, 1, 1, 1]
          3: alltoall src: [3, 3, 3, 3]
          1: alltoall dest: [0, 1, 2, 3]
          2: alltoall dest: [0, 1, 2, 3]
          0: alltoall dest: [0, 1, 2, 3]
          3: alltoall dest: [0, 1, 2, 3]

----




.. method:: ParallelContext.py_gather


    Syntax:
        ``destlist_on_root = pc.py_gather(srcitem, root)``

    Description:
        Each rank sends its srcitem to the root rank. The root rank assembles the
        arriving objects into an nhost size list such that the i'th element came from
        the i'th rank.
        The destlist_on_root return value for non-root ranks is None.
        The srcitem may be any pickleable Python object including None, Bool, int, h.Vector,
        etc. and will appear in the destination list as that type. This method can
        only be called from the python interpreter and cannot be called from HOC.
        All ranks (or all ranks in a subworld) must participate in this MPI collective.
        
        pc.py_gather uses less memory and is faster than the almost equivalent
        ``destlist = pc.py_alltoall([srcitem if i == root else None for i in range(nhost)])``
        "Almost" because the return value on non-root ranks is None for pc.py_allgather but
        a list of nhost None for pc.py_alltoall
 
    Example:

        .. code-block::
          python
          
          from neuron import h
          pc = h.ParallelContext()
          nhost = pc.nhost()
          rank = pc.id()

          root = 0 # any specific rank
          src = rank
          dest = pc.py_gather(src, root)

          def pr(label, val):
            from time import sleep
            sleep(.1) # try to avoid mixing different pr output
            print(f"{rank}: {label}: {val}")

          pr("gather src", src)     
          pr("gather dest", dest)

          src = [src if i == root else None for i in range(nhost)]
          dest = pc.py_alltoall(src)
          pr("alltoall src", src)  
          pr("alltoall dest", dest)

          pc.barrier()
          h.quit()
        
                    
        .. code-block::
          none
          
          $ mpiexec -n 4 nrniv -python -mpi test.py
          numprocs=4
          NEURON -- VERSION 7.6.4-4-gcd480afb master (cd480afb) 2019-01-04
          Duke, Yale, and the BlueBrain Project -- Copyright 1984-2018
          See http://neuron.yale.edu/neuron/credits
        
          3: gather src: 3
          1: gather src: 1
          2: gather src: 2
          0: gather src: 0
          3: gather dest: None
          1: gather dest: None
          2: gather dest: None
          0: gather dest: [0, 1, 2, 3]
          1: alltoall src: [1, None, None, None]
          2: alltoall src: [2, None, None, None]
          3: alltoall src: [3, None, None, None]
          0: alltoall src: [0, None, None, None]
          3: alltoall dest: [None, None, None, None]
          1: alltoall dest: [None, None, None, None]
          2: alltoall dest: [None, None, None, None]
          0: alltoall dest: [0, 1, 2, 3]

    .. note::

        Prior to NEURON 7.6, pc.nhost() and pc.id() returned a float instead of an int.

----




.. method:: ParallelContext.py_scatter


    Syntax:
        ``destitem_from_root = pc.py_scatter(srclist, root)``

    Description:
        The root rank sends the i'th element in its nhost size list to the i'th rank.
        The srclist must contain nhost pickleable Python objects including None, Bool,
        int, h.Vector,
        etc. and will appear in the destination list as that type. This method can
        only be called from the python interpreter and cannot be called from HOC.
        All ranks (or all ranks in a subworld) must participate in this MPI collective.
        
        pc.py_scatter uses less memory and is faster than the almost equivalent
        ``destitem = pc.pyalltoall(srclist if rank == root else [None]*nhost)``
        "Almost" because the return value on rank i for py.pyalltoall is a list
        filled with None
        except for the root'th item which is the i'th element of srclist of the root rank.

    Example:

        .. code-block::
          python
          
          from neuron import h
          pc = h.ParallelContext()
          nhost = pc.nhost()
          rank = pc.id()

          root = 0 # any specific rank
          src = [i for i in range(nhost)] if rank == root else None
          dest = pc.py_scatter(src, root)

          def pr(label, val):
            from time import sleep
            sleep(.1) # try to avoid mixing different pr output
            print(f"{rank}: {label}: {val}")

          pr("scatter src", src)     
          pr("scatter dest", dest)

          src = src if rank == root else [None]*nhost
          dest = pc.py_alltoall(src)
          pr("alltoall src", src)  
          pr("alltoall dest", dest)

          pc.barrier()
          h.quit()
         
         
                    
        .. code-block::
          none
          
          $ mpiexec -n 4 nrniv -python -mpi test.py
          numprocs=4
          NEURON -- VERSION 7.6.4-4-gcd480afb master (cd480afb) 2019-01-04
          Duke, Yale, and the BlueBrain Project -- Copyright 1984-2018
          See http://neuron.yale.edu/neuron/credits

          0: scatter src: [0, 1, 2, 3]
          2: scatter src: None
          1: scatter src: None
          3: scatter src: None
          2: scatter dest: 2
          0: scatter dest: 0
          1: scatter dest: 1
          3: scatter dest: 3
          1: alltoall src: [None, None, None, None]
          2: alltoall src: [None, None, None, None]
          0: alltoall src: [0, 1, 2, 3]
          3: alltoall src: [None, None, None, None]
          0: alltoall dest: [0, None, None, None]
          1: alltoall dest: [1, None, None, None]
          2: alltoall dest: [2, None, None, None]
          3: alltoall dest: [3, None, None, None]
         

----




.. method:: ParallelContext.py_broadcast


    Syntax:
        ``destitem_from_root = pc.py_broadcast(srcitem, root)``

    Description:
        The root rank sends the srcitem to every rank.
        The srcitem can be any pickleable Python object including None, Bool,
        int, h.Vector,
        etc. and will be returned as that type. This method can
        only be called from the python interpreter and cannot be called from HOC.
        All ranks (or all ranks in a subworld) must participate in this MPI collective.
        
        pc.py_broadcast uses less memory and is faster than the almost equivalent
        ``destitem = pc.pyalltoall([srcitem]*nhost if rank == root else [None]*nhost)``
        "Almost" because the return value on rank i for py.pyalltoall is a list
        filled with None
        except for the root'th item which is a copy of srcitem from the root rank.
        
    Example:

        .. code-block::
          python
          
          from neuron import h
          pc = h.ParallelContext()
          nhost = pc.nhost()
          rank = pc.id()

          root = 0 # any specific rank
          src = rank if rank == root else None
          dest = pc.py_broadcast(src, root)

          def pr(label, val):
            from time import sleep
            sleep(.1) # try to avoid mixing different pr output
            print(f"{rank}: {label}: {val}")

          pr("broadcast src", src)
          pr("broadcast dest", dest)

          src = [src]*nhost if rank == root else [None]*nhost
          dest = pc.py_alltoall(src)
          pr("alltoall src", src)
          pr("alltoall dest", dest)

          pc.barrier()
          h.quit()
       
                    
        .. code-block::
          none
          
          $ mpiexec -n 4 nrniv -python -mpi test.py
          numprocs=4
          NEURON -- VERSION 7.6.4-4-gcd480afb master (cd480afb) 2019-01-04
          Duke, Yale, and the BlueBrain Project -- Copyright 1984-2018
          See http://neuron.yale.edu/neuron/credits

          0: broadcast src: 0
          1: broadcast src: None
          2: broadcast src: None
          3: broadcast src: None
          0: broadcast dest: 0
          1: broadcast dest: 0
          2: broadcast dest: 0
          3: broadcast dest: 0
          2: alltoall src: [None, None, None, None]
          0: alltoall src: [0, 0, 0, 0]
          1: alltoall src: [None, None, None, None]
          3: alltoall src: [None, None, None, None]
          1: alltoall dest: [0, None, None, None]
          2: alltoall dest: [0, None, None, None]
          0: alltoall dest: [0, None, None, None]
          3: alltoall dest: [0, None, None, None]
          
 
----

.. method:: ParallelContext.broadcast


    Syntax:
        ``pc.broadcast(strdef, root)``

        ``pc.broadcast(vector, root)``


    Description:
        Every host gets the value from the host with pc.id == root. 
        The vector is resized to the size of the root host vector. 
        The return value is the length of the string or the size of the vector. 
        At the time that each other-than-root host reaches this statement 
        they receive the values sent from the root host. 

         

----


.. _subworld:

SubWorld
~~~~~~~~
Description:
    Without the methods discussed in this section, 
    the bulletin board and parallel network styles cannot be used together. 
    The parallel network style relies heavily on synchronization through 
    the use of blocking collective communication 
    methods and load balance is the primary consideration. The bulletin board 
    style is assynchronous and a process works on a submitted task generally 
    without communicating with other tasks except possibly and indirectly through 
    posting and taking messages on the bulletin board. 
    Without the subworld method, at most the network style can be used and then 
    switched to bulletin board style. The only way to simulate a parallel 
    network after executing :meth:`ParallelContext.runworker` would be to utilize 
    the :meth:`ParallelContext.context` method. In particular, without subworlds, 
    it is impossible to correctly submit bulletin board tasks, each of which 
    simulates a network specfied with the :ref:`ParallelNetwork` 
    methods --- even if the network is complete on a single process. This is
    because network simulations internally call MPI collectives which would
    deadlock due to not every worker participating in the collective.
     
    The :meth:`ParallelContext.subworlds` method divides the world of processors into subworlds, 
    each of which can execute a task (in parallel within the subworld) that independently and assynchronously 
    creates and simulates (and destroys if the task networks are different) 
    a separate 
    network described using the :ref:`ParallelNetwork` and 
    :ref:`ParallelTransfer` methods. The task, executing 
    in the subworld can also make use of the :ref:`ParallelContext_MPI` collectives. 
    Different subworlds can use the same global identifiers without 
    interference and the spike communication, transfers, and MPI collectives 
    are localized to within a subworld. I.e. in MPI terms, 
    each subworld utilizes a distinct MPI communicator. In a subworld, the 
    :meth:`ParallelContext.id` and :meth:`ParallelContext.nhost` refer to the rank and 
    number of processors in the subworld. (Note that every subworld has 
    a :meth:`ParallelContext.id` == 0 rank processor.)
     
    Only the rank :meth:`ParallelContext.id` == 0 subworld processors communicate 
    with the bulletin board. Of these processors, one
    (:meth:`~ParallelContext.id_world` == 0, i.e. :meth:`~ParallelContext.id_bbs` == 0 and :meth:`~ParallelContext.id` == 0)
    is the master processor and the others
    (:meth:`~ParallelContext.id_bbs` > 0 and :meth:`~ParallelContext.id` == 0)
    are the workers. The master 
    submits tasks to the bulletin board (and executes the task in parallel as a subworld with its total nhost ranks if no results 
    are available) and the workers execute tasks and id == 0 processes post the results 
    to the bulletin board. Remember, all the workers also have :meth:`ParallelContext.id` 
    == 0 but different :meth:`~ParallelContext.id_world` and :meth:`~ParallelContext.id_bbs` ranks. The subworld 
    :meth:`ParallelContext.id` ranks greater than 0 are not workers in the sense of directly interacting with the bulletin board by posting and taking messages --- their 
    global rank is :meth:`~ParallelContext.id_world` but their bulletin board rank, :meth:`~ParallelContext.id_bbs` is -1. To reduce confusion, it would be nice if the id_bbs value was the same as that of rank 0 of the subworld. But see the following paragraph for why the -1 value has the edge in usefulness. Fortunately the subworld (id_bbs) identifier is for these id()>0 ranks is easy to calculate as ``pc.id_world()//requested_subworld_size`` where the denominator is the argument used in :meth:`ParallelContext.subworlds`.
    (One cannot use ``pc.nhost()`` in place of the ``requested_subworld_size`` arg if ``pc.nhost_world()`` is not an integer multiple of ```requested_subworld_size`` since the last subworld will have ``pc.nhost() < requested_subworld_size``.)
    When a worker (or the master) receives a task to execute, the exact same 
    function with arguments that define the task will be executed on all the 
    processes of the subworld. A subworld is exactly analogous to the old 
    world of a network simulation in which processes distinguish themselves 
    by means of :meth:`ParallelContext.id` which is unique among 
    the :meth:`ParallelContext.nhost` processes in the subworld. 
     
    A runtime error will result if an :meth:`~ParallelContext.id_bbs` == -1 rank processor tries 
    to communicate with the bulletin board, thus the general idiom for 
    a task posting or taking information from the bulletin board should be 
    ``if (pc.id == 0) { ... }`` or if (pc.id_bbs != -1) { ... }. 
    The latter is more general since the former would not be correct if 
    :meth:`~ParallelContext.subworlds` has NOT been called since in that case 
    ``pc.id == pc.id_world == pc.id_bbs`` and 
    ``pc.nhost == pc.nhost_world == pc.nhost_bbs`` 
     

         

----



.. method:: ParallelContext.subworlds


    Syntax:
        ``pc.subworlds(subworld_size)``


    Description:
        Divides the world of all processors 
        into :func:`nhost_world` / subworld_size subworlds. 
        Note that the total number of processes, nhost_world, should be 
        an integer multiple of subworld_size. If that is not the case, the
        last subworld will have ``pc.nhost() < subworld_size``.
        The most useful subworld sizes are 1 and :func:`nhost_world` . 
        After return, for the processes 
        in each subworld, :meth:`ParallelContext.nhost` is equal to subworld_size (except the last if nhost_world is not in integer multiple of subworld_size).
        and the :meth:`ParallelContext.id` is the rank of the process with respect 
        to the subworld of which it is a part. 
         
        Each subworld has its own 
        unique MPI communicator for the :ref:`ParallelContext_MPI` functions such 
        as :meth:`ParallelContext.barrier` and so those collectives do not affect other subworlds. 
        All the :ref:`ParallelNetwork` notions are local to a subworld. I.e. independent 
        networks using the same gids can be simulated simultaneously in 
        different subworlds. Only rank 0 of a subworld ( :meth:`ParallelContext.id` 
        == 0) can use the bulletin board and has a non-negative :meth:`nhost_bbs` 
        and :meth:`id_bbs` . 
         
        Thus the bulletin board interacts with :func:`nhost_bbs` processes 
        each with :meth:`ParallelContext.id` == 0. And each of those rank 0 processes 
        interacts with :meth:`ParallelContext.nhost` processes using MPI commands 
        isolated within each subworld. 
         
        Probably the most useful values of subworld_size are 1 and :func:`nhost_world`. 
        The former uses the bulletin board to communicate between all processes 
        but allows the use of gid specified networks within each process. ie. 
        one master and nhost_world - 1 workers. 
        The latter uses all processes to simulate a parallel network and there 
        is only one process, the master, 
        (:meth:`id_world` == 0) interacting with the bulletin board. 
         

    Example:
        The following example is intended to be run with 6 processes. The subworlds 
        function with an argument of 3 will divide the 6 process world into 
        two subworlds each with 3 processes. To aid in seeing how the computation 
        progresses the function "f" prints its rank and number of processors 
        for the world, bulletin board, and net (subworld) as well as argument, 
        return value, and bulletin board defined userid. Prior to the runworker 
        call, all processes call f. After the runworker call, only the master 
        process returns and calls f. The master submits 4 tasks and then enters 
        a while loop waiting for results and, when a result is ready, prints 
        the userid, argument, and return value of the task. 
         

        .. code-block::
            python
 
            try:
                from mpi4py import MPI
            except:
                pass
            from neuron import h
            h.nrnmpi_init() #does nothing if mpi4py succeeded
            import time

            pc = h.ParallelContext() 
            pc.subworlds(3)

            def f(arg):
                ret = pc.id_world() * 100 + pc.id_bbs() * 10 + pc.id()
                print( 
                    "userid=%d arg=%d ret=%03d  world %d of %d  bbs %d of %d  net %d of %d" %
                    (h.hoc_ac_, arg, ret, pc.id_world(), pc.nhost_world(), pc.id_bbs(), pc.nhost_bbs(), pc.id(), pc.nhost()))
                time.sleep(1)
                return ret

            h.hoc_ac_ = -1 
            if (pc.id_world() == 0):
                print("before runworker")
            f(1)
            pc.runworker()
            print("\nafter runworker") 
            f(2)
             
            print("\nbefore submit")
            for i in range(3, 7):
                pc.submit(f, i)
            print("after submit") 
             
            while True:
                userid = pc.working()
                if not userid: break
                arg = pc.upkscalar()
                print("result userid=%d arg=%d return=%03d" % (userid, arg, pc.pyret()))

             
            print("\nafter working") 
            f(7)
            pc.done() 


         
        If the above code is saved in :file:`temp.py` and executed with 6 processes using 
        \ ``mpiexec -n 6 nrniv -mpi temp.py`` then the output will look like 
        (some lines may be out of order) 

        .. code-block::
            none

            $  mpirun -n 6 python temp.py
            numprocs=6
            NEURON -- VERSION 7.5 master (266b5a0) 2017-05-22
            Duke, Yale, and the BlueBrain Project -- Copyright 1984-2016
            See http://neuron.yale.edu/neuron/credits

            before runworker
            userid=-1 arg=1 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3
            userid=-1 arg=1 ret=091  world 1 of 6  bbs -1 of -1  net 1 of 3
            userid=-1 arg=1 ret=492  world 5 of 6  bbs -1 of -1  net 2 of 3
            userid=-1 arg=1 ret=192  world 2 of 6  bbs -1 of -1  net 2 of 3
            userid=-1 arg=1 ret=310  world 3 of 6  bbs 1 of 2  net 0 of 3
            userid=-1 arg=1 ret=391  world 4 of 6  bbs -1 of -1  net 1 of 3

            after runworker
            userid=-1 arg=2 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3

            before submit
            after submit
            userid=21 arg=4 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3
            userid=21 arg=4 ret=091  world 1 of 6  bbs -1 of -1  net 1 of 3
            userid=20 arg=3 ret=391  world 4 of 6  bbs -1 of -1  net 1 of 3
            userid=20 arg=3 ret=492  world 5 of 6  bbs -1 of -1  net 2 of 3
            userid=21 arg=4 ret=192  world 2 of 6  bbs -1 of -1  net 2 of 3
            userid=20 arg=3 ret=310  world 3 of 6  bbs 1 of 2  net 0 of 3
            result userid=21 arg=4 return=000
            result userid=20 arg=3 return=310
            userid=23 arg=6 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3
            userid=23 arg=6 ret=091  world 1 of 6  bbs -1 of -1  net 1 of 3
            userid=22 arg=5 ret=391  world 4 of 6  bbs -1 of -1  net 1 of 3
            userid=22 arg=5 ret=492  world 5 of 6  bbs -1 of -1  net 2 of 3
            userid=23 arg=6 ret=192  world 2 of 6  bbs -1 of -1  net 2 of 3
            userid=22 arg=5 ret=310  world 3 of 6  bbs 1 of 2  net 0 of 3
            result userid=23 arg=6 return=000
            result userid=22 arg=5 return=310

            after working
            userid=0 arg=7 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3
            $ 

        One can see from the output that before the runworker call, all the 
        processes called f. After runworker, only the master returned so there 
        is only one call to f. All tasks were submitted to the bulletin 
        board before any task generated print output. In this case, during 
        the while loop, the master started on the task with arg=4 and the two 
        associates within that subworld also executed f(4). Only the master 
        returned the result of f(4) to the bulletin board (the return values 
        of the two subworld associates were discarded). The master and its network 
        associates also executed f(5) and f(6). f(3) was executed by the world 
        rank 3 process (bbs rank 1, net rank 0) and that subworlds two net associates. 

         

----



.. method:: ParallelContext.nhost_world


    Syntax:
        ``numprocs = pc.nhost_world()``


    Description:
        Total number of processes in all subworlds. Equivalent to 
        :meth:`ParallelContext.nhost` when :func:`subworlds` has not been executed. 

         

----



.. method:: ParallelContext.id_world


    Syntax:
        ``rank = pc.id_world()``


    Description:
        Global world rank of the process. This is unique among all processes 
        of all subworlds and ranges from 0 to :func:`nhost_world` - 1 

         

----



.. method:: ParallelContext.nhost_bbs


    Syntax:
        ``numprocs = pc.nhost_bbs()``


    Description:
        If :func:`subworlds` has been called, nhost_bbs() returns the number of 
        subworlds if :meth:`ParallelContext.id` == 0 and -1 for all other ranks in 
        the subworld. 
        If subworlds has NOT been called then nhost_bbs, nhost_world, and nhost 
        are the same. 

         

----



.. method:: ParallelContext.id_bbs


    Syntax:
        ``rank = pc.id_bbs()``


    Description:
        If :func:`subworlds` has been called id_bbs() returns the subworld rank 
        if :meth:`ParallelContext.id` == 0 and -1 for all other ranks in the 
        subworld. 
        If subworlds has not been called then id_bbs, id_world, and id are the 
        same. 

         

----


.. _parallelnetwork:

Parallel Network
~~~~~~~~~~~~~~~~

Description:
    Extra methods for the ParallelContext that pertain to parallel network 
    simulations where cell communication involves discrete logical spike events. 
     
    The methods described in this section work for intra-machine connections 
    regardless of how NEURON is configured (Thus all parallel network models can 
    be executed on any serial machine). However machine spanning 
    connections can only be made if NEURON has been configured with 
    the --with-mpi option (or other options that automatically set it such as 
    --with-paranrn). (See :ref:`ParallelContext_MPI` for installation hints). 
     
    The fundamental requirement is that each 
    cell be associated with a unique integer global id (gid). The 
    :func:`ParallelNetManager` in nrn/share/lib/hoc/netparmpi.hoc is a sample 
    implementation that makes use of these facilities. That implementation 
    assumes that all conductance based cells contain a public 
    \ ``connect2target(targetsynapse, netcon)`` which connects the target synapse 
    object to a specific range variable (e.g. soma.v(.5)) and returns the 
    new NetCon in the second object argument. Artificial cells may either be 
    bare or wrapped in class and made public as a Point Process object field. That is, 
    cells built as NetworkReadyCells are compatible with the 
    ParallelNetManager and that manager follows as closely as possible 
    the style of network construction used by the NetGUI builder. 
     
    Notes: 
     
    Gid, sid, and pieces. 
     
    The typical network simulation sets up 
    a one to one correspondence between gid and cell. 
    This most common usage is suggested by 
    the method name, :meth:`ParallelContext.cell`, that makes the correspondence 
    as well as the accessor method, :meth:`ParallelContext.gid2cell`. 
    That's because, 
    almost always, a cell has one spike detection site and the entire cell is 
    on a single cpu. But either or both of those assertions can break down 
    and then one must be aware that, rigorously, 
    a gid is associated with a spike detection site (defined by 
    a NetCon source). For example, 
    many spike detection sites per cell are useful for reciprocal synapses. 
    Each side of each reciprocal synapse will require its own distinct gid. 
    When load balance is a problem, or when you have more cpus than cells, 
    it is useful to split cells into pieces and put the pieces on different 
    cpus (:meth:`ParallelContext.splitcell` and :meth:`ParallelContext.multisplit`). 
    But now, some pieces will not have a spike detection site and therefore 
    don't have to have a gid. In either case, it can be administratively 
    useful to invent an administrative policy for gid values that encodes 
    whole cell identification. For a cell piece that has no spike output, 
    one can still give it a gid associated with an arbitrary spike detection 
    site that is effectively turned off because it is not the source for 
    any existing NetCon and it was never specified as an 
    :meth:`ParallelContext.outputcell`. In the same way, it is also 
    useful to encode a :meth:`ParallelContext.multisplit` 
    sid (split id) with whole cell identification. 
     

.. warning::
    If mpi is 
    not available but NEURON has been built with PVM installed, an alternative 
    ParallelNetManager implementation with the identical interface is 
    available that makes use only of standard ParallelContext methods. 

         

----



.. method:: ParallelContext.set_gid2node


    Syntax:
        ``pc.set_gid2node(gid, id)``


    Description:
        If the id is equal to pc.id then this machine "owns" the gid and 
        the associated cell 
        should be eventually created only on this machine. 
        Note that id must be in the range 0 to pc.nhost()-1. The global id (gid) 
        can be any unique integer >= 0 but generally ranges from 0 to ncell-1 where 
        ncell is the total number of real and artificial cells. 
         
        Commonly, a cell has only one spike detector location and hence we normally 
        identify a gid with a cell. However, 
        cell can have several distinct spike detection locations or spike 
        detector point processes and each must be 
        associated with a distinct gid. (e.g. dendro-dendritic synapses). 

    .. seealso::
        :meth:`ParallelContext.id`, :meth:`ParallelContext.nhost`

         

----



.. method:: ParallelContext.gid_exists


    Syntax:
        ``integer = pc.gid_exists(gid)``


    Description:
        Return 3 if the gid is owned by this machine and the gid is already 
        associated with an output cell in the sense that its spikes will be 
        sent to all other machines. (i.e. :meth:`ParallelContext.outputcell` has 
        also been called with that gid or :meth:`ParallelContext.cell` has been 
        called with a third arg of 1.) 
         
        Return 2 if the gid is owned by this machine and has been associated with 
        a NetCon source location via the :func:`cell` method. 
         
        Return 1 if the gid is owned by this machine but has not been associated with 
        a NetCon source location. 
         
        Return 0 if the gid is NOT owned by this machine. 

         

----



.. method:: ParallelContext.threshold


    Syntax:
        ``th = pc.threshold(gid)``

        ``th = pc.threshold(gid, th)``


    Description:
        Return the threshold of the source variable determined by the first arg 
        of the :func:`NetCon` constructor which is used to associate the gid with a 
        source variable via :func:`cell` . If the second arg is present the threshold 
        detector is given that threshold. This method can only be called if the 
        gid is owned by this machine and :func:`cell` has been previously called. 


----



.. method:: ParallelContext.cell


    Syntax:
        ``pc.cell(gid, netcon)``

        ``pc.cell(gid, netcon, 0)``


    Description:
        The cell which is the source of the :func:`NetCon` is associated with the global 
        id. By default,(no third arg or third arg = 1) 
        the spikes generated by that cell will be sent to every other machine 
        (see :meth:`ParallelContext.outputcell`). A cell commonly has only one spike 
        generation location, but, for example in the case of reciprocal 
        dendro-dendritic synapses, there is no reason why it cannot have several. 
        The NetCon source defines the spike generation location. 
        Note that it is an error if the gid does not exist on this machine. The 
        normal idiom is to use a NetCon returned by a call to the cell's 
        connect2target(None, netcon) method or else, if the cell is an unwrapped 
        artificial cell, use a \ ``netcon = h.NetCon(cell, None)`` statement.
        In either case, after
        ParallelContext.cell() has been called, this NetCon can be
        destroyed to save memory; the spike detection threshold
        can be accessed by :meth:`ParallelContext.threshold`, and the
        weights and delays of projections to synaptic targets
        are specified by the parameters of the NetCons created
        by :meth:`ParallelContext.gid_connect`.
         
        Note that cells which do not send spikes to other machines are not required 
        to call this and in fact do not need a gid. However the administrative 
        detail would be significantly more complicated due to the multiplication 
        of cases in regard to whether the source and target exist AND the source 
        is an outputcell. 

         

----



.. method:: ParallelContext.outputcell


    Syntax:
        ``pc.outputcell(gid)``


    Description:
        Spikes this cell generates are to be distributed to all the other machines. 
        Note that :meth:`ParallelContext.cell` needs to be called prior to this and this 
        does not need to be called if the third arg of that was non-zero. 
        In principle there is no reason for a cell to even have a gid if it is not 
        an outputcell. However the separation between pc.cell and pc.outputcell 
        allows uniform administrative setup of the network to defer marking a cell 
        as an output cell until an actual machine spanning connection is made for 
        which the source is on this machine and the target is on another machine. 

         

----



.. method:: ParallelContext.spike_record


    Syntax:
        ``pc.spike_record(gid, spiketimevector, gidvector)``


    Description:
        This is a synonym for :meth:`NetCon.record` but obviates the requirement of 
        creating a NetCon using information about the source cell that is 
        relatively more tedious to obtain. This can only be called on the source 
        cell's machine. Note that a prerequisite is a call 
        to :meth:`ParallelContext.cell` . A call to :meth:`ParallelContext.outputcell` is NOT 
        a prerequisite. 

        If the gid arg is -1, then spikes from ALL output gids on this
        machine will be recorded.
         

----



.. method:: ParallelContext.gid_connect


    Syntax:
        ``netcon = pc.gid_connect(srcgid, target)``

        ``netcon = pc.gid_connect(srcgid, target, netcon)``


    Description:
        A virtual connection is made between the source cell global id (which 
        may or may not 
        be owned by this machine) and the target (a synapse or artificial cell object) 
        which EXISTS on this machine. A :class:`NetCon` object is returned and the 
        full delay for the connection should be given to it (as well as the weight).
        This is not the NetCon that
        monitors the spike source variable for threshold crossings, so its
        threshold parameter will not affect simulations.  Threshold crossings
        are determined by the detector that belonged to the NetCon used to
        associate the presynaptic spike source with a gid (see
        :meth:`ParallelContext.cell` and :meth:`ParallelContext.threshold`).         

        Note that if the srcgid is owned by this machine then :func:`cell` must be called 
        earlier to make sure that the srcgid is associated with a NetCon source 
        location. 
         
        Note that if the srcgid is not owned by this machine, then this machines 
        target will only get spikes from the srcgid if the source gid's machine 
        had called :meth:`ParallelContext.outputcell` or the third arg of 
        :meth:`ParallelContext.cell` was 1. 
         
        If the third arg exists, it must be a NetCon object with target the same 
        as the second arg. The src of that NetCon will be replaced by srcgid and 
        that NetCon returned. The purpose is to re-establish a connection to 
        the original srcgid after a :meth:`ParallelContext.gid_clear` . 

         

----



.. method:: ParallelContext.psolve


    Syntax:
        ``pc.psolve(tstop)``


    Description:
        This should be called on every machine to start something analogous to 
        cvode.solve(tstop). In fact, if the variable step method is invoked this 
        is exactly what will end up happening except the solve will be broken into 
        steps determined by the result of :meth:`ParallelContext.set_maxstep`. 

    Note:
        If CoreNEURON is active, psolve will be executed in CoreNEURON.
        Calls to psolve with CoreNEURON active and inactive can be
        interleaved and calls to finitialize can be interspersed. The result
        should be exactly the same as if all execution was done in NEURON
        (except for round-off error differences due to high performance
        optimizations).
         

----



.. method:: ParallelContext.timeout


    Syntax:
        ``oldtimeout = pc.timeout(seconds)``


    Description:
        During execution of :meth:`ParallelContext.psolve` , 
        sets the timeout for when to abort when seconds pass and t does not 
        increase.  Returns the old timeout.  The standard timeout is 20 seconds. 
        If the arg is 0, then there is no timeout. 
        The purpose of a timeout is to avoid wasting time on massively 
        parallel supercomputers when an error occurs such that one would wait 
        forever in a collective.  This function allows one to change the timeout 
        in those rare cases during a simulation where processes have to wait on 
        some process to finish a large amount work or some time step has an 
        extreme load imbalance. 

         

----



.. method:: ParallelContext.set_maxstep


    Syntax:
        ``local_minimum_delay = pc.set_maxstep(default_max_step)``


    Description:
        This should be called on every machine after all the NetCon delays have 
        been specified. It looks at all the delays on all the machines 
        associated with the netcons 
        created by the :meth:`ParallelContext.gid_connect` calls, ie the netcons 
        that conceptually span machines, and sets every machine's maximum step 
        size to the minimum delay of those netcons 
        (but not greater than default_max_step). The method returns this machines 
        minimum spanning netcon delay.  Assuming computational balance, generally 
        it is better to maximize the step size since it means fewer MPI_Allgather 
        collective operations per unit time. 

    .. warning::
        Note: No spikes can be delivered between machines unless this method 
        is called. finitialize relies on this method having been called. 
        If any trans-machine NetCon delay is reduced below the 
        step size, this method MUST be called again. Otherwise an INCORRECT 
        simulation will result. 

         

----



.. method:: ParallelContext.spike_compress

    Syntax:
        :samp:`nspike = pc.spike_compress({nspike}, {gid_compress}, {xchng_meth})`

    Description:
        If nspike > 0, selects an alternative implementation of spike exchange 
        that significantly compresses the buffers and can reduce interprocessor 
        spike exchange time by a factor of 10. This works only with the 
        fixed step methods. The optional second argument is 1 by default and 
        works only if the number of cells on each cpu is less than 256. 
        Nspike refers to the number of (spiketime, gid) pairs that fit into the 
        fixed buffer that is exchanged every :func:`set_maxstep` integration interval. 
        (overflow in the case where more spikes are generated in the interval 
        than can fit into the first buffer are exchanged when necessary by 
        a subsequent MPI_Allgatherv collective.) If necessary, the integration 
        interval is reduced so that there are less than 256 dt steps in the 
        interval. This allows the default (double spiketime, int gid) which 
        is at least 12 and possible 16 bytes in size to be reduced to a two 
        byte sequence. 
         
        This method should only be called after the entire network has 
        been set up since the gid compression mapping requires a knowledge 
        of which cells are sending interprocessor spikes. 
         
        If nspike = 0 , compression is turned off. 
         
        If nspike < 0, the current value of nspike is returned. 
         
        If gid_compress = 0, or if some cpu has more than 256 cells that send 
        interprocessor spikes, the real 4 byte integer gids are used in the 
        (spiketime, gid) pairs and only the spiketime is compressed to 1 byte. i.e. 
        instead of 2 bytes the pair consists of 5 bytes. 

        xchng_meth is a bit-field.
        bits | usage
           0 | 0: Allgather, 1: Multisend (MPI_ISend)
           1 | unused
           2 | 0: multisend_interval = 1, 1: multisend_interval = 2
           3 | 0: don't use phase2, 1: use phase2

    .. seealso::
        :meth:`CVode.queue_mode`

         

----



.. method:: ParallelContext.gid2obj


    Syntax:
        ``object = pc.gid2obj(gid)``


    Description:
        The cell or artificial cell object is returned that is associated with the 
        global id. Note that the gid must be owned by this machine. If the gid is 
        associated with a POINT_PROCESS that is located in a section which in turn 
        is inside an object, this method returns the POINT_PROCESS object. 

    .. seealso::
        :meth:`ParallelContext.gid_exists`, :meth:`ParallelContext.gid2cell`

    .. warning::
        Note that if a cell has several spike detection sources with different 
        gids, this is the method to use to return the POINT_PROCESS object itself. 

         

----



.. method:: ParallelContext.gid2cell


    Syntax:
        ``object = pc.gid2cell(gid)``


    Description:
        The cell or artificial cell object is returned that is associated with the 
        global id. Note that the gid must be owned by this machine. 
        If the gid is 
        associated with a POINT_PROCESS that is located in a section which in turn 
        is inside an object, this method returns the cell object, not the POINT_PROCESS 
        object. 

    .. seealso::
        :meth:`ParallelContext.gid_exists`, :meth:`ParallelContext.gid2obj`

    .. warning::
        Note that if a cell has several spike detection sources with different 
        gids, there is no way to distinguish them with this method. With those gid 
        arguments, gid2cell would 
        return the same cell where they are located. 

         

----



.. method:: ParallelContext.spike_statistics


    Syntax:
        ``nsendmax = pc.spike_statistics(_ref_nsend, _ref_nrecv, _ref_nrecv_useful)``


    Description:
        Returns the spanning spike statistics since the last :func:`finitialize` . All arguments 
        are optional. 
         
        nsendmax is the maximum number of spikes sent from this machine to all 
        other machines due to a single maximum step interval. 
         
        nsend is the total number of spikes sent from this machine to all other machines. 
         
        nrecv is the total number of spikes received by this machine. This 
        number is the same for all machines. 
         
        nrecv_useful is the total number of spikes received from other machines that 
        are sent to cells on this machine. (note: this does not include any 
        nsend spikes from this machine) 

    .. seealso::
        :meth:`ParallelContext.wait_time`, :meth:`ParallelContext.set_maxstep`

    .. note::

        The arguments for this function must be NEURON references to numbers; these can be
        created via, e.g. ``_ref_nsend = h.ref(0)`` and then dereferenced to get their
        values via ``_ref_nsend[0]``.

----



.. method:: ParallelContext.max_histogram


    Syntax:
        ``pc.max_histogram(vec)``


    Description:
        The vector, vec, of size maxspikes, is used to accumulate histogram information about the 
        maximum number of spikes sent by any cpu during the spike exchange process. 
        Every spike exchange, vec[max_spikes_sent_by_any_host] is incremented by 1. 
        It only makes sense to do this on one cpu, normally pc.id() == 0. 
        If some host sends more than maxspikes at the end of an
        integration interval, no element of vec is incremented.
         
        Note that the current implementation of the spike exchange mechanism uses 
        MPI_Allgather with a fixed buffer size that allows up to nrn_spikebuf_size 
        spikes per cpu to be sent to all other machines. The default value of this 
        is 0. If some cpu needs to send more than this number of spikes, then 
        a second MPI_Allgatherv is used to send the overflow. 

         

----



.. _paralleltransfer:

Parallel Transfer
~~~~~~~~~~~~~~~~~


    Description:
        Extends the :ref:`ParallelContext_MPI` :ref:`ParallelNetwork` methods to allow parallel simulation 
        of models involving gap junctions and/or 
        synapses where the postsynaptic conductance continuously 
        depends on presynaptic voltage. 
        Communication overhead for such models 
        is far greater than when the only communication between cells is with 
        discrete events. The greater overhead is due to the requirement for 
        exchanging information every time step. 
         
        Gap junctions are assumed to couple cells relatively weakly so that 
        the modified euler method is acceptable for accuracy and stability. 
        For purposes of load balance, and regardless of coupling strength, 
        a cell may be split into two subtrees 
        with each on a different processor. See :meth:`ParallelContext.splitcell`. 
        Splitting a cell into more than two pieces can be done with 
        :meth:`ParallelContext.multisplit` . 
         
        Except for "splitcell" and "multisplit, the methods described in this section work for intra-machine connections 
        regardless of how NEURON is configured. However 
        machine spanning connections can only be made if NEURON has been configured 
        with the --with-paranrn option. 
        (This automatically sets the --with-mpi option). 
         

    .. warning::
        Works for the fixed step method and the global variable step ode method 
        restricted to at_time events and NO discrete events. Presently does NOT 
        work with IDA (dae equations) or local variable step method. Does not work 
        with Cvode + discrete events. 

         

----



.. method:: ParallelContext.source_var


    Syntax:
        ``pc.source_var(_ref_v, source_global_index, sec=section)``


    Description:
        Associates the source voltage variable with an integer. This integer has nothing 
        to do with and does not conflict with the discrete event gid used by the 
        :ref:`ParallelNetwork` methods. 
        Must and can only be executed on the machine where the source voltage 
        exists. If extracellular is inserted at this location the voltage
        transferred is section.v(x) + section.vext[0](x) . I.e. the internal
        potential (appropriate for gap junctions).


    .. warning::
        An error will be generated if the the first arg pointer is not a
        voltage in ``section``. This was not an error prior
        to version 1096:294dac40175f trunk 19 May 2014

----



.. method:: ParallelContext.target_var


    Syntax:
        ``pc.target_var(_ref_target_variable, source_global_index)``

        ``pc.target_var(targetPointProcess, _ref_target_variable, source_global_index)``


    Description:
        Values for the source_variable associated with the source_global_index will 
        be copied to the target_variable every time step (more often for the 
        variable step methods). 
         
        Transfer occurs during :func:`finitialize` just prior to BEFORE BREAKPOINT blocks 
        of mod files and calls to type 0 :func:`FInitializeHandler` statements. For the 
        fixed step method, transfer occurs just before calling the SOLVE blocks. 
        For the variable step methods transfer occurs just after states are scattered. 
        Though any source variable can be transferred to any number of any target 
        variable, it generally only makes sense to transfer voltage values. 

    .. warning::
        If multiple threads are used, then the first arg must be the target point 
        process of which target_variable is a range variable. This is required so 
        that the system can determine which thread owns the target_variable. 
        Also, for the variable step methods, target_variable should not be located 
        at section position 0 or 1. 

         

----



.. method:: ParallelContext.setup_transfer


    Syntax:
        ``pc.setup_transfer()``


    Description:
        This method must be called after all the calls to :func:`source_var` and 
        :func:`target_var` and before initializing the simulation. It sets up the 
        internal maps needed for both intra- and inter-processor 
        transfer of source variable values to target variables. 

         

----



.. method:: ParallelContext.splitcell


    Syntax:
        ``pc.splitcell_connect(host_with_other_subtree, sec=rootsection)``


    Description:
        The root of the subtree specified by ``rootsection``
        is connected to the root of the 
        corresponding subtree located on the 
        host indicated by the argument. The method is very restrictive but 
        is adequate to solve the load balance problem. 
        The host_with_other_subtree must be either pc.id() + 1 or pc.id() - 1 
        and there can be only one split cell between hosts i and i+1. 
        A rootsection is defined as a section in which 
        :meth:`SectionRef.has_parent` returns 0. 
         
        This method is not normally called by the user but 
        is wrapped by the :func:`ParallelNetManager` method, 
        :meth:`ParallelNetManager.splitcell` which provides a simple interface to 
        support load balanced network simulations. 
         
        See :meth:`ParallelContext.multisplit` for less restrictive 
        parallel simulation of individual cells. 

    .. warning::
        Implemented only for fixed step methods. Cannot presently 
        be used with variable step 
        methods, or models with :func:`LinearMechanism`, or :func:`extracellular` . 

         

----



.. method:: ParallelContext.multisplit


    Syntax:
        ``pc.multisplit(section(x), sid)``

        ``pc.multisplit(section(x), sid, backbone_style)``

        ``pc.multisplit()``


    Description:
        For parallel simulation of single cells. Generalizes 
        :meth:`ParallelContext.splitcell` in a number of ways. 
        section(x) identifies a split node and can be any node, including 
        soma(0.5). The number of split nodes allowed on a (sub)tree is two or 
        fewer. Nodes with the same sid are connected by wires (0 resistance). 
         
        The default backbone_style (no third arg) is 2. With this style, we 
        allow multiple pieces of the same cell to be on the same cpu. This means 
        that one can split a cell into more pieces than available cpus in order 
        to more effectively load balance. 
         
        For backbone_style 2, the entire cell is solved 
        exactly via gaussian elimination regardless of the number of backbones 
        or their size. So the stability-accuracy properties are the same as if 
        the cell were entirely on one cpu. In this case all calls to multisplit 
        for that entire single cell must have no third arg or a third arg of 2. 
        Best performance militates that you should 
        split a cell so that it has as few backbones as possible consistent 
        with load balance since the reduced 
        tree matrix must be solved between the MPI matrix send phase and the MPI 
        matrix receive phase and that is a computation interval in which, 
        in many situations, nothing else can be accomplished. 
         
        The no arg call signals that no further multisplit calls will be 
        forthcoming and the system can determine the communication pattern 
        needed to carry out the multisplit computations. All hosts, even those 
        that have no multisplit cells, must participate in this determination. 
        (If anyone calls multisplit(...), everyone must call multisplit().) 
         
        For backbone_style 0 or 1, 
        if nodes have the same split id, sid, they must be on different hosts 
        but that is not a serious restriction since in that case 
        the subtrees would normally be connected together using 
        the standard :func:`connect` statement. 
         
        If all the trees connected into a single cell have only one 
        sid, the simulation is numerically identical to :meth:`ParallelContext.splitcell` 
        which is numerically identical to all the trees 
        connected together on a single cpu to form one cell. 
        If one or more of the trees has two sids, then numerical accuracy, 
        stability, and performance are a bit more ambiguous and depend on the 
        electrical distance between the two sids. The rule of thumb is that 
        voltage at one sid point should not significantly 
        affect voltage at the other sid point within a single time step. Note 
        that this electical distance has nothing to do with nseg. The stability 
        criterion is not proportional to dt/dx^2 but the much more favorable 
        dt/L^2 where dx is the size of the shortest segment and L is the 
        distance between the sid nodes. 
        In principle the subtrees of the whole cell can be the 
        individual sections. However the matrix solution of the nodes on the 
        path between the two sids takes twice as many divisions and 4 times 
        as many multiplications and subtractions as normally occurs on that 
        path. Hence there is an accuracy/performance optimum with respect 
        to the distance between sids on the same tree. This also complicates 
        load balance considerations. 
         
        If the third arg exists and is 1, for one or both 
        of the sids forming a backbone, 
        the backbone is declared to be short which means that it is solved 
        exactly by gaussian elimination without discarding any off diagonal 
        elements. Two short backbones cannot be connected together but they 
        may alternate with long backbones. If the entire cell consists of 
        single sid subtrees connected to a short backbone then the numerical 
        accuracy is the same as if the entire tree was gausian eliminated on 
        a single cpu. It does not matter if a one sid subtree is declared short 
        or not; it is solved exactly in any case. 
         
    .. warning::
        Implemented only for fixed step methods. Cannot presently 
        be used with variable step 
        methods, or models with :func:`LinearMechanism`, or :func:`extracellular` . 

    .. note::

        Prior to NEURON 7.5, the segment form was not supported and ``pc.multisplit(section(x), sid)``
        would instead be written ``pc.multisplit(x, sid, sec=section)``.

         

----



.. method:: ParallelContext.gid_clear


    Syntax:
        ``pc.gid_clear()``

        ``pc.gid_clear(type)``


    Description:
        With type = 1 
        erases the internal lists pertaining to gid information and cleans 
        up all the internal references to those gids. This allows one 
        to start over with new :func:`set_gid2node` calls. Note that NetCon and cell 
        objects would have to be dereferenced separately under user control. 
         
        With type = 2 clears any information setup by :meth:`ParallelContext.splitcell` or 
        :meth:`ParallelContext.multisplit`. 
         
        With type = 3 clears any information setup by :meth:`ParallelContext.setup_transfer`. 
         
        With a type arg of 0 or no arg, clears all the above information. 

         

----



.. method:: ParallelContext.Threads


    Description:
        Extends ParallelContext to allow parallel multicore simulations using 
        threads. 
        The methods in this section are only available in the multicore version of NEURON. 
         
        Multiple threads can only be used with fixed step or global variable time step integration methods.
	Also, they cannot be used with :func:`extracellular`, :func:`LinearMechanism`, 
	or the rxd (reaction-diffusion) module. Note that rxd provides its own threading
	for extracellular diffusion and 3d intracellular simulation, specified via
	e.g. ``rxd.nthread(4)``.
         
        Mechanisms that are not thread safe can only be used by thread 0. 
         
        Mod files that use VERBATIM blocks are not considered thread safe. The 
        mod file author can use the THREADSAFE keyword in the NEURON block to 
        force the thread enabled translation. 
         
        Mod files that assign values to GLOBAL variables are not considered 
        thread safe. If the mod file is using the GLOBAL as a counter, prefix 
        the offending assignment statements with the PROTECT keyword so that 
        multiple threads do not attempt to update the value at the same time 
        (race condition). If the mod file is using the GLOBAL essentially as 
        a file scope LOCAL along with the possibility of passing values back 
        to hoc in response to calling a PROCEDURE, use the THREADSAFE keyword 
        in the NEURON block to automatically treat those GLOBAL variables 
        as thread specific variables. NEURON assigns and evaluates only 
        the thread 0 version and if FUNCTIONs and PROCEDUREs are called from 
        Python, the thread 0 version of these globals are used. 


----



.. method:: ParallelContext.nthread


    Syntax:
        ``n = pc.nthread(n)``

        ``n = pc.nthread(n, 0)``

        ``n = pc.nthread()``


    Description:
        Specifies number of parallel threads. If the second arg is 0, the threads 
        are computed sequentially (but with thread 0 last). Sequential threads 
        can help with debugging since there can be no confounding race 
        conditions due to programming errors. With no args, the number of threads 
        is not changed. In all cases the number of threads is returned. On launch, 
        there is one thread. 


----



.. method:: ParallelContext.nworker


    Syntax:
        ``n = pc.nworker()``


    Description:
        Queries the number of active **worker** threads, that is to say the
        number of system threads that are executing work in parallel. If no
        work is being processed in parallel, this returns **zero**. This is
        related to, but sometimes different from, the number of threads that
        is set by :func:`ParallelContext.nthread`. That function sets (and
        queries) the number of thread data structures (``NrnThread``) that the
        model is partitioned into, and its second argument determines whether
        or not worker threads are launched to actually process those data in
        parallel.

        .. code-block:: python

            from neuron import config, h
            pc = h.ParallelContext()
            threads_enabled = config.arguments["NRN_ENABLE_THREADS"]

            # single threaded mode, no workers
            pc.nthread(1)
            assert pc.nworker() == 0

            # second argument specifies serial execution, no workers (but
            # there are two thread data structures)
            pc.nthread(2, False)
            assert pc.nworker() == 0

            # second argument specifies parallel execution, two workers if
            # threading was enabled at compile time
            pc.nthread(2, True)
            assert pc.nworker() == 2 * threads_enabled


        In the current implementation, ``nworker - 1`` extra threads are
        launched, and the final thread data structure is processed by the main
        application thread in parallel.


----



.. method:: ParallelContext.partition


    Syntax:
        ``pc.partition(i, seclist)``

        ``pc.partition()``


    Description:
        The seclist is a :func:`SectionList` which contains the root sections of cells 
        (or cell pieces, see :func:`multisplit`) which should be simulated by the thread 
        indicated by the first arg index. Either all or no thread can have 
        an associated seclist. The no arg form of pc.partition() unrefs the seclist 
        for all the threads. 


----


.. method:: ParallelContext.get_partition


    Syntax:
        ``seclist = pc.get_partition(i)``


    Description:
        Returns a new :func:`SectionList` with references to all the root sections
        of the ith thread.


----



.. method:: ParallelContext.thread_stat


    Syntax:
        ``pc.thread_stat()``


    Description:
        For developer use. Does not do anything in distributed versions. 


----



.. method:: ParallelContext.thread_busywait


    Syntax:
        ``previous = pc.thread_busywait(next)``


    Description:
        When next is 1, during a :func:`psolve` run, overhead for pthread condition waiting 
        is avoided by having threads watch continuously for a procedure to execute. 
        This works only if the number of threads is less than the number of cores 
        and uses 100% cpu time even when waiting. 


----



.. method:: ParallelContext.thread_how_many_proc


    Syntax:
        ``n = pc.thread_how_many_proc()``


    Description:
        Returns the number of concurrent threads supported by the hardware. This
        is the value returned by `std::thread::hardware_concurrency()
        <https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency>`_.
        On a system that supports hyperthreading this will typically be double
        the number of physical cores available, and it may not take into account
        constraints such as MPI processes being bound to specific cores in a
        cluster environment.


----



.. method:: ParallelContext.sec_in_thread


    Syntax:
        ``i = pc.sec_in_thread(sec=section)``


    Description:
        Whether or not ``section`` resides in the thread indicated by the 
        return value. 


----



.. method:: ParallelContext.thread_ctime


    Syntax:
        ``ct = pc.thread_ctime(i)``

        ``pc.thread_ctime()``


    Description:
        The high resolution walltime time in seconds the indicated thread 
        used during time step integration. Note that this does not include 
        reduced tree computation time used by thread 0 when :func:`multisplit` is 
        active. With no arg, sets thread_ctime of all threads to 0.

         
----

.. method:: ParallelContext.t

    Syntax:
        ``t = pc.t(tid)``

    Description:
        Return the current time of the tid'th thread

----

.. method:: ParallelContext.dt

    Syntax:
        ``dt = pc.dt(tid)``

    Description:
        Return the current timestep value for the tid'th thread


----

.. method:: ParallelContext.prcellstate

    Syntax:
        ``pc.precellstate(gid, "suffix")``

    Description:
        Creates the file <gid>_suffix.nrndat with all the range variable
        values and synapse/NetCon information associated with the gid.
        More complete than the HOC version of prcellstate.hoc in the standard
        library but a more terse in regard to names of variables. The purpose
        is for diagnosing the reason why a spike raster for a simulation is
        not the same for different nhost or gid distribution. One examines
        the diff between corresponding files from different runs.

        The format of the file is:

        .. code-block::
            none

            gid
            t
            # nodes, spike generator node
            List of node indices, parent node index, area, connection coefficients
              between node and parent
            List of node voltages
            For each mechanism in the cell
            Mechanism type, mechanism name, # variables for the mechanism instance
            For each instance of that mechanism in the cell
              If the mechanism is a POINT_PROCESS with a NET_RECEIVE block,
                node index, "nri", netreceive index for that POINT_PROCESS instance
              For each variable
                node index, variable index, variable value
            Number of netcons attached to the the cell.
            For each netcon
              netreceive index, srcgid or type name of source object, active, delay, weight vector


----

..  method:: ParallelContext.nrncore_write

    Syntax:
        ``pc.nrncore_write([path[, append_files_dat]])``

    Description:
        Writes files describing the existing model in such a way that those
        files can be read by CoreNEURON to simulate the model and produce
        exactly the same results as if the model were simulated in NEURON.

        The files are written in the directory specified by the path argument
        (default '.').

        Rank 0 writes a file called bbcore_mech.dat (into path) which lists
        all the membrane mechanisms in ascii format of:

        name type pointtype artificial is_ion param_size dparam_size charge_if_ion

        At the end of the bbcore_mech.dat file is a binary value that is
        used by the CoreNEURON reader to determine if byteswapping is needed
        in case of machine endianness difference between writing and reading.

        Each rank also writes pc.nthread() pairs of model data files containing
        mixed ascii and binary data that completely defines the model
        specification within a thread, The pair of files in each thread are
        named <gidgroup>_1.dat and <gidgroup>_2.dat  where gidgroup is one
        of the gids in the thread (the files contain data for all the gids
        in a thread). <gidgroup>_1.dat contains network topology data and
        <gidgroup>_2.dat contains all the data needed to actually construct
        the cells and synapses and specify connection weights and delays.

        If the second argument does not exist or has a value of False (or 0),
        rank 0 writes a "files.dat" file with version string, a -1
        indicator if there are gap junctions, and a integer value that
        specifies the total number of gidgroups followed by one gidgroup value per
        line for all threads of all ranks.

        If the model is too large to exist in NEURON (models typcially use
        an order of magnitude less memory in CoreNEURON) the model can
        be constructed in NEURON as a series of submodels.
        When one submodel is constructed
        on each rank, this function can be called with a second argument
        with a value of True (or nonzero) which signals that the existing
        files.dat file should have its n_gidgroups line updated
        and the pc.nthread() gidgroup values for each rank should be
        appended to the files.dat file. Note that one can either create
        submodels sequentially within a single launch, though that requires
        a "teardown" function to destroy the model in preparation for building
        the next submodel, or sequentially create the submodels as a series
        of separate launches. A user written "teardown" function should,
        in order, free all gids with :func:`gid_clear` , arrange for all
        NetCon to be freed, and arrange for all Sections to be destroyed.
        These latter two are straightforward if the submodel is created as
        an instance of a class. An example of sequential build, nrncore_write,
        teardown is the test_submodel.py in
        http://github.com/neuronsimulator/ringtest.

        Multisplit is not supported.
        The model cannot be more complicated than a spike or gap
        junction coupled parallel network model of real and artificial cells.
        Real cells must have gids, Artificial cells without gids connect
        only to cells in the same thread. No POINTER to data outside of the
        thread that holds the pointer. 

----

..  method:: ParallelContext.nrncore_run

    Syntax:
        ``pc.nrncore_run(argstr, [bool])``

    Description:
        Run the model using CoreNEURON in online (direct transfer) mode
        using the arguments specified in
        argstr. If the optional second arg, bool, default 0, is 1, then
        trajectory values are sent back to NEURON on every time step to allow
        incremental plotting of Graph lines. Otherwise, trajectories are
        buffered and sent back at the end of the run. In any case, all
        variables and event queue state are copied back to NEURON at the end
        of the run as well as spike raster data.

        This method is not generally used since running a model using CoreNEURON
        in online mode is easier with the idiom:

        .. code-block:: python

            from neuron import h, gui
            pc = h. ParallelContext()
            # construct model ...

            # run model
            from neuron import coreneuron
            coreneuron.enable = True
            h.stdinit()
            pc.psolve(h.tstop)

        In this case, :func:`psolve`, uses ``nrncore_run`` behind the scenes
        with the argstr it gets from ``coreneuron.nrncore_arg(h.tstop)``
        which is ``" --tstop 5 --cell-permute 1 --verbose 2 --voltage 1000."``

        CoreNEURON in online mode does not do the
        equivalent :func:`finitialize`
        but relies on NEURON's initialization of states and event queue.
