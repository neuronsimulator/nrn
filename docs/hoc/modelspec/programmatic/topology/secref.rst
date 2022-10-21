
.. _hoc_secref:

SectionRef
----------



.. hoc:class:: SectionRef


    Syntax:
        ``section sref = new SectionRef()``


    Description:
        SectionRef keeps a pointer/reference to a section 
        The reference is to the currently accessed section at the 
        time the object was created. 
         
        This class allows sections to be referenced as normal object variables 
        for assignment and passing as arguments. 


----



.. hoc:method:: SectionRef.sec


    Syntax:
        ``sref.sec``


    Description:
        special syntax that makes the reference the currently 
        accessed section. 
        This class allows sections to be referenced as normal object variables 
        for assignment and passing as arguments. The usage is 

        .. code-block::
            none

            create soma, axon 
            axon.diam=2 
            soma.diam=10 
            access axon 
            objref s1, s2 
            soma s1 = new SectionRef()	// s1 holds a reference to the soma 
            print s1.sec.diam		// print the diameter of the soma 
            s2 = s1				// s2 also holds a reference to the soma 
            s2.sec { psection() }		// print all info about soma 
            axon s2 = new SectionRef() 
            proc c() { 
            	$o1.sec connect $o2.sec(0), 1 
            } 
            c(s1, s2) 
            topology() 

        This last is a procedure that takes two SectionRef args and 
        connects them end to end. 


----



.. hoc:method:: SectionRef.parent


    Syntax:
        ``sref.parent``


    Description:
        parent of sref.sec becomes the currently accessed section. Generally it 
        is used in a context like \ ``sref.parent { statement }`` just like a 
        normal section name and does NOT need a section_pop 
        If there is a chance that a section does not have a parent then 
        :hoc:meth:`SectionRef.has_parent` should be called first to avoid an execution error.
        Note that the parent is the current parent of sref.sec, not necessarily 
        the parent when the SectionRef was created. 


----



.. hoc:method:: SectionRef.trueparent


    Syntax:
        ``sref.trueparent``


    Description:
        trueparent of sref.sec becomes the currently accessed section. 
        This is normally identical to :hoc:meth:`SectionRef.parent` except when the
        parent's :hoc:func:`parent_connection` is equal to the parent's
        :hoc:func:`section_orientation`.
        If there is a chance that a section does not have a trueparent then 
        :hoc:meth:`SectionRef.has_trueparent` should be called first to avoid an execution error.


----



.. hoc:method:: SectionRef.child


    Syntax:
        ``sref.child[i]``


    Description:
        the ith child of sref.sec becomes the currently accessed section. 
        Generally it 
        is used in a context like 

        .. code-block::
            none

            for i=0, sref.nchild-1 sref.child[i] { statement } 

        Note that the children are the current children of sref.sec, not necessarily 
        the same as when the SectionRef was created since sections may be 
        deleted or re-connected subsequent to the instantiation of the SectionRef. 


----



.. hoc:method:: SectionRef.root


    Syntax:
        ``sref.root``


    Description:
        root of sref.sec becomes the currently accessed section. 


----



.. hoc:method:: SectionRef.has_parent


    Syntax:
        ``boolean = sref.has_parent``


    Description:
        returns 1 if sref.sec has a parent and 0 if sref.sec is a root section. 
        Invoking sref.parent when sref.sec is a root section will print an 
        error message and halt execution. 


----



.. hoc:method:: SectionRef.has_trueparent


    Syntax:
        ``boolean = sref.has_trueparent``


    Description:
        returns 1 if the sref.sec parent node is not the root node and 0 otherwise. 
        Invoking sref.trueparent when it is the root node will print an 
        error message and halt execution. 


----



.. hoc:method:: SectionRef.nchild


    Syntax:
        ``integer = sref.nchild``


    Description:
        Return the number of child sections connected to sref.sec 

         

----



.. hoc:method:: SectionRef.is_cas


    Syntax:
        ``boolean = sref.is_cas()``


    Description:
        Returns 1 if this section reference is the currently accessed section, 0 otherwise. 

         

----



.. hoc:method:: SectionRef.exists


    Syntax:
        ``boolean = sref.exists()``


    Description:
        Returns 1 if the section has not been deleted, 0 otherwise. 

    .. seealso::
        :hoc:func:`delete_section`, :hoc:func:`section_exists`

         
         

