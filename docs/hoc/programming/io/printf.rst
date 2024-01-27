
.. _hoc_printf_doc:


Printf (Formatted Output)
-------------------------

.. hoc:function:: printf

         

    Name:
        printf, fprint, sprint --- formatted output 
         

    Syntax:
        ``printf(format, ...)``

        ``fprint(format, ...)``

        ``sprint(string, format, ...)``



    Description:
        ``Printf`` places output on the standard output.  ``fprint`` places output 
        on the file opened with the ``wopen(filename)`` command (standard 
        output if no file is opened).  ``Sprint`` places output in its *string* 
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

         
        ``printf`` and ``fprint`` return the number of characters printed. 
         

    Example:

        .. code-block::
            none

            printf("\tpi=%-20.10g sin(pi)=%f\n", PI, sin(PI)) 
                    pi=3.141592654          sin(pi)=0.000000 
                    42 

         

    .. seealso::
        :hoc:meth:`File.ropen`
        

    .. warning::
        Only a subset of the C standard library functions. 
         

----


Redirect Standard Out
---------------------

.. hoc:function:: hoc_stdio


    Syntax:
        :samp:`hoc_stdout("{filename}")`

        ``hoc_stdout()``


    Description:
        With a filename argument, switches the original standard out to filename. 
        With no arguments. switches current standard out back to original filename. 
         
        Only one level of switching allowed. Switching back to original causes 
        future output to append to the stdout. Switching to "filename" writes 
        stdout from the beginning of the file. 

    Example:

        .. code-block::
            none

            proc p() { 
            	print "one" // to original standard out 
                    hoc_stdout("temp.tmp") 
                    print "two" // to temp.tmp 
            	forall psection() // to temp.tmp 
                    hoc_stdout() 
                    print "three" // to original standard out 
            } 
            p() 


