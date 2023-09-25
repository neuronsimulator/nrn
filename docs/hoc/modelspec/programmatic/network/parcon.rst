
.. _hoc_parcon:

ParallelContext
---------------

.. toctree::
    :hidden:

    lyttonmpi.rst


.. hoc:class:: ParallelContext


    Syntax:
        ``objref pc``

        ``pc = new ParallelContext()``

        ``pc = new ParallelContext(nhost)``


    Description:
        "Embarrassingly" parallel computations using a Bulletin board style 
        analogous to LINDA. (But see the :ref:`hoc_ParallelNetwork`,
        :hoc:class:`ParallelNetManager` and :ref:`hoc_ParallelTransfer` discussions.
        Also see :ref:`hoc_SubWorld` for a way to simultaneously use
        the bulletin board and network simulations involving global identifiers.) 
        Useful when doing weeks or months worth of 
        simulation runs each taking more than a second and where not much 
        communication is required.  Eg.  parameter sensitivity, and some forms 
        of optimization.  The underlying strategy is to keep all machines in a 
        PVM or :ref:`hoc_ParallelContext_MPI`
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
        :hoc:meth:`submit`, :hoc:meth:`context`, :hoc:meth:`pack`, and :hoc:meth:`post`
        have been augmented and :hoc:meth:`pyret` and :hoc:meth:`upkpyobj` have been introduced
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
            none

            func f() {              // a function with no context that *CHANGES* 
               return $1*$1         //except its argument 
            } 
             
            objref pc 
            pc = new ParallelContext() 
            pc.runworker()          // master returns immediately, workers in 
                                    // infinite loop running and jobs from bulletin board 
            s = 0 
            if (pc.nhost == 1) {    // use the serial form 
               for i=1, 20 { 
                  s += f(i) 
               } 
            }else{                  // use the bulletin board form 
               for i=1, 20 {        // scatter processes 
                  pc.submit("f", i) // any context needed by f had better be 
               }                    // the same on all hosts 
               while (pc.working) { // gather results 
                  s += pc.retval    // the return value for the executed function 
               } 
            } 
            print s 
            pc.done                 // tell workers to quit 

         
        Several things need to be highlighted: 
         
        If a given task submits other tasks, only those child tasks 
        will be gathered by the working loop for that given task. 
        At this time the system groups tasks according to the parent task 
        and the pc instance is not used. See :hoc:meth:`ParallelContext.submit` for
        further discussion of this limitation. The safe strategy is always to 
        use the idiom: 

        .. code-block::
            none

            for i = 1,n {pc.submit(...)} // scatter a set of tasks 
            while(pc.working)) { ... }   // gather them all 

         
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
         
        The pc.working call checks to see if a result is ready. If so it returns 
        the unique system generated task id (a positive integer) 
        and the return value of the task 
        function is accessed via 
        the pc.retval function. The arguments to the function executed by the 
        submit call are also available. If all submissions have been computed and all 
        results have been returned, pc.working returns 0. If results are 
        pending, working executes tasks from ANY ParallelContext until a 
        result is ready. This last feature keeps cpus busy but places stringent 
        requirements on how the user changes global context without 
        introducing bugs. See the discussion in :hoc:meth:`ParallelContext.working` .
         
        ParallelContext.working may not return results in the order of 
        submission. 
         
        Hoc code subsequent to pc.runworker() is executed only by the 
        master since that call returns immediately if the process is 
        the master and otherwise starts an infinite loop on each worker 
        which requests and executes submit tasks from ANY ParallelContext 
        instance. This is the standard way to seed the bulletin board with 
        submissions. Note that workers may also execute tasks that themselves 
        cause submissions. If subsidiary tasks call pc.runworker, the call 
        returns immediately. Otherwise the task 
        it is working on would never complete! 
        The pc.runworker() function is also called for each worker after all hoc files 
        are read in and executed. 
         
        The basic organization of a simulation is: 

        .. code-block::
            none

            //setup which is exactly the same on every machine. 
            // ie declaration of all functions, procedures, setup of neurons 
             
            pc.runworker() to start the execute loop if this machine is a worker 
             
            // the master scatters tasks onto the bulletin board and gathers results 
             
            pc.done() 

        Issues having to do with context can become quite complex. Context 
        transfer from one machine to another should be as small as possible. 
        Don't fall into the trap of a context transfer which takes longer 
        than the computation itself. Remember, you can do thousands of 
        c statements in the time it takes to transfer a few doubles. 
        Also, with a single cpu, it is often the case that  statements 
        can be moved out of an innermost loop, but can't be in a parallel 
        computation. eg. 

        .. code-block::
            none

            // pretend g is a Vector assigned earlier to conductances to test 
            for i = 1, 20 
               forall gnabar_hh = g.x[i] 
               for j = 1, 5 
                  stim.amp = s[j] 
                  run() 
               } 
            } 

        ie we only need to set gnabar_hh 20 times. But the first pass at 
        parallelization would look like: 

        .. code-block::
            none

            for i = 1, 20 { 
               for j= 1, 5 { 
                  sprint(cmd, "{forall gnabar_hh = g[%d]} stim.amp = s[%d] run()\n", i, j) 
                  pc.submit(cmd) 
               } 
            } 
            while (pc.working) { 
            } 

        and not only do we take the hit of repeated evaluation of gnabar_hh 
        but the statement must be interpreted each time. A run must be quite 
        lengthy to amortize this overhead. 
         
        Python version 
         
        Here we re-implement the first example above as a Python program 

        .. code-block::
            none

            from neuron import h 
             
            def f(arg):             # a function with no context that *CHANGES* 
               return arg*arg       #except its argument 
             
            pc = h.ParallelContext() 
            pc.runworker()          # master returns immediately, workers in 
                                    # infinite loop 
            s = 0 
            if pc.nhost() == 1:     # use the serial form 
               for i in range(1, 21): 
                  s += f(i) 
            else:                   # use the bulletin board form 
               for i in range(1, 21): # scatter processes 
                  pc.submit(f, i)   # any context needed by f had better be the same on all$ 
               while pc.working():  # gather results 
                  s += pc.pyret()   # the return value for the executed function 
            print s 
            pc.done()               # wait for workers to finish printing 

        Note the replacement of the string "f" in the submit method by a Python 
        Callable object and the retrieval of the result by the pyret() method 
        instead of retval(). 
         
        The PVM (parallel virtual machine) 
        should be setup so that it allows 
        execution on all hosts of the csh script :file:`$NEURONHOME/bin/bbsworker.sh`. 
        (Simulations may also be run under :ref:`hoc_ParallelContext_MPI` but the launch
        mechanisms are quite different) 
        The simulation hoc files should be available on each machine with 
        the same relative path with respect to the user's $HOME directory. 
        For example, I start my 3 machine pvm with the command 

        .. code-block::
            none

               pvm hineshf 

        where hineshf is a text file with the contents: 

        .. code-block::
            none

            hines ep=$HOME/nrn/bin 
            spine ep=$HOME/nrn/bin 
            pinky ep=$HOME/nrn/bin 

        Again, the purpose of the ep=$HOME/nrn/bin tokens is to specify the path 
        to find bbsworker.sh 
         
        A simulation is started by moving to the proper working directory 
        (should be a descendant of your $HOME directory) and launching neuron as in 

        .. code-block::
            none

            special init.hoc 

        The exact same hoc files should exist in the same relative locations 
        on all host machines. 

    .. warning::
        Not much checking for correctness or help in finding common bugs. 

         

----



