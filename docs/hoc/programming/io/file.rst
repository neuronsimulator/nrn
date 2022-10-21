
.. _hoc_file:

File Access (Recommended Way)
-----------------------------



.. hoc:class:: File


    Syntax:
        ``fobj = new File()``

        ``fobj = new File("filename")``


    Description:
        This class allows you to open several files at once, whereas the top level 
        functions, :hoc:func:`ropen` and :hoc:func:`wopen` ,
        allow you to deal only with one file at a time. 
         
        The functionality of :hoc:func:`xopen` is not implemented in this class so use

        .. code-block::
            none

            	strdef tstr 
            	fobj.getname(tstr) 
            	xopen(tstr) 

        to execute a sequence of interpreter commands in a file. 
         

    Example:

        .. code-block::
            none

            objref f1, f2, f3	//declare object references 
            f1 = new File()		//state that f1, f2, and f3 are pointers to the File class 
            f2 = new File() 
            f3 = new File() 
            f1.ropen("file1")	//open file1 for reading 
            f2.wopen("file2")	//open file2 for writing 
            f3.aopen("file3")	//open file3 for appending to the end of the file 

        opens file1, file2, and file3 for reading, writing, and appending (respectively). 
         

    .. warning::
        The mswindows/dos version must deal with the difference between 
        binary and text mode files. The difference is transparent to the 
        user except for one limitation. If you mix text and binary data 
        and you write text first to the file, then you need to do a 
        File.seek(0) to explicitly switch to binary mode just after opening the file 
        and prior to the first File.printf. 
        An error message will occur if you 
        read/write text to a file in text mode and then try to read/write a binary 
        vector.  This issue does not arise with the unix version. 

    .. seealso::
        :ref:`hoc_printf_doc`, :hoc:func:`ropen`, :hoc:func:`xopen`, :hoc:func:`wopen`


----



.. hoc:method:: File.ropen


    Syntax:
        ``.ropen()``

        ``.ropen("name")``


    Description:
        Open the file for reading. If the argument is 
        not present it opens (for reading) the last specified file. 

         

----



.. hoc:method:: File.wopen


    Syntax:
        ``.wopen()``

        ``.wopen("name")``


    Description:
        Open the file for writing.  If the argument is 
        not present it opens the last specified file. 

         

----



.. hoc:method:: File.aopen


    Syntax:
        ``.aopen()``

        ``.aopen("name")``


    Description:
        Open the file for appending to the end of the file. If the argument is 
        not present it opens the last specified file. 

         

----



.. hoc:method:: File.xopen


    Syntax:
        ``.xopen()``

        ``.xopen("name")``


    Description:
        Open the file and execute it. (not implemented) 
         
        Note: if instead of a "*name*", the number 0,1,or 2 is specified then 
        the stdin, stdout, or stderr is opened. (not implemented) 

         

----



.. hoc:method:: File.close


    Syntax:
        ``.close()``


    Description:
        Flush and close the file. This occurs automatically 
        whenever opening another file or destroying the object. 

         

----



.. hoc:method:: File.mktemp


    Syntax:
        ``boolean = f.mktemp()``


    Description:
        Sets the name to a temporary filename in the /tmp directory (or other 
        writable path for mswin and mac). Success returns 1. 

         

----



.. hoc:method:: File.unlink


    Syntax:
        ``boolean = f.unlink()``


    Description:
        Remove the file specified by the current name. A return value of 
        1 means the file was removed (or at least it's link count was reduced by 
        one and the filename no longer exists). 

         

----



.. hoc:method:: File.printf


    Syntax:
        ``.printf("format", args, ...)``


    Description:
        As in standard C \ ``printf`` and the normal 
        hoc :hoc:func:`printf` .

         

----



.. hoc:method:: File.scanvar


    Syntax:
        ``.scanvar()``


    Description:
        Reads the next number as in the hoc function \ ``fscan()`` and 
        returns its value. 
         
        Note: in order that .eof will return 
        true after the last number, the last digit of that number 
        should either be the last character in the file or 
        be followed by a newline which is the last character in the file. 

         

----



.. hoc:method:: File.scanstr


    Syntax:
        ``.scanstr(strdef)``


    Description:
        Read the next string (delimited by whitespace) into 
        \ ``strdef``. Returns the length of a string (if failure then returns 
        -1 and \ ``strdef`` is unchanged). 

         

