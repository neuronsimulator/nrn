Obsolete Pointer Functions
==========================


.. hoc:function:: this_node


    Syntax:
        ``this_node(x)``


    Description:
        Return a pointer (coded as a double) to the segment 
        of the currently accessed section that 
        contains location *x*. If you wish to compute a segment number 
        index where 1 is the first nonzero area segment and nseg is the last 
        nonzero area segment 
        of the currently accessed section corresponding to position x use the 
        hoc function 

        .. code-block::
            none

            func segnum() { 
                    if ($1 <= 0) { 
                            return 0 
                    }else if ($1 >= 1) { 
                            return nseg+1 
                    }else { 
                            return int($1*nseg + .5) 
                    } 
            } 


    .. warning::
        This function is useless and should be removed. 

----



.. hoc:function:: this_section


    Syntax:
        ``this_section(x)``


    Description:
        Return a pointer (coded as a double) to the section which contains location 0 of the 
        currently accessed section. This pointer can be used as the argument 
        to :hoc:func:`push_section`. Functions that return pointers coded as doubles
        are unsafe with 64 bit pointers. This function has been superseded by 
        :hoc:class:`SectionRef`. See :hoc:meth:`~SectionRef.sec`.

         

