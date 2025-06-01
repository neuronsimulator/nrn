Error Handling
--------------

.. function:: coredump_on_error

    Syntax:
        ``h.coredump_on_error(True or False)``

    Description:
        On unix machines, sets a flag which requests (True) a coredump in case 
        of memory or bus errors --- Or floating exceptions if
        :func:`nrn_fenableexcept` has been turned on. 1 or 0 may be used as synonyms
        for True and False, respectively.

----

.. function:: nrn_feenableexcept

    Syntax:
        ``previous_floating_point_mask = h.nrn_feenableexcept(boolean)``

    Description:
        Sets or turns off a flag which, if on, causes a SIGFPE when a floating error occurs which consist of
        divide by zero, overflow, or invalid result. Known to work on linux. Turning on the flag is very helpful
        in finding the code location at which a variable is assigned a value of NaN or Inf. For a serial model, this
        is most easily done when running under gdb (or lldb on macOS). For a parallel model, one can combine with coredump_on_error
        and, to force a coredump on abort(), use the bash command 'ulimit -c unlimited'.

        Return is the previous value of the floating-point mask. (or -1
        on failure to set the floating-point flags; or -2 if feenableexcept
        does not exist).

        Without an arg, SIGFPE is turned on.
    Note:
        The normal trap for exp(x) for x > 700 in mod files becomes
        a floating exception when x is out of range.

----

.. function:: show_errmess_always

    Syntax:
        ``h.show_errmess_always(boolean)``

    Description:
        Sets or turns off a flag which, if on, always prints the error message even 
        if normally turned off by an :func:`execute1` statement or other call to the 
        interpreter. 


----

.. function:: execerror

    Syntax:
        ``h.execerror("message1", "message2")``

    Description:
        Raise an error and print the messages along with a NEURON interpreter stack
        trace. If there are no arguments, then nothing is printed.

        The error may be caught in Python as a :class:`RuntimeError` exception,
        however this happens after NEURON prints out its internal stack trace.

        For example, consider the code:

        .. code::
            python

            from neuron import h
            try:
                h.execerror("Unable to initialize model", "not enough parameters")
            except RuntimeError as e:
                print(f"Caught an error: {e}")
        
        Running this displays:

        .. code::

            NEURON: Unable to initialize model not enough parameters
             near line 0
             objref hoc_obj_[2]
                               ^
                    execerror("Unable to ...", "not enough...")
            Caught an error: hocobj_call error: hoc_execerror: Unable to initialize model not enough parameters
        
        For pure Python models, consider if using Python's ``raise`` keyword will work instead.




