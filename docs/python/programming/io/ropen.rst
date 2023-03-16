.. _ropen:

Open and Write to Files (Obsolete)
----------------------------------



.. function:: ropen


    Syntax:
        ``h.ropen("infile")``

        ``h.ropen()``


    Description:
        ``h.ropen("infile")`` opens *infile* for reading.  A subsequent ``h.ropen()`` with no arguments 
        closes the previously opened *infile*. A file which is re-opened with 
        ``ropen(infile)`` positions the stream at the beginning of the file. 

    Example:

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
        This function returns 0 if the file is not successfully opened. 
         

    .. seealso::
        :ref:`read <keyword_read>`, :func:`fscan`, :func:`getstr`, :class:`File`
        

    .. warning::
        There is no way to open more than one read file at a time.  The same is 
        true for write files. 
         


----



.. function:: wopen


    Syntax:
        ``h.wopen("outfile")``

        ``h.wopen()``


    Description:
        ``h.wopen()`` is similar to ``h.ropen()`` but opens a file for writing. Contents of an 
        already existing *outfile* are destroyed.  Wopened files are written to 
        with :func:`fprint`. With no argument, the previously wopened file is closed. 
        h.wopen() returns 0 on failure to open a file. 

    Example:

        .. code-block::
            python

            from neuron import h

            def w_open(ndat, x, y):
                h.wopen("file.dat") 
                for i in range(ndat):
                    h.fprint(f"{x[i]} {y[i]}\n")
                h.wopen()

            # ndat is number of data points
            ndat = 3
            x = [42.0, 14.0, 12.0]
            y = [13.7, 64.1, 9.0]
            w_open(ndat, x, y)

            


    .. seealso::
        :func:`fprint`, :func:`File`


----



.. function:: xopen


    Syntax:
        ``h.xopen("hocfile")``

        ``h.xopen("hocfile", "RCSrevision")``


    Description:
        ``h.xopen()`` executes the commands in ``hocfile``.  This is a convenient way 
        to define user functions and procedures. 
        An optional second argument is the RCS revision number in the form of a 
        string. The RCS file with that revision number is checked out into a 
        temporary file and executed. The temporary file is then removed.  A file 
        of the same primary name is unaffected. 


----



.. function:: fprint

    Syntax:
        ``h.fprint()``

    Example:

        .. code-block::
            python

            h.fprint("%g %g\n", x, y)

    Description:

        Same as :func:`printf` but prints to a file opened with :func:`wopen`. If no file 
        is opened it prints to the standard output.