.. hoc:method:: ParallelContext.nhost


    Syntax:
        ``n = pc.nhost()``


    Description:
        Returns number of host neuron processes (master + workers). 
        If PVM (or MPI) is not being used then nhost == 1 and all ParallelContext 
        methods still work properly. 

        .. code-block::
            none

            if (pc.nhost == 1) { 
               for i=1, 20 { 
                  print i, sin(i) 
               } 
            }else{ 
               for i=1,20 { 
                  pc.submit(i, "sin", i) 
               } 
             
               while (pc.working) { 
                  print pc.userid, pc.retval 
               } 
            } 


         

----



.. hoc:method:: ParallelContext.id


    Syntax:
        ``myid = pc.id()``


    Description:
        The ihost index which ranges from 0 to pc.nhost-1 . Otherwise 
        it is 0. The master machine always has an pc.id == 0. 

    .. warning::
        For MPI, the pc.id is the rank from 
        MPI_Comm_rank.  For PVM the pc.id is the order that the HELLO message was 
        received by the master. 

         

----



.. hoc:method:: ParallelContext.submit


    Syntax:
        ``pc.submit("statement\n")``

        ``pc.submit("function_name", arg1, ...)``

        ``pc.submit(object, "function_name", arg1, ...)``

        ``pc.submit(userid, ..as above..)``

        ``pc.submit(python_callable, arg1, ...)``


    Description:
        Submits statement for execution by any host. Submit returns the userid not the 
        system generated global id of the task. 
        However when the task is executed, the :hoc:data:`hoc_ac_` variable
        is set to this unique id (positive integer) of the task. 
        This unique id is returned by :hoc:meth:`ParallelContext.working` .
         
        If the first argument to submit is a non-negative integer 
        then args are not saved and when the id for this 
        task is returned by :hoc:meth:`ParallelContext.working`,
        that non-negative integer can be retrieved with 
        :hoc:meth:`ParallelContext.userid`
         
        If there is no explicit userid, then the args (after the function name) 
        are saved locally and can be unpacked when the corresponding working 
        call returns. A local userid (unique only for this ParallelContext) 
        is generated and returned by the submit call and is also retrieved with 
        :hoc:meth:`ParallelContext.userid` when the corresponding working call returns.
        This is very useful in associating a particular parameter vector with 
        its return value and avoids the necessity of explicitly saving them 
        or posting them. If they are not needed and you do not wish to 
        pay the overhead of storage, supply an explicit userid. 
        Unpacking args must be done in the same order and have the same 
        type as the args of the "function_name". They do not have to be unpacked. 
        Saving args is time efficient since it does not imply extra communication 
        with the server. 
         
        The argument form causes function_name(copyofarg1, ...) to execute 
        on some indeterminate host in the PVM. Args must be scalars, strings, or 
        Vectors. Note that they are *COPIES* so that even string and Vector 
        arguments are call by value and not call by reference. (This is different 
        from the normal semantics of a direct function call). In this case 
        efficiency was chosen at the expense of pedantic consistency 
        since it is expected 
        that in most cases the user does not need the return copy. In the event 
        more than a single scalar return value is required use :hoc:meth:`ParallelContext.post`
        within the function_name body with a key equal to the id of the task. 
        For example: 

        .. code-block::
            none

            func function_name() {local id 
               id = hoc_ac_ 
               $o1.reverse() 
               pc.post(id, $o1) 
               return 0 
            } 
            ... 
            while( (id = pc.working) != 0) { 
               pc.take(id) 
               pc.upkvec.printf 
            } 

        The object form executes the function_name(copyofarg1, ...) in the 
        context of the object. IT MUST BE THE CASE that the string result 

        .. code-block::
            none

               print object 

        identifies the "same" object on the host executing the function 
        as on the host that submitted the task. This is guaranteed only if 
        all hosts, when they start up, execute the same code that creates 
        these objects. If you start creating these objects after the worker 
        code diverges from the master (after pc.runworker) you really have to 
        know what you are doing and the bugs will be VERY subtle. 
         
        The python_callable form allows args to be any Python objects as well 
        as numbers, strings, or hoc Vectors. The return is a Python object 
        and can only be retrieved with :hoc:func:`pyret` . The Python objects must be
        pickleable (hoc objects are not presently pickleable). Python object arguments 
        may be retrieved with :hoc:func:`upkpyobj`.

    .. seealso::
        :hoc:meth:`ParallelContext.working`,
        :hoc:meth:`ParallelContext.retval`, :hoc:meth:`ParallelContext.userid`,
        :hoc:meth:`ParallelContext.pyret`

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
            none

            for i=1,10 pc1.submit(...) 
            for i=1,10 pc2.submit(...) 
            for i=1,10 { pc1.working() ...) 
            for i=1,10 { pc2.working() ...) 

        since pc1.working may get a result from a pc2 submission 
        If this behavior is at all inconvenient, I will change the semantics 
        so that pc1 results only are gathered by pc1.working calls and by no 
        others. 
         
        Searching for the proper object context (pc.submit(object, ...) on the 
        host executing the submitted task is linear in the 
        number of objects of that type. 

         

----



.. hoc:method:: ParallelContext.working


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
            none

            while ((id = pc.working) > 0) { 
               // gather results of previous pc.submit calls 
               print id, pc.retval 
            } 

        Note that if the submitted task was specified as a Python callable, then 
        :hoc:func:`pyret` would have to be used in place of :hoc:func:`retval` .
         
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
            none

            function f() { 
               ... write some values to some global variables ... 
               pc.submit("g", ...) 
               // when g is executed on another host it will not in general 
               // see the same global variable values you set above. 
               pc.working() // get back result of execution of g(...) 
               // now the global variables may be different than what you 
               // set above. And not because g changes them but perhaps 
               // because the host executing this task started executing 
               // another task that called f which then wrote DIFFERENT values 
               // to these global variables. 

        I only know one way around this problem. Perhaps there are other and 
        better ways. 

        .. code-block::
            none

            function f() { local id 
               id = hoc_ac_; 
               ... write some values to some global variables ... 
               pc.post(id, the, global, variables) 
               pc.submit("g", ...) 
               pc.working() 
               pc.take(id) 
               // unpack the info back into the global variables 
               ... 
            } 


    .. seealso::
        :hoc:meth:`ParallelContext.submit`,
        :hoc:meth:`ParallelContext.retval`, :hoc:meth:`ParallelContext.userid`,
        :hoc:meth:`ParallelContext.pyret`

    .. warning::
        Submissions are grouped according to parent task id and not by 
        parallel context instance. If suggested by actual experience, the 
        grouping will be according to the pair (parent task id, parallel 
        context instance). Confusion arises only in the case where a task 
        submits jobs  with one pc and fails to gather them before 
        submitting another group of jobs with another pc. See the bugs section 
        of :hoc:meth:`ParallelContext.submit`

         

----



.. hoc:method:: ParallelContext.retval


    Syntax:
        ``scalar = pc.retval()``


    Description:
        The return value of the function executed by the task gathered by the 
        last :hoc:meth:`ParallelContext.working` call.
        If the statement form of the submit is used then the return value 
        is the value of :hoc:data:`hoc_ac_` when the statement completes on the executing host.

         

----



.. hoc:method:: ParallelContext.pyret


    Syntax:
        ``python_object = pc.pyret()``


    Description:
        If a task is submitted defined as a Python callable then the return 
        value can be any Python object and can only be retrieved with pyret(). 
        This function can only be called once for the task result gathered 
        by the last :hoc:meth:`ParallelContext.working` call.

         

----



.. hoc:method:: ParallelContext.userid


    Syntax:
        ``scalar = pc.userid()``


    Description:
        The return value of the corresponding submit call. 
        The value of the userid is either the 
        first argument (if it was a non-negative integer) 
        of the submit call or else it is a positive integer unique only to 
        this ParallelContext. 
         
        See :hoc:meth:`ParallelContext.submit` with regard to retrieving the original
        arguments of the submit call corresponding to the working return. 
         
        Can be useful in organizing results according to an index defined during 
        submission. 
         

         

----



.. hoc:method:: ParallelContext.runworker


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
        is waiting for a result in a :hoc:meth:`ParallelContext.working` call.
        Many parallel processing bugs 
        are due to inconsistent context among hosts and those bugs 
        can be VERY subtle. Tasks should not change the context required 
        by other tasks without extreme caution. The only way I know how 
        to do this safely 
        is to store and retrieve a copy of 
        the authoritative context on the bulletin board. See 
        :hoc:meth:`ParallelContext.working` for further discussion in this regard.
         
        The runworker method is called automatically for each worker after 
        all files have been read in and executed --- i.e. if the user never 
        calls it explicitly from hoc. Otherwise the workers would exit since 
        the standard input is at the end of file for workers. 
        This is useful in those cases where 
        the only distinction between master and workers is that code 
        executed from the gui or console. 

         

----



.. hoc:method:: ParallelContext.done


    Syntax:
        ``pc.done()``


    Description:
        Sends the QUIT message to all worker hosts. Those NEURON processes then 
        exit. The master waits til all worker output has been transferred to 
        the master host. 

         

----



.. hoc:method:: ParallelContext.context


    Syntax:
        ``pc.context("statement\n")``

        ``pc.context("function_name", arg1, ...])``

        ``pc.context(object, "function_name", arg1, ...)``

        ``pc.context(userid, ..as above..)``

        ``pc.context(python_callable, arg1, ...)``


    Description:
        The arguments have the same semantics as those of the :hoc:meth:`ParallelContext.submit` method.
        The function or statement is executed on every worker host 
        but is not executed on the master. pc.context can only be 
        called by the master. The workers will execute the context statement 
        when they are idle or have completed their current task. 
        It probably only makes sense for the python_callable to return None. 
         
        There is no return in the 
        sense that :hoc:meth:`ParallelContext.working` does not return when one
        of these tasks completes. 
         
        This method was introduced with the following protocol in mind 

        .. code-block::
            none

            proc save_context() { // executed on master 
               sprint(tstr, "%s", this) 
               pc.look_take(tstr) // remove previous context if it exists 
               // master packs a possibly complicated context from within 
               // an object whose counterpart exists on all workers 
               pc.post(tstr) 
               pc.context(this, "restore_context", tstr) // all workers do this 
            } 
             
            proc restore_context() { 
               pc.look($s1) // don't remove! Others need it as well. 
               // worker unpacks possibly complicated context 
            } 


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



.. hoc:method:: ParallelContext.post


    Syntax:
        ``pc.post(key)``

        ``pc.post(key, ...)``


    Description:
        Post the message with the address key, (key may be a string or scalar), 
        and a body consisting of any number of :hoc:meth:`ParallelContext.pack` calls since
        the last post, and any number of arguments of type scalar, Vector, strdef 
        or Python object. 
         
        Later unpacking of the message body must be done in the same order as 
        this posting sequence. 

    .. seealso::
        :hoc:meth:`ParallelContext.pack`

         

----



.. hoc:method:: ParallelContext.take


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
        a variable. eg \ ``&x``. Unpacked Vectors will be resized to the 
        correct size of the vector item of the message. 
        To unpack Python objects, :hoc:func:`upkpyobj` must be used.

    .. seealso::
        :hoc:meth:`ParallelContext.upkstr`, :hoc:meth:`ParallelContext.upkscalar`,
        :hoc:meth:`ParallelContext.upkvec`, :hoc:meth:`ParallelContext.upkpyobj`

         

----



.. hoc:method:: ParallelContext.look


    Syntax:
        ``boolean = pc.look(key)``

        ``boolean = pc.look(key, ...)``


    Description:
        Like :hoc:meth:`ParallelContext.take` but does not block or remove message
        from bulletin board. Returns 1 if the key exists, 0 if the key does 
        not exist on the bulletin board. The message associated with the 
        key (if the key exists) is available for unpacking each time 
        pc.look returns 1. 

    .. seealso::
        :hoc:meth:`ParallelContext.look_take`, :hoc:meth:`ParallelContext.take`

         

----



.. hoc:method:: ParallelContext.look_take


    Syntax:
        ``boolean = pc.look_take(key, ...)``


    Description:
        Like :hoc:meth:`ParallelContext.take` but does not block. The message is
        removed from the bulletin board and two processes will never receive 
        this message. Returns 1 if the key exists, 0 if the key does not 
        exist on the bulletin board. If the key exists, the message can 
        be unpacked. 
         
        Note that a look followed by a take is *NOT* equivalent to look_take. 
        It can easily occur that another task might take the message between 
        the look and take and the latter will then block until some other 
        process posts a message with the same key. 

    .. seealso::
        :hoc:meth:`ParallelContext.take`, :hoc:meth:`ParallelContext.look`

         

----



.. hoc:method:: ParallelContext.pack


    Syntax:
        ``pc.pack(...)``


    Description:
        Append arguments consisting of scalars, Vectors, strdefs, 
        and pickleable Python objects into a message body 
        for a subsequent post. 

    .. seealso::
        :hoc:meth:`ParallelContext.post`

         

----



.. hoc:method:: ParallelContext.unpack


    Syntax:
        ``pc.unpack(...)``


    Description:
        Extract items from the last message retrieved with 
        take, look, or look_take. The type and sequence of items retrieved must 
        agree with the order in which the message was constructed with post 
        and pack. 
        Note that scalar items must be retrieved with pointer syntax as in 
        \ ``&soma.gnabar_hh(.3)`` 
        To unpack Python objects, :hoc:func:`upkpyobj` must be used.

    .. seealso::
        :hoc:meth:`ParallelContext.upkscalar`
        :hoc:meth:`ParallelContext.upkvec`, :hoc:meth:`ParallelContext.upkstr`
        :hoc:meth:`ParallelContext.upkpyobj`

         

----



.. hoc:method:: ParallelContext.upkscalar


    Syntax:
        ``x = pc.upkscalar()``


    Description:
        Return the scalar item which must be the next item in the unpacking 
        sequence of the message retrieved by the previous take, look, or look_take. 

         

----



.. hoc:method:: ParallelContext.upkstr


    Syntax:
        ``str = pc.upkstr(str)``


    Description:
        Copy the next item in the unpacking 
        sequence into str and return that strdef. 

         

----



.. hoc:method:: ParallelContext.upkvec


    Syntax:
        ``vec = pc.upkvec()``

        ``vec = pc.upkvec(vecsrc)``


    Description:
        Copy the next item in the unpacking 
        sequence into vecsrc (if that arg exists, it will be resized if necessary). 
        If the arg does not exist return a new Vector. 

         

----



.. hoc:method:: ParallelContext.upkpyobj


    Syntax:
        ``python_object = pc.upkpyobj()``


    Description:
        Return a reference to the (copied via pickling/unpickling) 
        Python object which must be the next item in the unpacking 
        sequence of the message retrieved by the previous take, look, or look_take. 

         

----



.. hoc:method:: ParallelContext.time


    Syntax:
        ``st = pc.time()``


    Description:
        Returns a high resolution elapsed wall clock time on the processor 
        (units of seconds) since an arbitrary time in the past. 
        Normal usage is 

        .. code-block::
            none

            st = pc.time 
            ... 
            print pc.time - st 


    .. warning::
        A wrapper for MPI_Wtime when MPI is used. When PVM is used, the return 
        value is :samp:`clock_t times(struct tms {*buf})/100`. 

         

----



.. hoc:method:: ParallelContext.wait_time


    Syntax:
        ``total = pc.wait_time()``


    Description:
        The amount of time (seconds) 
        on a worker spent waiting for a message from the master. For the master, 
        it is the amount of time in the pc.take calls that was spent waiting. 
         
        To determine the time spent exchanging spikes during a simulation, use 
        the idiom: 

        .. code-block::
            none

            wait = pc.wait_time() 
            pc.solve(tstop) 
            wait = pc.wait_time() - wait - pc.step_wait()


         

----



.. hoc:method:: ParallelContext.step_time


    Syntax:
        ``total = pc.step_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent integrating equations, checking thresholds, and delivering 
        events. It is essentially pc.integ_time + pc.event_time. 
        It does not include gap junction voltage transfer time or multisplit
        communication time.
         

----


.. hoc:method:: ParallelContext.step_wait


    Syntax:
        ``total = pc.step_wait()``

        ``0 = pc.step_wait(-1)``


    Description:
        The barrier time (seconds) between the end of a step and the
        beginning of spike exchange. Note that pc.wait_time() includes
        this barrier time. step_wait is useful in calculating a more
        accurate load balance (properly reduced by dynamic load imbalance)
        and a better statistic for spike exchange communication time.
         
        The barrier overhead during a simulation can be turned off with
        pc.step_wait(-1) in which case the time spent in
        allgather spike exchange will include that barrier time. In this case
        pc.step_wait() will return 0.0 . 

        Prior to the existence of this function, load balance was generally
        computed as (average step_time / maximum step_time). That is accurate
        to the extent that each individual step on a given process takes
        constant time but fails to the extent that there is significant
        dynamic variation in a dt step on a given process during a run (e.g
        a lot of variation in the number of spikes delivered per time step).

        A better evaluation of load balance (accounting for dynamic load
        imbalance as well as static load imbalance) is
	(average step_time / maximum (step_time + step_wait))

        Note that if static load imbalance dominates the load imbalance,
        then one expects the minimum step_wait to be close to 0.


----



.. hoc:method:: ParallelContext.send_time


    Syntax:
        ``total = pc.send_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent directing source gid spikes arriving on the target gid 
        to the proper PreSyn. 

         

----



.. hoc:method:: ParallelContext.event_time


    Syntax:
        ``total = pc.event_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent checking thresholds and delivering spikes. Note that 
        pc.event_time() + pc.send_time() will include all spike related time but 
        NOT the time spent exchanging spikes between cpus. 
        (Currently only for fixed step) 

         

----



.. hoc:method:: ParallelContext.integ_time


    Syntax:
        ``total = pc.integ_time()``


    Description:
        The amount of time (seconds) 
        on a cpu spent integrating equations. (currently only for fixed step) 

         

----



.. hoc:method:: ParallelContext.vtransfer_time


    Syntax:
        ``transfer_exchange_time = pc.vtransfer_time()``

        ``splitcell_exchange_time = pc.vtransfer_time(1)``

        ``reducedtree_computation_time = pc.vtransfer_time(2)``


    Description:
        The amount of time (seconds) 
        spent transferring and waiting for voltages or matrix elements. 
        The :hoc:func:`integ_time` is reduced by transfer and splitcell exchange times.
         
        splitcell_exchange_time includes the reducedtree_computation_time. 
         
        reducedtree_computation_time refers to the extra time used by the 
        :hoc:meth:`ParallelContext.multisplit` backbone_style 1 and 2 methods between
        send and receive of matrix information. This amount is also included 
        in the splitcell_exchange_time. 

         

----



.. hoc:method:: ParallelContext.mech_time


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
        :hoc:meth:`MechanismType.internal_type`


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
    Communication between normal hoc code executing on the master NEURON 
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




.. _hoc_ParallelContext_MPI:

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



.. hoc:method:: ParallelContext.barrier


    Syntax:
        ``waittime = pc.barrier()``


    Description:
        Does an MPI_Barrier and returns the wait time at the barrier.  Execution 
        resumes only after all process reach this statement. 

         

----



.. hoc:method:: ParallelContext.allreduce


    Syntax:
        ``result = pc.allreduce(value, type)``

        ``pc.allreduce(src_dest_vector, type)``


    Description:
        Type is 1, 2, or 3 and the every host gets a 
        result as sum over all value, maximum 
        value, or minimum value respectively 
         
        If the first arg is a Vector the reduce is done element-wise. ie 
        min of each rank's v.x[0] returned in each rank's v.x[0], etc. Note that 
        each vector must have the same size. 

         

----



.. hoc:method:: ParallelContext.allgather


    Syntax:
        ``pc.allgather(value, result_vector)``


    Description:
        Every host gets the value from every other host. The value from a host id 
        is in the id'th element of the vector. The vector is resized to size 
        pc.nhost. 

         

----



.. hoc:method:: ParallelContext.alltoall


    Syntax:
        ``pc.alltoall(vsrc, vcnts, vdest)``


    Description:
        Analogous to MPI_Alltoallv(...). vcnts must be of size pc.nhost and 
        vcnts.sum must equal the size of vsrc. 
        For host i, vcnts.x[j] elements of 
        vsrc are sent to host j beginning at the index vcnts.sum(0,j-1). 
        On host j, those elements are put into vdest beginning at the location 
        after the elements received from hosts 0 to i-1. 
        The vdest is resized to the number of elements received. 
        Note that vcnts are generally different for different hosts. If you need 
        to know how many came from what host, use the idiom 
        \ ``pc.alltoall(vcnts, one, vdest)`` where one is a vector filled with 1. 

        .. code-block::
            none

            // assume vsrc is a sorted Vector with elements ranging from 0 to tstop 
            // then the following is a parallel sort such that vdest is sorted on 
            // host i and for i < j, all the elements of vdest on host i are < 
            // than all the elements on host j. 
            vsrc.sort 
            cnts = new Vector(pc.nhost) 
            j = 0 
            for i=0, pc.nhost-1 { 
              x = (i+1)*tvl 
              k = 0 
              while (j < s.size) { 
                if (s.x[j] < x) { 
                  j += 1 
                  k += 1 
                }else{ 
                  break 
                } 
              } 
              cnts.x[i] = k 
            } 
            pc.alltoall(vsrc, cnts, vdest)  


         
----



.. hoc:method:: ParallelContext.py_alltoall


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
          pc = h.ParallelContext()
          nhost = int(pc.nhost())
          rank = int(pc.id())
          
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
          
          if rank == 0: print 'source data'
          for r in serialize(): print rank, data
          
          data = pc.py_alltoall(data)
          
          if rank == 0: print 'destination data'
          for r in serialize(): print rank, data
          
          pc.runworker()
          pc.done()
          h.quit()

        .. code-block::
          none

          $ mpiexec -n 4 nrniv -mpi -python test.py
          numprocs=4
          NEURON -- VERSION 7.3 (806:ba5e547c21f6) 2013-03-13
          Duke, Yale, and the BlueBrain Project -- Copyright 1984-2012
          See http://www.neuron.yale.edu/credits.html
          
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



.. hoc:method:: ParallelContext.broadcast


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



.. _hoc_subworld:

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
    network after executing :hoc:meth:`ParallelContext.runworker` would be to utilize
    the :hoc:meth:`ParallelContext.context` method. In particular, without subworlds,
    it is impossible to correctly submit bulletin board tasks, each of which 
    simulates a network specfied with the :ref:`hoc_ParallelNetwork`
    methods --- even if the network is complete on a single process. 
     
    The :hoc:meth:`ParallelContext.subworlds` method divides the world of processors into subworlds,
    each of which can execute a task that independently and assynchronously 
    creates and simulates (and destroys if the task networks are different) 
    a separate 
    network described using the :ref:`hoc_ParallelNetwork` and
    :ref:`hoc_ParallelTransfer` methods. The task, executing
    in the subworld can also make use of the :ref:`hoc_ParallelContext_MPI` collectives.
    Different subworlds can use the same global identifiers without 
    interference and the spike communication, transfers, and MPI collectives 
    are localized to within a subworld. I.e. in MPI terms, 
    each subworld utilizes a distinct MPI communicator. In a subworld, the 
    :hoc:meth:`ParallelContext.id` and :hoc:meth:`ParallelContext.nhost` refer to the rank and
    number of processors in the subworld. (Note that every subworld has 
    a :hoc:meth:`ParallelContext.id` == 0 rank processor.)
     
    Only the rank :hoc:meth:`ParallelContext.id` == 0 subworld processors communicate
    with the bulletin board. Of these processors, one (:hoc:meth:`~ParallelContext.id_world` == 0) is
    the master processor and the others are the workers. The master 
    submits tasks to the bulletin board (and executes a task if no results 
    are available) and the workers execute tasks and post the results 
    to the bulletin board. Remember, all the workers also have :hoc:meth:`ParallelContext.id`
    == 0 but different :hoc:meth:`~ParallelContext.id_world` and :hoc:meth:`~ParallelContext.id_bbs` ranks. The subworld
    :hoc:meth:`ParallelContext.id` ranks greater than 0 are not called workers --- their
    global rank is :hoc:meth:`~ParallelContext.id_world` but their bulletin board rank, :hoc:meth:`~ParallelContext.id_bbs` is -1.
    When a worker (or the master) receives a task to execute, the exact same 
    function with arguments that define the task will be executed on all the 
    processes of the subworld. A subworld is exactly analogous to the old 
    world of a network simulation in which processes distinguish themselves 
    by means of :hoc:meth:`ParallelContext.id` which is unique among
    the :hoc:meth:`ParallelContext.nhost` processes in the subworld.
     
    A runtime error will result if an :hoc:meth:`~ParallelContext.id_bbs` == -1 rank processor tries
    to communicate with the bulletin board, thus the general idiom for 
    a task posting or taking information from the bulletin board should be either 
    ``if (pc.id == 0) { ... }`` or ``if (pc.id_bbs != -1) { ... }``. 
    The latter is more general since the former would not be correct if 
    :hoc:meth:`~ParallelContext.subworlds` has NOT been called since in that case
    ``pc.id == pc.id_world == pc.id_bbs`` and 
    ``pc.nhost == pc.nhost_world == pc.nhost_bbs`` 
     

         

----



.. hoc:method:: ParallelContext.subworlds


    Syntax:
        ``pc.subworlds(subworld_size)``


    Description:
        Divides the world of all processors 
        into :hoc:func:`nhost_world` / subworld_size subworlds.
        Note that the total number of processes, nhost_world, should be 
        an integer multiple of subworld_size. 
        The most useful subworld sizes are 1 and :hoc:func:`nhost_world` .
        After return, for the processes 
        in each subworld, :hoc:meth:`ParallelContext.nhost` is equal to subworld_size
        and the :hoc:meth:`ParallelContext.id` is the rank of the process with respect
        to the subworld of which it is a part. 
         
        Each subworld has its own 
        unique MPI communicator for the :ref:`hoc_ParallelContext_MPI` functions such
        as :hoc:meth:`ParallelContext.barrier` and so those collectives do not affect other subworlds.
        All the :ref:`hoc_ParallelNetwork` notions are local to a subworld. I.e. independent
        networks using the same gids can be simulated simultaneously in 
        different subworlds. Only rank 0 of a subworld ( :hoc:meth:`ParallelContext.id`
        == 0) can use the bulletin board and has a non-negative :hoc:meth:`nhost_bbs`
        and :hoc:meth:`id_bbs` .
         
        Thus the bulletin board interacts with :hoc:func:`nhost_bbs` processes
        each with :hoc:meth:`ParallelContext.id` == 0. And each of those rank 0 processes
        interacts with :hoc:meth:`ParallelContext.nhost` processes using MPI commands
        isolated within each subworld. 
         
        Probably the most useful values of subworld_size are 1 and :hoc:func:`nhost_world`.
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
        call all processes call f. After the runworker call, only the master 
        process returns and calls f. The master submits 4 tasks and then enters 
        a while loop waiting for results and, when a result is ready, prints 
        the userid, argument, and return value of the task. 
         

        .. code-block::
            none

            objref pc 
            pc = new ParallelContext() 
            {pc.subworlds(3)} 
            func f() {local ret 
              ret = pc.id_world*100 + pc.id_bbs*10 + pc.id  
              printf( \ 
               "userid=%d arg=%d ret=%03d  world %d of %d  bbs %d of %d  net %d of %d\n", \  
               hoc_ac_, $1, ret, \ 
               pc.id_world, pc.nhost_world, pc.id_bbs, pc.nhost_bbs, pc.id, pc.nhost) 
              system("sleep 1") 
              return ret 
            } 
            hoc_ac_ = -1 
            if (pc.id_world == 0) { printf("before runworker\n") } 
            {f(1)} 
            {pc.runworker()} 
            {printf("\nafter runworker\n") f(2) } 
             
            {printf("\nbefore submit\n")} 
            for i=3, 6 { pc.submit("f", i) } 
            {printf("after submit\n")} 
             
            while((userid = pc.working()) != 0) { 
              arg = pc.upkscalar() 
              printf("result userid=%d arg=%d return=%03d\n", \ 
                userid, arg, pc.retval) 
            } 
             
            {printf("\nafter working\n") f(7) } 
            {pc.done()} 
            quit() 

         
        If the above code is saved in :file:`temp.hoc` and executed with 6 processes using 
        \ ``mpiexec -n 6 nrniv -mpi temp.hoc`` then the output will look like 
        (some lines may be out of order) 

        .. code-block::
            none

            $ mpiexec -n 6 nrniv -mpi temp.hoc 
            numprocs=6 
            NEURON -- VERSION 7.2 (454:bb5c4f755f59) 2010-07-30 
            Duke, Yale, and the BlueBrain Project -- Copyright 1984-2008 
            See http://www.neuron.yale.edu/credits.html 
             
            before runworker 
            userid=-1 arg=1 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3 
            userid=-1 arg=1 ret=192  world 2 of 6  bbs -1 of -1  net 2 of 3 
            userid=-1 arg=1 ret=492  world 5 of 6  bbs -1 of -1  net 2 of 3 
            userid=-1 arg=1 ret=391  world 4 of 6  bbs -1 of -1  net 1 of 3 
            userid=-1 arg=1 ret=091  world 1 of 6  bbs -1 of -1  net 1 of 3 
            userid=-1 arg=1 ret=310  world 3 of 6  bbs 1 of 2  net 0 of 3 
             
            after runworker 
            userid=-1 arg=2 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3 
             
            before submit 
            after submit 
            userid=21 arg=4 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3 
            userid=20 arg=3 ret=310  world 3 of 6  bbs 1 of 2  net 0 of 3 
            userid=20 arg=3 ret=391  world 4 of 6  bbs -1 of -1  net 1 of 3 
            userid=21 arg=4 ret=091  world 1 of 6  bbs -1 of -1  net 1 of 3 
            userid=21 arg=4 ret=192  world 2 of 6  bbs -1 of -1  net 2 of 3 
            userid=20 arg=3 ret=492  world 5 of 6  bbs -1 of -1  net 2 of 3 
            result userid=21 arg=4 return=000 
            userid=22 arg=5 ret=091  world 1 of 6  bbs -1 of -1  net 1 of 3 
            userid=22 arg=5 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3 
            userid=22 arg=5 ret=192  world 2 of 6  bbs -1 of -1  net 2 of 3 
            result userid=22 arg=5 return=000 
            userid=23 arg=6 ret=000  world 0 of 6  bbs 0 of 2  net 0 of 3 
            userid=23 arg=6 ret=192  world 2 of 6  bbs -1 of -1  net 2 of 3 
            userid=23 arg=6 ret=091  world 1 of 6  bbs -1 of -1  net 1 of 3 
            result userid=23 arg=6 return=000 
            result userid=20 arg=3 return=310 
             
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



.. hoc:method:: ParallelContext.nhost_world


    Syntax:
        ``numprocs = pc.nhost_world()``


    Description:
        Total number of processes in all subworlds. Equivalent to 
        :hoc:meth:`ParallelContext.nhost` when :hoc:func:`subworlds` has not been executed.

         

----



.. hoc:method:: ParallelContext.id_world


    Syntax:
        ``rank = pc.id_world()``


    Description:
        Global world rank of the process. This is unique among all processes 
        of all subworlds and ranges from 0 to :hoc:func:`nhost_world` - 1

         

----



.. hoc:method:: ParallelContext.nhost_bbs


    Syntax:
        ``numprocs = pc.nhost_bbs()``


    Description:
        If :hoc:func:`subworlds` has been called, nhost_bbs() returns the number of
        subworlds if :hoc:meth:`ParallelContext.id` == 0 and -1 for all other ranks in
        the subworld. 
        If subworlds has NOT been called then nhost_bbs, nhost_world, and nhost 
        are the same. 

         

----



.. hoc:method:: ParallelContext.id_bbs


    Syntax:
        ``rank = pc.id_bbs()``


    Description:
        If :hoc:func:`subworlds` has been called id_bbs() returns the subworld rank
        if :hoc:meth:`ParallelContext.id` == 0 and -1 for all other ranks in the
        subworld. 
        If subworlds has not been called then id_bbs, id_world, and id are the 
        same. 

         

----



.. _hoc_parallelnetwork:

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
    --with-paranrn). (See :ref:`hoc_ParallelContext_MPI` for installation hints).
     
    The fundamental requirement is that each 
    cell be associated with a unique integer global id (gid). The 
    :hoc:func:`ParallelNetManager` in nrn/share/lib/hoc/netparmpi.hoc is a sample
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
    the method name, :hoc:meth:`ParallelContext.cell`, that makes the correspondence
    as well as the accessor method, :hoc:meth:`ParallelContext.gid2cell`.
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
    cpus (:hoc:meth:`ParallelContext.splitcell` and :hoc:meth:`ParallelContext.multisplit`).
    But now, some pieces will not have a spike detection site and therefore 
    don't have to have a gid. In either case, it can be administratively 
    useful to invent an administrative policy for gid values that encodes 
    whole cell identification. For a cell piece that has no spike output, 
    one can still give it a gid associated with an arbitrary spike detection 
    site that is effectively turned off because it is not the source for 
    any existing NetCon and it was never specified as an 
    :hoc:meth:`ParallelContext.outputcell`. In the same way, it is also
    useful to encode a :hoc:meth:`ParallelContext.multisplit`
    sid (split id) with whole cell identification. 
     

