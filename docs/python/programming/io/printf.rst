.. _printf_doc:


Printf (Formatted Output)
-------------------------

.. function:: printf

    .. warning::

        For code written in Python, it is generally more practical to use Python string
        formatting and file IO.

    Name:
        printf, fprint, sprint --- formatted output 
         

    Syntax:
        ``h.printf(format, ...)``

        ``h.fprint(format, ...)``

        ``h.sprint(string, format, ...)``



    Description:
        ``h.printf`` places output on the standard output.  ``h.fprint`` places output 
        on the file opened with the ``h.wopen(filename)`` command (standard 
        output if no file is opened).  ``h.sprint`` places output in its *string* 
        argument.  These functions are subsets of their counterparts in 
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
         


        f 
            signed value of the form -dddd.ddddd 

        e 
            signed value of the form -d.dddddde-nn 

        g 
            signed value in either 'e' or 'f' form based on given value 
            and precision.  Trailing zeros and the decimal point are printed 
            only if necessary. 

        d 
            signed value truncated and printed as integer. 

        o 
            printed as unsigned octal integer. 

        x 
            printed as unsigned hexadecimal integer 

        c 
            number treated as ascii code and printed as single character 

        s 
            string is printed, arg must be a string. 

         
        Between ``%`` and the conversion type, optional flags, width, precision 
        and size specifiers can be placed.  The most useful flag is '-' which 
        left justifies the result, otherwise the number is right justified in its 
        field. Width and precision specifiers are of the form ``width.precis``. 
         
        Special characters of note are: 
         


        ``\n`` 
            newline 

        ``\t`` 
            tab 

        ``\r`` 
            carriage return without the line feed 

         
        ``h.printf`` and ``h.fprint`` return the number of characters printed. 
         

    Example:

        .. code::

            h.printf("\tpi=%-20.10g sin(pi)=%f\n", h.PI, h.sin(h.PI)) 

                    pi=3.141592654          sin(pi)=0.000000 
                    42 

         
    Pure Python equivalent example:

        .. code::

            import math
            print('\tpi=%-20.10g sin(pi)=%f' % (math.pi, math.sin(math.pi)))

        .. note::

            The parentheses around the ``print`` argument are supplied in this way to allow
            it to work with both Python 2 and Python 3.

            This is not an identical replacement because it does not return the number of characters.
            In Python 2, this is a statement not a function and attempting to assign it to a variable is
            a syntax error. In Python 3, ``print`` is a function and the return is ``None``.


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

            from neuron import h

            def p():
                print('one') # to original standard out
                h.hoc_stdout('temp.tmp')
                print('two') # to temp.tmp
                for sec in h.allsec():
                    h.psection(sec=sec) # to temp.tmp
                h.hoc_stdout()
                print('three') # to the original standard out

            p() 

    .. note::

        Despite the misleading name, this redirects standard out from both Python and HOC.
