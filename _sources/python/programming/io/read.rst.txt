.. _read:

Read from Terminal and Files
----------------------------

.. warning::
    These functions are provided for legacy code support. When writing new code, consider using Python input-output equivalents.

.. function:: xred


    Syntax:
        ``var = h.xred(promptstring, default, min, max)``


    Description:
        ``xred()`` reads a value from the standard input after printing *promptstring* 
        on the console.  The value read must be in the range *min* <= *val* <= *max* and 
        the user will be prompted to enter another number if the value is not in 
        that range.  If the user types the :kbd:`Return` key, the *default* value is used 
        (if it is in the valid range). 


----



.. function:: getstr


    Syntax:
        ``h.getstr(strvar)``

        ``h.getstr(strvar, 1)``


    Description:
        ``getstr()`` reads a string up to and including the next newline from the file 
        opened with the :meth:`~File.ropen` command (or the currently executing file or 
        the standard input) and places it 
        in its string variable argument. With a second arg equal to 1, getstr reads 
        a single word starting at the next non-whitespace character up to 
        but not including the following whitespace (similar to fscan).
    Example:
        .. code-block::
            python


            ###########################################
            ### create a file titled "file.dat" with: #
            ### hello                                 #
            ### world                                 #
            ###########################################
            
            from neuron import h
            
            def r_open(ndat):
                h.ropen("file.dat")
                string = ""
                s = h.ref(string)
                x = []
                for i in range(ndat):
                    h.getstr(s, 1)
                    x.append(s[0])
                h.ropen()
                return x

            # ndat is number of data points
            ndat = 2
            x = r_open(ndat)
            print(x)


    .. seealso::
        :class:`StringFunctions`, :func:`sscanf`, :class:`File`


----



.. function:: sred


    Syntax:
        ``index = h.sred(prompt, defaultchar, charlist)``


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
            python
            
            from neuron import h

            def response(answer):
                if (answer == 0):
                    print("No")
                else:
                    print("Yes")

            i = 0 
            while i == 0:
                i = h.sred("Shall we?", "y", "ny")
                response(i)
    



----



.. function:: fscan


    Syntax:
        ``var = h.scan()``


    Description:
        ``fscan()`` reads the next number from the file opened with the :meth:`~File.ropen` 
        command. If no file is opened the number is read from the currently 
        executing file. If no file is being executed the number is read from 
        the standard input. 
        A number is scanned as long as it begins with a digit, decimal point, or 
        sign.  There can be more than one number per line but they must be set 
        apart from each other by spaces or tabs.  Strings that can't be scanned 
        into numbers are skipped. 

    Example:
        Suppose in response to the command: ``print(fscan(), fscan())`` 
        the user types: ``this is a number 1.3e4 this is not45 this is 25`` 
        Then NEURON will print: ``13000 25`` 
         

        .. code-block::
            python

            ###########################################
            ### create a file titled "file.dat" with: #
            ### 42 13.7                               #
            ### 14 64.1                               #
            ### 12 9                                  #
            ###########################################

            from neuron import h

            def r_open(ndat):
                h.ropen("file.dat")
                x = []
                y = []
                for i in range(ndat):
                    x.append(h.fscan())
                    y.append(h.fscan())
                    h.ropen()
                return x, y

            # ndat is number of data points
            ndat = 3
            x, y = r_open(ndat)
             


    Diagnostics:
        ``fscan()`` and ``getstr()`` returns to the HOC 
        interpreter with a run-time error on EOF. 
         
    .. note::
    These functions are provided for legacy code support. 
    In Python, it only supports file input not terminal input. 

    When writing new code, consider using Python input-output equivalents.

    .. seealso::
        :meth:`File.scanvar`, :ref:`read <keyword_read>`, :meth:`File.ropen`, :func:`File`, :func:`sscanf`, :class:`StringFunctions`, :func:`getstr`