.. warning::
    If mpi is 
    not available but NEURON has been built with PVM installed, an alternative 
    ParallelNetManager implementation with the identical interface is 
    available that makes use only of standard ParallelContext methods. 

         

----



.. hoc:method:: ParallelContext.set_gid2node


    Syntax:
        ``pc.set_gid2node(gid, id)``


    Description:
        If the id is equal to pc.id then this machine "owns" the gid and 
        the associated cell 
        should be eventually created only on this machine. 
        Note that id must be in the range 0 to pc.nhost-1. The global id (gid) 
        can be any unique integer >= 0 but generally ranges from 0 to ncell-1 where 
        ncell is the total number of real and artificial cells. 
         
        Commonly, a cell has only one spike detector location and hence we normally 
        identify a gid with a cell. However, 
        cell can have several distinct spike detection locations or spike 
        detector point processes and each must be 
        associated with a distinct gid. (e.g. dendro-dendritic synapses). 

    .. seealso::
        :hoc:meth:`ParallelContext.id`, :hoc:meth:`ParallelContext.nhost`

         

----



.. hoc:method:: ParallelContext.gid_exists


    Syntax:
        ``integer = pc.gid_exists(gid)``


    Description:
        Return 3 if the gid is owned by this machine and the gid is already 
        associated with an output cell in the sense that its spikes will be 
        sent to all other machines. (i.e. :hoc:meth:`ParallelContext.outputcell` has
        also been called with that gid or :hoc:meth:`ParallelContext.cell` has been
        called with a third arg of 1.) 
         
        Return 2 if the gid is owned by this machine and has been associated with 
        a NetCon source location via the :hoc:func:`cell` method.
         
        Return 1 if the gid is owned by this machine but has not been associated with 
        a NetCon source location. 
         
        Return 0 if the gid is NOT owned by this machine. 

         

