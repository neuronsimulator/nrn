Error Handling
--------------

.. function:: coredump_on_error

    Syntax:
        ``h.coredump_on_error(1 or 0)``

    Description:
        On unix machines, sets a flag which requests (1) a coredump in case 
        of memory or bus errors. 


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
        Raise an error and print the messages along with an interpreter stack
        trace. If there are no arguments, then nothing is printed.

