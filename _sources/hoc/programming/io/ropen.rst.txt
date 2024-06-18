
.. _hoc_ropen:

Open and Write to Files (Obsolete)
----------------------------------



.. hoc:function:: ropen


    Syntax:
        ``ropen("infile")``

        ``ropen()``


    Description:
        ``Ropen()`` opens *infile* for reading.  A subsequent ``ropen()`` with no arguments 
        closes the previously opened *infile*. A file which is re-opened with 
        ``ropen(infile)`` positions the stream at the beginning of the file. 

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
        :ref:`read <hoc_keyword_read>`, :hoc:func:`fscan`, :hoc:func:`getstr`, :hoc:class:`File`
        

    .. warning::
        There is no way to open more than one read file at a time.  The same is 
        true for write files. 
         


----



.. hoc:function:: wopen


    Syntax:
        ``wopen("outfile)``

        ``wopen()``


    Description:
        ``Wopen`` is similar to ``ropen`` but opens a file for writing. Contents of an 
        already existing *outfile* are destroyed.  Wopened files are written to 
        with :hoc:func:`fprint`. With no argument, the previously wopened file is closed.
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
        :hoc:func:`fprint`, :hoc:func:`File`


----



.. hoc:function:: xopen


    Syntax:
        ``xopen("hocfile")``

        ``xopen("hocfile", "RCSrevision")``


    Description:
        ``Xopen()`` executes the commands in ``hocfile``.  This is a convenient way 
        to define user functions and procedures. 
        An optional second argument is the RCS revision number in the form of a 
        string. The RCS file with that revision number is checked out into a 
        temporary file and executed. The temporary file is then removed.  A file 
        of the same primary name is unaffected. 


----



.. hoc:function:: fprint

        Same as :hoc:func:`printf` but prints to a file opened with :hoc:func:`wopen`. If no file
        is opened it prints to the standard output.