----



.. hoc:method:: ParallelContext.threshold


    Syntax:
        ``th = pc.threshold(gid)``

        ``th = pc.threshold(gid, th)``


    Description:
        Return the threshold of the source variable determined by the first arg 
        of the :hoc:func:`NetCon` constructor which is used to associate the gid with a
        source variable via :hoc:func:`cell` . If the second arg is present the threshold
        detector is given that threshold. This method can only be called if the 
        gid is owned by this machine and :hoc:func:`cell` has been previously called.


----



.. hoc:method:: ParallelContext.cell


    Syntax:
        ``pc.cell(gid, netcon)``

        ``pc.cell(gid, netcon, 0)``


    Description:
        The cell which is the source of the :hoc:func:`NetCon` is associated with the global
        id. By default,(no third arg or third arg = 1) 
        the spikes generated by that cell will be sent to every other machine 
        (see :hoc:meth:`ParallelContext.outputcell`). A cell commonly has only one spike
        generation location, but, for example in the case of reciprocal 
        dendro-dendritic synapses, there is no reason why it cannot have several. 
        The NetCon source defines the spike generation location. 
        Note that it is an error if the gid does not exist on this machine. The 
        normal idiom is to use a NetCon returned by a call to the cell's 
        connect2target(nil, netcon) method or else, if the cell is an unwrapped 
        artificial cell, use a \ ``netcon = new NetCon(cell, nil)`` statement to 
        get a temporary netcon which can be destroyed after its use in the 
        pc.cell call. The weight and delay of this temporary netcon are 
        not relevant; they come into the picture with 
        :hoc:meth:`ParallelContext.gid_connect` .
         
        Note that cells which do not send spikes to other machines are not required 
        to call this and in fact do not need a gid. However the administrative 
        detail would be significantly more complicated due to the multiplication 
        of cases in regard to whether the source and target exist AND the source 
        is an outputcell. 

         

