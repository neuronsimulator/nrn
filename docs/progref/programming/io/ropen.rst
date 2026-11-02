.. _ropen:

Open and Write to Files (Obsolete)
----------------------------------



.. function:: ropen

    .. tab:: Python
    
    
        Syntax:
            ``n.ropen("infile")``

            ``n.ropen()``


        Description:
            ``n.ropen("infile")`` opens *infile* for reading.  A subsequent ``n.ropen()`` with no arguments 
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

                from neuron import n

                def r_open(ndat):
                    n.ropen("file.dat")
                    x = []
                    y = []
                    for i in range(ndat):
                        x.append(n.fscan())
                        y.append(n.fscan())
                    n.ropen()
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
         


    .. tab:: HOC


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
            :ref:`read <hoc_keyword_read>`, :func:`fscan`, :func:`getstr`, :class:`File`
        
        
        .. warning::
            There is no way to open more than one read file at a time.  The same is 
            true for write files. 
        
----



.. function:: wopen

    .. tab:: Python
    
    
        Syntax:
            ``n.wopen("outfile")``

            ``n.wopen()``


        Description:
            ``n.wopen()`` is similar to ``n.ropen()`` but opens a file for writing. Contents of an 
            already existing *outfile* are destroyed.  Wopened files are written to 
            with :func:`fprint`. With no argument, the previously wopened file is closed. 
            n.wopen() returns 0 on failure to open a file. 

        Example:

            .. code-block::
                python

                from neuron import n

                def w_open(ndat, x, y):
                    n.wopen("file.dat") 
                    for i in range(ndat):
                        n.fprint(f"{x[i]} {y[i]}\n")
                    n.wopen()

                # ndat is number of data points
                ndat = 3
                x = [42.0, 14.0, 12.0]
                y = [13.7, 64.1, 9.0]
                w_open(ndat, x, y)

            


        .. seealso::
            :func:`fprint`, :func:`File`


    .. tab:: HOC


        Syntax:
            ``wopen("outfile)``
        
        
            ``wopen()``
        
        
        Description:
            ``Wopen`` is similar to ``ropen`` but opens a file for writing. Contents of an 
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

    .. tab:: Python
    
    
        Syntax:
            ``n.xopen("hocfile")``

            ``n.xopen("hocfile", "RCSrevision")``


        Description:
            ``n.xopen()`` executes the commands in ``hocfile``.  This is a convenient way 
            to define user functions and procedures. 
            An optional second argument is the RCS revision number in the form of a 
            string. The RCS file with that revision number is checked out into a 
            temporary file and executed. The temporary file is then removed.  A file 
            of the same primary name is unaffected. 


    .. tab:: HOC


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



.. function:: fprint

    .. tab:: Python
    
        Syntax:
            ``n.fprint(format_specifier, ...)``

        Example:

            .. code-block::
                python

                n.fprint("%g %g\n", x, y)

        Description:

            Same as :func:`printf` but prints to a file opened with :func:`wopen`. If no file 
            is opened it prints to the standard output.

    .. tab:: HOC


        Same as :func:`printf` but prints to a file opened with :func:`wopen`. If no file
        is opened it prints to the standard output.
        