----



.. hoc:method:: File.gets


    Syntax:
        ``.gets(strdef)``


    Description:
        Read up to and including end of line. Returns length of	string. 
        If at the end of file, returns -1 and does not change the argument. 

         

----



.. hoc:method:: File.getname


    Syntax:
        ``strdef = file.getname()``

        ``strdef = file.getname(strdef)``


    Description:
        Return the name of the last specified file as a strdef. 
        For backward compatibility, if the arg is present also copy it to that. 

         

----



.. hoc:method:: File.dir


    Syntax:
        ``strdef = file.dir()``


    Description:
        Return the pathname of the last directory moved to in the chooser. 
        If the :hoc:meth:`File.chooser` has not been created, return the empty string.

         

----



.. hoc:method:: File.eof


    Syntax:
        ``.eof()``


    Description:
        Return true if at end of ropen'd file. 

         

----



.. hoc:method:: File.flush


    Syntax:
        ``.flush()``


    Description:
        Flush pending output to the file. 

         

----



.. hoc:method:: File.isopen


    Syntax:
        ``.isopen()``


    Description:
        Return true if a file is open. 

         

----



.. hoc:method:: File.chooser


    Syntax:
        ``.chooser()``

        ``.chooser("w,r,a,x,d or nothing")``

        ``.chooser("w,r,a,x,d or nothing", "Banner", "filter", "accept", "cancel", "path")``



    Description:
        File chooser interface for writing , reading, appending, or 
        just specifying a directory or filename without opening. The banner is 
        optional. The filter, eg. \ ``"*.dat"`` specifies the files shown 
        in the browser part of the chooser. 
        The "path" arg specifies the file or directory to use when the 
        browser first pops up. 
        The form with args sets the style of the chooser but 
        does not pop it up. With no args, the browser pops up and can 
        be called several times. Each time starting where it left 
        off previously. 
         
        The "d" style is used for selecting a directory (in 
        contrast to a file). 
        With the "d" style, three buttons are placed beneath the 
        browser area with :guilabel:`Open` centered beneath the :guilabel:`Show`, :guilabel:`Cancel` button pair. 
        The :guilabel:`Open` button must be pressed for the 
        dialog to return the name of the directory. The :guilabel:`Show` button merely 
        selects the highlighted browser entry and shows the relevant directory 
        contents. A returned directory 
        string always has a final "/". 
         
        The "*x*" style is unimplemented. Use 

        .. code-block::
            none

            		f.chooser("", "Execute a hoc file", "*.hoc", "Execute") 
            		if (f.chooser()) { 
            			f.getname(*str*) 
            			xopen(*str*) 
            		} 

        The following comes courtesy of Zach Mainen, ``zach@helmholtz.sdsc.edu``. 

         

----



.. hoc:method:: File.vwrite


    Syntax:
        ``.vwrite(&x)``

        ``.vwrite(n, &x)``


    Description:
        Write binary doubles to a file from an array or variable 
        using \ ``fwrite()``. The form with two arguments specifies the 
        number of elements to write and the address from which to 
        begin writing.  With one argument, *n* is assumed to be 1. 
        Must be careful that  *x*\ [] has at least *n* 
        elements after its passed address. 

         

----



.. hoc:method:: File.vread


    Syntax:
        ``.vread(&x)``

        ``.vread(n, &x)``


    Description:
        Read binary doubles from a file into a pre-existing array 
        or variable using \ ``fread()``. 

    .. seealso::
        :hoc:func:`vwrite`
        

         
         

----



.. hoc:method:: File.seek


    Syntax:
        ``.seek()``

        ``.seek(offset)``

        ``.seek(offset,origin)``


    Description:
        Set the file position.  Any subsequent file access will access 
        data beginning at the new position.  Without arguments, goes to 
        the beginning of file.  Offset is in characters and is measured 
        from the beginning of the file unless origin is 1 (measures from 
        the current position) or 2 (from the end of the file).  Returns 
        0 if successful, non-zero on error.  Used with :hoc:meth:`tell`.

         

----



.. hoc:method:: File.tell


    Syntax:
        ``.tell()``


    Description:
        Return the current file position or -1 on error.  Used with :hoc:meth:`seek`.