----



.. hoc:method:: ParallelContext.outputcell


    Syntax:
        ``pc.outputcell(gid)``


    Description:
        Spikes this cell generates are to be distributed to all the other machines. 
        Note that :hoc:meth:`ParallelContext.cell` needs to be called prior to this and this
        does not need to be called if the third arg of that was non-zero. 
        In principle there is no reason for a cell to even have a gid if it is not 
        an outputcell. However the separation between pc.cell and pc.outputcell 
        allows uniform administrative setup of the network to defer marking a cell 
        as an output cell until an actual machine spanning connection is made for 
        which the source is on this machine and the target is on another machine. 

         

----



.. hoc:method:: ParallelContext.spike_record


    Syntax:
        ``pc.spike_record(gid, spiketimevector, gidvector)``


    Description:
        This is a synonym for :hoc:meth:`NetCon.record` but obviates the requirement of
        creating a NetCon using information about the source cell that is 
        relatively more tedious to obtain. This can only be called on the source 
        cell's machine. Note that a prerequisite is a call 
        to :hoc:meth:`ParallelContext.cell` . A call to :hoc:meth:`ParallelContext.outputcell` is NOT
        a prerequisite. 

        If the gid arg is -1, then spikes from ALL output gids on this
        machine will be recorded.
         

----



