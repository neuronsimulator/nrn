.. _ropen:

ropen
-----



.. function:: ropen


    Syntax:
        :code:`ropen("infile")`

        :code:`ropen()`


    Description:
        :code:`Ropen()` opens *infile* for reading.  A subsequent :code:`ropen()` with no arguments 
        closes the previously opened *infile*. A file which is re-opened with 
        :code:`ropen(infile)` positions the stream at the beginning of the file. 

    Example:

        .. code-block::
            none

            proc r() { 
               ropen("file.dat") 
               for (i=0; i<ndat; i=i+1) { 
                  x[i] = fscan()  y[i]=fscan() 
               } 
               ropen() 
            } 

         
         

    Diagnostics:
        This function returns 0 if the file is not successfully opened. 
         

    .. seealso::
        :ref:`read <keyword_read>`, :func:`fscan`, :func:`getstr`, :class:`File`
        

    .. warning::
        There is no way to open more than one read file at a time.  The same is 
        true for write files. 
         


----



.. function:: wopen


    Syntax:
        :code:`wopen("outfile)`

        :code:`wopen()`


    Description:
        :code:`Wopen` is similar to :code:`ropen` but opens a file for writing. Contents of an 
        already existing *outfile* are destroyed.  Wopened files are written to 
        with :func:`fprint`. With no argument, the previously wopened file is closed. 
        Wopen returns 0 on failure to open a file. 

    Example:

        .. code-block::
            none

            proc w() { 
               wopen("file.dat") 
               for (i=0; i<ndat; i=i+1) { 
                  fprint("%g %g\n", x[i], y[i]) 
               } 
               wopen() 
            } 


    .. seealso::
        :func:`fprint`, :func:`File`


----



.. function:: xopen


    Syntax:
        :code:`xopen("hocfile")`

        :code:`xopen("hocfile", "RCSrevision")`


    Description:
        :code:`Xopen()` executes the commands in :code:`hocfile`.  This is a convenient way 
        to define user functions and procedures. 
        An optional second argument is the RCS revision number in the form of a 
        string. The RCS file with that revision number is checked out into a 
        temporary file and executed. The temporary file is then removed.  A file 
        of the same primary name is unaffected. 


----



.. function:: fprint

        Same as :func:`printf` but prints to a file opened with :func:`wopen`. If no file 
        is opened it prints to the standard output.

