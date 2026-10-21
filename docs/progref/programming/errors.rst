Error Handling
--------------

.. function:: coredump_on_error

    .. tab:: Python
    
        Syntax:
            ``n.coredump_on_error(True or False)``

        Description:
            On unix machines, sets a flag which requests (True) a coredump in case 
            of memory or bus errors --- Or floating exceptions if
            :func:`nrn_feenableexcept` has been turned on. 1 or 0 may be used as synonyms
            for True and False, respectively.

    .. tab:: HOC


        Syntax:
            ``coredump_on_error(1 or 0)``
        
        
        Description:
            On unix machines, sets a flag which requests (1) a coredump in case 
            of memory or bus errors --- Or floating exceptions if
            :func:`nrn_feenableexcept` has been turned on.
        
----

.. function:: nrn_feenableexcept

    .. tab:: Python
    
        Syntax:
            ``old = n.nrn_feenableexcept(1)``

            ``n.nrn_feenableexcept(old)``

            ``n.nrn_feenableexcept(0)``

            ``n.nrn_feenableexcept()``

        Description:
            Arm or mask hardware traps for an invalid operation, divide by
            zero, and overflow. Underflow and inexact stay masked. The
            setting applies to the current thread. The argument should be 0 or
            1. 0 disables floating exception interrupts and 1 enables them.

            The return value is the previous state, 0 or 1, so it can be
            saved and restored. A failure to change the traps raises a
            NEURON error.

            When a trap fires, NEURON prints one of

            ``Floating exception: Divide by zero``

            ``Floating exception: Invalid (no well defined result)``

            ``Floating exception: Overflow``

            then ``Floating point exception`` and a backtrace when one is
            available.

            On Linux and macOS, a trap inside HOC error recovery becomes a
            NEURON error. From Python that is a ``RuntimeError`` after the
            report has been printed, and the traps stay armed. With no HOC
            recovery on the stack, or after :func:`coredump_on_error`, the
            process aborts after the report. By default, a parallel run aborts
            when the trap becomes a NEURON error. On Apple silicon the trap
            arrives as ``SIGILL``.
            On Linux and on Intel Mac it arrives as ``SIGFPE``. lldb on
            Apple silicon stops on the Mach exception
            ``EXC_BAD_INSTRUCTION`` before that signal is delivered.

            On Windows the same report is printed and the process aborts.
            The trap does not become a Python exception.

            HOC ``print(1/0)`` is rejected by the interpreter and is not one
            of these traps. A hardware overflow is an expression such as
            ``1e308*1e308``. Some math libraries, including the MinGW library
            in the Windows installer, check ``sqrt`` and ``log`` in software
            and do not raise the hardware trap for those calls.

            Arming the traps is a way to find the assignment that first
            produces a NaN or an Inf. Run under a debugger to stop at the
            trap. The installation debug notes give the lldb command for
            Apple silicon.
        Note:
            While these traps are off, ``exp(x)`` for ``x > 700`` in mod
            files warns and returns ``exp(700)``. While they are on, that
            clamp is skipped and an out-of-range ``exp`` can raise the
            overflow trap.

    .. tab:: HOC


        Syntax:
            ``old = nrn_feenableexcept(1)``

            ``nrn_feenableexcept(old)``

            ``nrn_feenableexcept(0)``

            ``nrn_feenableexcept()``

        Description:
            Arm or mask hardware traps for an invalid operation, divide by
            zero, and overflow. Underflow and inexact stay masked. The
            setting applies to the current thread. The argument should be 0 or
            1. 0 disables floating exception interrupts and 1 enables them.

            The return value is the previous state, 0 or 1, so it can be
            saved and restored. A failure to change the traps raises an
            interpreter error.

            When a trap fires, NEURON prints one of

            ``Floating exception: Divide by zero``

            ``Floating exception: Invalid (no well defined result)``

            ``Floating exception: Overflow``

            then ``Floating point exception`` and a backtrace when one is
            available.

            On Linux and macOS, a trap inside the interpreter's error
            recovery raises an error after the report, and the traps stay
            armed. With no recovery on the stack, or after
            :func:`coredump_on_error`, the process aborts after the report.
            By default, a parallel run aborts when the trap becomes a NEURON
            error. On Apple silicon the trap arrives as ``SIGILL``. On Linux
            and on Intel Mac it
            arrives as ``SIGFPE``. lldb on Apple silicon stops on the Mach
            exception ``EXC_BAD_INSTRUCTION`` before that signal is
            delivered.

            On Windows the same report is printed and the process aborts.

            ``print(1/0)`` is rejected by the interpreter and is not one of
            these traps. A hardware overflow is an expression such as
            ``1e308*1e308``. Some math libraries, including the MinGW library
            in the Windows installer, check ``sqrt`` and ``log`` in software
            and do not raise the hardware trap for those calls.

            Arming the traps is a way to find the assignment that first
            produces a NaN or an Inf.
        Note:
            While these traps are off, ``exp(x)`` for ``x > 700`` in mod
            files warns and returns ``exp(700)``. While they are on, that
            clamp is skipped and an out-of-range ``exp`` can raise the
            overflow trap.
        
----

.. function:: show_errmess_always

    .. tab:: Python
    
        Syntax:
            ``n.show_errmess_always(boolean)``

        Description:
            Sets or turns off a flag which, if on, always prints the error message even 
            if normally turned off by an :func:`execute1` statement or other call to the 
            interpreter. 


    .. tab:: HOC


        Syntax:
            ``show_errmess_always(boolean)``
        
        
        Description:
            Sets or turns off a flag which, if on, always prints the error message even 
            if normally turned off by an :func:`execute1` statement or other call to the
            interpreter. 
        
----

.. function:: execerror

    .. tab:: Python
    
        Syntax:
            ``n.execerror("message1", "message2")``

        Description:
            Raise an error and print the messages along with a NEURON interpreter stack
            trace. If there are no arguments, then nothing is printed.

            The error may be caught in Python as a :class:`RuntimeError` exception,
            however this happens after NEURON prints out its internal stack trace.

            For example, consider the code:

            .. code::
                python

                from neuron import n
                try:
                    n.execerror("Unable to initialize model", "not enough parameters")
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




    .. tab:: HOC


        Syntax:
            ``execerror("message1", "message2")``
        
        
        Description:
            Raise an error and print the messages along with an interpreter stack
            trace. If there are no arguments, then nothing is printed.
        