.. hoc:method:: ParallelContext.gid_connect


    Syntax:
        ``netcon = pc.gid_connect(srcgid, target)``

        ``netcon = pc.gid_connect(srcgid, target, netcon)``


    Description:
        A virtual connection is made between the source cell global id (which 
        may or may not 
        be owned by this machine) and the target (a synapse or artificial cell object) 
        which EXISTS on this machine. A :hoc:class:`NetCon` object is returned and the
        full delay for the connection should be given to it (as well as the weight). 
         
        Note that if the srcgid is owned by this machine then :hoc:func:`cell` must be called
        earlier to make sure that the srcgid is associated with a NetCon source 
        location. 
         
        Note that if the srcgid is not owned by this machine, then this machines 
        target will only get spikes from the srcgid if the source gid's machine 
        had called :hoc:meth:`ParallelContext.outputcell` or the third arg of
        :hoc:meth:`ParallelContext.cell` was 1.
         
        If the third arg exists, it must be a NetCon object with target the same 
        as the second arg. The src of that NetCon will be replaced by srcgid and 
        that NetCon returned. The purpose is to re-establish a connection to 
        the original srcgid after a :hoc:meth:`ParallelContext.gid_clear` .

         

----



.. hoc:method:: ParallelContext.psolve


    Syntax:
        ``pc.psolve(tstop)``


    Description:
        This should be called on every machine to start something analogous to 
        cvode.solve(tstop). In fact, if the variable step method is invoked this 
        is exactly what will end up happening except the solve will be broken into 
        steps determined by the result of :hoc:meth:`ParallelContext.set_maxstep`.

         

