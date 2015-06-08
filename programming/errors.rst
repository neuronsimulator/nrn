Error Handling
--------------

.. function:: coredump_on_error

    Syntax:
        ``coredump_on_error(1 or 0)``

    Description:
        On unix machines, sets a flag which requests (1) a coredump in case 
        of memory or bus errors. 


----

.. function:: nrn_feenableexcept

    Syntax:
        ``nrn_feenableexcept(boolean)``

    Description:
        Sets or turns off a flag which, if on, causes a SIGFPE when a floating error occurs which consist of
        divide by zero, overflow, or invalid result. Known to work on linux. Turning on the flag is very helpful
        in finding the code location at which a variable is assigned a value of NaN or Inf. For a serial model, this
        is most easily done when running under gdb. For a parallel model, one can combine with coredump_on_error
        and, to force a coredump on abort(), use the bash command 'ulimit -c unlimited'.

----

.. function:: show_errmess_always

    Syntax:
        ``show_errmess_always(boolean)``

    Description:
        Sets or turns off a flag which, if on, always prints the error message even 
        if normally turned off by an :func:`execute1` statement or other call to the 
        interpreter. 


----

.. function:: execerror

    Syntax:
        ``execerror("message1", "message2")``

    Description:
        Raise an error and print the messages along with an interpreter stack
        trace. If there are no arguments, then nothing is printed.

