
.. _hoc_read:

Read from Terminal and Files
----------------------------



.. hoc:function:: xred


    Syntax:
        ``var = xred(promptstring, default, min, max)``


    Description:
        ``Xred()`` reads a value from the standard input after printing *promptstring* 
        on the console.  The value read must be in the range *min* <= *val* <= *max* and 
        the user will be prompted to enter another number if the value is not in 
        that range.  If the user types the :kbd:`Return` key, the *default* value is used 
        (if it is in the valid range). 


----



.. hoc:function:: getstr


    Syntax:
        ``getstr(strvar)``

        ``getstr(strvar, 1)``


    Description:
        ``Getstr()`` reads a string up to and including the next newline from the file 
        opened with the :hoc:meth:`~File.ropen` command (or the currently executing file or
        the standard input) and places it 
        in its string variable argument. With a second arg equal to 1, getstr reads 
        a single word starting at the next non-whitespace character up to 
        but not including the following whitespace (similar to fscan). 

    .. seealso::
        :hoc:class:`StringFunctions`, :hoc:func:`sscanf`, :hoc:class:`File`


----



.. hoc:function:: sred


    Syntax:
        ``index = sred(prompt, defaultchar, charlist)``


    Description:
        ``sred()`` reads a character typed on the standard input after printing the 
        first argument followed by the default character. The user is required to 
        enter one of the characters in the character list (or return if the default 
        happens to be one of these characters). The function returns the index in 
        the character list of the character typed. The index of the first character 
        is 0. The character accepted becomes the next default when this statement 
        is executed again. This function was contributed by Stewart Jaslove. 

    Example:

        .. code-block::
            none

            i = sred("Shall we?", "y", "ny") 
            if (i == 0) print "No" else print "yes" 



----



.. hoc:function:: fscan


    Syntax:
        ``var = fscan()``


    Description:
        ``fscan()`` reads the next number from the file opened with the :hoc:meth:`~File.ropen`
        command. If no file is opened the number is read from the currently 
        executing file. If no file is being executed the number is read from 
        the standard input. 
        A number is scanned as long as it begins with a digit, decimal point, or 
        sign.  There can be more than one number per line but they must be set 
        apart from each other by spaces or tabs.  Strings that can't be scanned 
        into numbers are skipped. 

    Example:
        Suppose in response to the HOC command: ``print fscan(), fscan()`` 
        the user types: ``this is a number 1.3e4 this is not45 this is 25`` 
        Then HOC will print: ``13000 25`` 
         

        .. code-block::
            none

            while(1) print fscan() 
             
            notice that when no file is open, fscan scans the remainder of the hoc file 
            following only scans the numbers from 10 to 170 
            10 
            n 
            20 
            n 30 na 40 nan 50 nano 60 nanotube 70 ni 80 nai 90 Nan NaN 
             
            i 100 in 110 inf 120 infi 130 ib 140 inc 150 infinity 160 170 Inf INF 
             
            following scans the numbers 
            1 2 3 4 5 6 7 8 9 10 
            - + does not scan 
             
            1.1 -1.2 1.3e-4 1.4e+4 -1.5e5 -1.6e-1 
             
            1+2+3 scans just the "1" 
            4xxx5 scans just the "4" 
             
            1,2,3 scans just the "1" 
            3, 4, 5 scans the three numbers 
             
            now there will be an EOF error 
             


    Diagnostics:
        ``Fscan()`` and ``getstr()`` returns to the HOC 
        interpreter with a run-time error on EOF. 
         

    .. seealso::
        :hoc:meth:`File.scanvar`, :ref:`read <hoc_keyword_read>`, :hoc:meth:`File.ropen`, :hoc:func:`File`, :hoc:func:`sscanf`, :hoc:class:`StringFunctions`, :hoc:func:`getstr`