----



.. hoc:method:: ParallelContext.timeout


    Syntax:
        ``oldtimeout = pc.timeout(seconds)``


    Description:
        During execution of :hoc:meth:`ParallelContext.psolve` ,
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



.. hoc:method:: ParallelContext.set_maxstep


    Syntax:
        ``local_minimum_delay = pc.set_maxstep(default_max_step)``


    Description:
        This should be called on every machine after all the NetCon delays have 
        been specified. It looks at all the delays on all the machines 
        associated with the netcons 
        created by the :hoc:meth:`ParallelContext.gid_connect` calls, ie the netcons
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



.. hoc:method:: ParallelContext.spike_compress

    Syntax:
        :samp:`nspike = pc.spike_compress({nspike}, {gid_compress}, {xchng_meth})`

    Description:
        If nspike > 0, selects an alternative implementation of spike exchange 
        that significantly compresses the buffers and can reduce interprocessor 
        spike exchange time by a factor of 10. This works only with the 
        fixed step methods. The optional second argument is 1 by default and 
        works only if the number of cells on each cpu is less than 256. 
        Nspike refers to the number of (spiketime, gid) pairs that fit into the 
        fixed buffer that is exchanged every :hoc:func:`set_maxstep` integration interval.
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
        :hoc:meth:`CVode.queue_mode`

         

----



.. hoc:method:: ParallelContext.gid2obj


    Syntax:
        ``object = pc.gid2obj(gid)``


    Description:
        The cell or artificial cell object is returned that is associated with the 
        global id. Note that the gid must be owned by this machine. If the gid is 
        associated with a POINT_PROCESS that is located in a section which in turn 
        is inside an object, this method returns the POINT_PROCESS object. 

    .. seealso::
        :hoc:meth:`ParallelContext.gid_exists`, :hoc:meth:`ParallelContext.gid2cell`

    .. warning::
        Note that if a cell has several spike detection sources with different 
        gids, this is the method to use to return the POINT_PROCESS object itself. 

         

----



.. hoc:method:: ParallelContext.gid2cell


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
        :hoc:meth:`ParallelContext.gid_exists`, :hoc:meth:`ParallelContext.gid2obj`

    .. warning::
        Note that if a cell has several spike detection sources with different 
        gids, there is no way to distinguish them with this method. With those gid 
        arguments, gid2cell would 
        return the same cell where they are located. 

         

----



.. hoc:method:: ParallelContext.spike_statistics


    Syntax:
        ``nsendmax = pc.spike_statistics(&nsend, &nrecv, &nrecv_useful)``


    Description:
        Returns the spanning spike statistics since the last :hoc:func:`finitialize` . All arguments
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
        :hoc:meth:`ParallelContext.wait_time`, :hoc:meth:`ParallelContext.set_maxstep`

         

----



.. hoc:method:: ParallelContext.max_histogram


    Syntax:
        ``pc.max_histogram(vec)``


    Description:
        The vector, vec, of size maxspikes, is used to accumulate histogram information about the 
        maximum number of spikes sent by any cpu during the spike exchange process. 
        Every spike exchange, vec.x[max_spikes_sent_by_any_host] is incremented by 1. 
        It only makes sense to do this on one cpu, normally pc.id == 0. 
        If some host sends more than maxspikes at the end of an
        integration interval, no element of vec is incremented.
         
        Note that the current implementation of the spike exchange mechanism uses 
        MPI_Allgather with a fixed buffer size that allows up to nrn_spikebuf_size 
        spikes per cpu to be sent to all other machines. The default value of this 
        is 0. If some cpu needs to send more than this number of spikes, then 
        a second MPI_Allgatherv is used to send the overflow. 

         

----




.. _hoc_paralleltransfer:

Parallel Transfer
~~~~~~~~~~~~~~~~~


    Description:
        Extends the :ref:`hoc_ParallelContext_MPI` :ref:`hoc_ParallelNetwork` methods to allow parallel simulation
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
        with each on a different processor. See :hoc:meth:`ParallelContext.splitcell`.
        Splitting a cell into more than two pieces can be done with 
        :hoc:meth:`ParallelContext.multisplit` .
         
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



.. hoc:method:: ParallelContext.source_var


    Syntax:
        ``section pc.source_var(&v(x), source_global_index)``


    Description:
        Associates the source voltage variable with an integer. This integer has nothing 
        to do with and does not conflict with the discrete event gid used by the 
        :ref:`hoc_ParallelNetwork` methods.
        Must and can only be executed on the machine where the source voltage 
        exists. If extracellular is inserted at this location the voltage
        transferred is section.v(x) + section.vext[0](x) . I.e. the internal
        potential (appropriate for gap junctions).


    .. warning::
        An error will be generated if the the first arg pointer is not a
        voltage in the currently accessed section. This was not an error prior
        to version 1096:294dac40175f trunk 19 May 2014

----



