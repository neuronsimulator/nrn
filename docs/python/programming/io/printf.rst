.. _printf_doc:


Printf (Formatted Output)
-------------------------

.. function:: printf

    .. warning::

        For code written in Python, it is generally more practical to use Python string
        formatting (e.g., f-strings) and file IO.

    Name:
        printf, fprint, sprint --- formatted output 
         

    Syntax:
        ``h.printf(format, ...)``

        ``h.fprint(format, ...)``

        ``h.sprint(strdef, format, ...)``



    Description:
        ``h.printf`` places output on the standard output.  ``h.fprint`` places output 
        on the file opened with the ``h.wopen(filename)`` command (standard 
        output if no file is opened).  ``h.sprint`` places output in its ``strdef`` 
        argument. (Note: ``strdef`` must be a NEURON string reference, created via
        e.g., ``mystr = n.ref("")``, not a regular Python string as the latter is immutable.)
        These functions are subsets of their counterparts in 
        the C standard library. 
         
        Each of these functions converts, formats, and prints its arguments after 
        the *format* string under control of the *format* string. 
         
        The *format* string contains two types of objects: plain characters which 
        are simply printed, and conversion specifications 
        each of which causes conversion and printing of the next 
        successive argument. 
         
        Each conversion specification is introduced by the character '\ ``%``\ '
        and ends with a conversion type specifier.  The type specifiers 
        supported are: 

        .. list-table::
            :header-rows: 1

            * - Format
            - Description
            * - ``f``
            - Signed value of the form ``-dddd.ddddd``.
            * - ``e``
            - Signed value of the form ``-d.dddddde-nn``.
            * - ``g``
            - Signed value in either ``e`` or ``f`` form based on given value and precision. Trailing zeros and the decimal point are printed only if necessary.
            * - ``d``
            - Signed value truncated and printed as integer.
            * - ``o``
            - Printed as unsigned octal integer.
            * - ``x``
            - Printed as unsigned hexadecimal integer.
            * - ``c``
            - Number treated as ASCII code and printed as single character.
            * - ``s``
            - String is printed, argument must be a string.

         
        Between ``%`` and the conversion type, optional flags, width, precision 
        and size specifiers can be placed.  The most useful flag is '-' which 
        left justifies the result, otherwise the number is right justified in its 
        field. Width and precision specifiers are of the form ``width.precis``. 
         
        Special characters of note are: 
         
        .. list-table:: Escape Characters
            :header-rows: 1

            * - Escape Sequence
            - Description
            * - ``\n``
            - newline
            * - ``\t``
            - tab
            * - ``\r``
            - carriage return without the line feed

         
        ``h.printf`` and ``h.fprint`` return the number of characters printed. 
         

    Example:

        .. code::

            h.printf("\tpi=%-20.10g sin(pi)=%f\n", n.PI, h.sin(n.PI)) 

                    pi=3.141592654          sin(pi)=0.000000 
                    42 

         
    Pure Python almost equivalent example:

        .. code::

            print(f'\tpi={n.PI:<20.10g} sin(pi)={h.sin(n.PI):f}')

        .. note::

            This is not an identical replacement because it does not return the number of characters;
            the return is always ``None``.


    .. seealso::
        :meth:`File.ropen`
        

    .. warning::
        Only a subset of the C standard library functions. 
         

----


Redirect Standard Out
---------------------

.. function:: hoc_stdout


    Syntax:
        :samp:`h.hoc_stdout("{filename}")`

        ``h.hoc_stdout()``


    Description:
        With a filename argument, switches the original standard out to filename. 
        With no arguments. switches current standard out back to original filename. 
         
        Only one level of switching allowed. Switching back to original causes 
        future output to append to the stdout. Switching to "filename" writes 
        stdout from the beginning of the file. 

    Example:

        .. code::

            from neuron import n

            def p():
                print('one') # to original standard out
                h.hoc_stdout('temp.tmp')
                print('two') # to temp.tmp
                for sec in n.allsec():
                    n.psection(sec=sec) # to temp.tmp
                h.hoc_stdout()
                print('three') # to the original standard out

            p() 

    .. note::

        Despite the misleading name, this redirects standard out from both Python and HOC.