.. hoc:method:: ParallelContext.target_var


    Syntax:
        ``pc.target_var(&target_variable, source_global_index)``

        ``pc.target_var(targetPointProcess, &target_variable, source_global_index)``


    Description:
        Values for the source_variable associated with the source_global_index will 
        be copied to the target_variable every time step (more often for the 
        variable step methods). 
         
        Transfer occurs during :hoc:func:`finitialize` just prior to BEFORE BREAKPOINT blocks
        of mod files and calls to type 0 :hoc:func:`FInitializeHandler` statements. For the
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



.. hoc:method:: ParallelContext.setup_transfer


    Syntax:
        ``pc.setup_transfer()``


    Description:
        This method must be called after all the calls to :hoc:func:`source_var` and
        :hoc:func:`target_var` and before initializing the simulation. It sets up the
        internal maps needed for both intra- and inter-processor 
        transfer of source variable values to target variables. 

         

----



.. hoc:method:: ParallelContext.splitcell


    Syntax:
        ``rootsection pc.splitcell_connect(host_with_other_subtree)``


    Description:
        The root of the subtree specified by the currently accessed section 
        is connected to the root of the 
        corresponding subtree located on the 
        host indicated by the argument. The method is very restrictive but 
        is adequate to solve the load balance problem. 
        The host_with_other_subtree must be either pc.id + 1 or pc.id - 1 
        and there can be only one split cell between hosts i and i+1. 
        A rootsection is defined as a section in which 
        :hoc:meth:`SectionRef.has_parent` returns 0.
         
        This method is not normally called by the user but 
        is wrapped by the :hoc:func:`ParallelNetManager` method,
        :hoc:meth:`ParallelNetManager.splitcell` which provides a simple interface to
        support load balanced network simulations. 
         
        See :hoc:meth:`ParallelContext.multisplit` for less restrictive
        parallel simulation of individual cells. 

    .. warning::
        Implemented only for fixed step methods. Cannot presently 
        be used with variable step 
        methods, or models with :hoc:func:`LinearMechanism`, or :hoc:func:`extracellular` .

         

----



.. hoc:method:: ParallelContext.multisplit


    Syntax:
        ``section pc.multisplit(x, sid)``

        ``section pc.multisplit(x, sid, backbone_style)``

        ``pc.multisplit()``


    Description:
        For parallel simulation of single cells. Generalizes 
        :hoc:meth:`ParallelContext.splitcell` in a number of ways.
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
        the standard :hoc:func:`connect` statement.
         
        If all the trees connected into a single cell have only one 
        sid, the simulation is numerically identical to :hoc:meth:`ParallelContext.splitcell`
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
        methods, or models with :hoc:func:`LinearMechanism`, or :hoc:func:`extracellular` .

         

----



.. hoc:method:: ParallelContext.gid_clear


    Syntax:
        ``pc.gid_clear()``

        ``pc.gid_clear(type)``


    Description:
        With type = 1 
        erases the internal lists pertaining to gid information and cleans 
        up all the internal references to those gids. This allows one 
        to start over with new :hoc:func:`set_gid2node` calls. Note that NetCon and cell
        objects would have to be dereferenced separately under user control. 
         
        With type = 2 clears any information setup by :hoc:meth:`ParallelContext.splitcell` or
        :hoc:meth:`ParallelContext.multisplit`.
         
        With type = 3 clears any information setup by :hoc:meth:`ParallelContext.setup_transfer`.
         
        With a type arg of 0 or no arg, clears all the above information. 

         

----



.. hoc:method:: ParallelContext.Threads


    Description:
        Extends ParallelContext to allow parallel multicore simulations using 
        threads. 
        The methods in this section are only available in the multicore version of NEURON. 
         
        Multiple threads can only be used with fixed step or global variable time step integration methods.
	Also, they cannot be used with :hoc:func:`extracellular`, :hoc:func:`LinearMechanism`,
	or reaction-diffusion models using rxd.
         
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
        as thread specific variables. Hoc assigns and evaluates only 
        the thread 0 version and if FUNCTIONs and PROCEDUREs are called from 
        Hoc, the thread 0 version of these globals are used. 


----



.. hoc:method:: ParallelContext.nthread


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



.. hoc:method:: ParallelContext.partition


    Syntax:
        ``pc.partition(i, seclist)``

        ``pc.partition()``


    Description:
        The seclist is a :hoc:func:`SectionList` which contains the root sections of cells
        (or cell pieces, see :hoc:func:`multisplit`) which should be simulated by the thread
        indicated by the first arg index. Either all or no thread can have 
        an associated seclist. The no arg form of pc.partition() unrefs the seclist 
        for all the threads. 


----



.. hoc:method:: ParallelContext.thread_stat


    Syntax:
        ``pc.thread_stat()``


    Description:
        For developer use. Does not do anything in distributed versions. 


----



.. hoc:method:: ParallelContext.thread_busywait


    Syntax:
        ``previous = pc.thread_busywait(next)``


    Description:
        When next is 1, during a :hoc:func:`psolve` run, overhead for pthread condition waiting
        is avoided by having threads watch continuously for a procedure to execute. 
        This works only if the number of threads is less than the number of cores 
        and uses 100% cpu time even when waiting. 


----



.. hoc:method:: ParallelContext.thread_how_many_proc


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



.. hoc:method:: ParallelContext.sec_in_thread


    Syntax:
        ``sec  i = pc.sec_in_thread()``


    Description:
        The currently accessed section resides in the thread indicated by the 
        return value. 


----



.. hoc:method:: ParallelContext.thread_ctime


    Syntax:
        ``ct = pc.thread_ctime(i)``

        ``pc.thread_ctime()``


    Description:
        The high resolution walltime time in seconds the indicated thread 
        used during time step integration. Note that this does not include 
        reduced tree computation time used by thread 0 when :hoc:func:`multisplit` is
        active. 

         
----

.. hoc:method:: ParallelContext.t

    Syntax:
        ``t = pc.t(tid)``

    Description:
        Return the current time of the tid'th thread

----

.. hoc:method:: ParallelContext.dt

    Syntax:
        ``dt = pc.dt(tid)``

    Description:
        Return the current timestep value for the tid'th thread


----

.. hoc:method:: ParallelContext.prcellstate

    Syntax:
        ``pc.precellstate(gid, "suffix")``

    Description:
        Creates the file <gid>_suffix.nrndat with all the range variable
        values and synapse/NetCon information associated with the gid.
        More complete than the HOC version of prcellstate.hoc in the standard
        library but more terse in regard to names of variables. The purpose
        is for diagnosing the reason why a spike raster for a simulation is
        not the same for different nhost or gid distribution. One examines
        the diff between corresponding files from different runs.

        The format of the file is:

        .. code-block::
            none

            gid
            t
            celsius
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

..  :hoc:method:: ParallelContext.nrnbbcore_write

    Syntax:
        ``pc.nrnbbcore_write([path[, gidgroup_vec]])``

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

        If the second argument does not exist, 
        rank 0 writes a "files.dat" file with a first value that
	specifies the total number of gidgroups and one gidgroup value per
        line for all threads of all ranks.

        If the model is too large to exist in NEURON (models typcially use
        an order of magnitude less memory in CoreNEURON) the model can
	be constructed in NEURON as a series of submodels.
        When one piece is constructed
        on each rank, this function can be called with a second argument which
        must be a Vector. In this case, rank 0 will NOT write a files.dat
        and instead the pc.nthread() gidgroup values for the rank will be
        returned in the Vector. 

        Multisplit is not supported.
        The model cannot be more complicated than a spike or gap
        junction coupled parallel network model of real and artificial cells.
        Real cells must have gids, Artificial cells without gids connect
        only to cells in the same thread. No POINTER to data outside of the
        thread that holds the pointer. 
