.. _secref:

SectionRef
----------

.. note::

    Much of this functionality is available in Python through :class:`Section` methods in
    recent versions of NEURON. It is, however, sometimes necessary to use this class to
    interoperate with legacy code.


.. class:: SectionRef

    .. tab:: Python
    
    
        Syntax:
            ``sref = n.SectionRef(sec=section)``


        Description:
            SectionRef keeps a pointer/reference to section. If the ``sec=`` argument is
            omitted, the reference is to the currently accessed section.
         
            This class overcomes a HOC limitation where sections were not treated as objects.

    .. tab:: HOC


        Syntax:
            ``section sref = new SectionRef()``
        
        
        Description:
            SectionRef keeps a pointer/reference to a section 
            The reference is to the currently accessed section at the 
            time the object was created. 
        
        
            This class allows sections to be referenced as normal object variables 
            for assignment and passing as arguments. 
        
----



.. data:: SectionRef.sec

    .. tab:: Python
    
    
        Syntax:
            ``sref.sec``


        Description:
            Returns the :class:`Section` the ``SectionRef`` references.

            .. code::

                from neuron import n

                s = n.Section("s")
                s2 = n.Section("s2")
                sref = n.SectionRef(sec=s2)

                print(sref.sec==s)  # False
                print(sref.sec==s2) # True



    .. tab:: HOC


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
                soma s1 = new SectionRef()  // s1 holds a reference to the soma 
                print s1.sec.diam           // print the diameter of the soma 
                s2 = s1                             // s2 also holds a reference to the soma 
                s2.sec { psection() }               // print all info about soma 
                axon s2 = new SectionRef() 
                proc c() { 
                    $o1.sec connect $o2.sec(0), 1 
                } 
                c(s1, s2) 
                topology() 
        
        
            This last is a procedure that takes two SectionRef args and 
            connects them end to end. 
        
----



.. data:: SectionRef.parent

    .. tab:: Python
    
    
        Syntax:
            ``sref.parent``


        Description:

            Returns the parent of ``sref.sec``.

        .. warning::

            If there is a chance that a section does not have a parent then 
            :meth:`SectionRef.has_parent` should be called first to avoid an execution error. 
            Note that the parent is the current parent of sref.sec, not necessarily 
            the parent when the SectionRef was created. 


    .. tab:: HOC


        Syntax:
            ``sref.parent``
        
        
        Description:
            parent of sref.sec becomes the currently accessed section. Generally it 
            is used in a context like \ ``sref.parent { statement }`` just like a 
            normal section name and does NOT need a section_pop 
            If there is a chance that a section does not have a parent then 
            :meth:`SectionRef.has_parent` should be called first to avoid an execution error.
            Note that the parent is the current parent of sref.sec, not necessarily 
            the parent when the SectionRef was created. 
        
----



.. data:: SectionRef.trueparent

    .. tab:: Python
    
    
        Syntax:
            ``sref.trueparent``


        Description:
            Returns the trueparent of ``sref.sec``.

            This is normally identical to :meth:`SectionRef.parent` except when the 
            parent's :func:`parent_connection` is equal to the parent's 
            :func:`section_orientation`. 

            If there is a chance that a section does not have a trueparent then 
            :meth:`SectionRef.has_trueparent` should be called first to avoid an execution error. 


    .. tab:: HOC


        Syntax:
            ``sref.trueparent``
        
        
        Description:
            trueparent of sref.sec becomes the currently accessed section. 
            This is normally identical to :meth:`SectionRef.parent` except when the
            parent's :func:`parent_connection` is equal to the parent's
            :func:`section_orientation`.
            If there is a chance that a section does not have a trueparent then 
            :meth:`SectionRef.has_trueparent` should be called first to avoid an execution error.
        
----



.. data:: SectionRef.child

    .. tab:: Python
    
    
        Syntax:
            ``sref.child[i]``


        Description:
            Returns the ith child of ``sref.sec``.
            Generally it is used in a context like 

            .. code::
            
                for child in sref.child:
                    print(child)

            Note that the children are the current children of sref.sec, not necessarily 
            the same as when the SectionRef was created since sections may be 
            deleted or re-connected subsequent to the instantiation of the SectionRef. 


    .. tab:: HOC


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



.. data:: SectionRef.root

    .. tab:: Python
    
    
        Syntax:
            ``sref.root``


        Description:

            Returns the root of ``sref.sec``.


    .. tab:: HOC


        Syntax:
            ``sref.root``
        
        
        Description:
            root of sref.sec becomes the currently accessed section. 
        
----



.. method:: SectionRef.has_parent

    .. tab:: Python
    
    
        Syntax:
            ``boolean = sref.has_parent()``


        Description:
            Returns ``True`` if sref.sec has a parent and ``False`` if sref.sec is a root section. 
            Invoking sref.parent when sref.sec is a root section will print an 
            error message and halt execution.

        Note:

            If ``sec`` is a Section, then ``sec.parentseg()`` is either the segment the section is
            attached to or ``None`` if ``sec`` does not have a parent.


    .. tab:: HOC


        Syntax:
            ``boolean = sref.has_parent``
        
        
        Description:
            returns 1 if sref.sec has a parent and 0 if sref.sec is a root section. 
            Invoking sref.parent when sref.sec is a root section will print an 
            error message and halt execution. 
        
----



.. method:: SectionRef.has_trueparent

    .. tab:: Python
    
    
        Syntax:
            ``boolean = sref.has_trueparent()``


        Description:
            returns ``True`` if the sref.sec parent node is not the root node and ``False`` otherwise. 
            Invoking sref.trueparent when it is the root node will print an 
            error message and halt execution. 


    .. tab:: HOC


        Syntax:
            ``boolean = sref.has_trueparent``
        
        
        Description:
            returns 1 if the sref.sec parent node is not the root node and 0 otherwise. 
            Invoking sref.trueparent when it is the root node will print an 
            error message and halt execution. 
        
----



.. method:: SectionRef.nchild

    .. tab:: Python
    
    
        Syntax:
            ``num = sref.nchild()``


        Description:
            Return the number of child sections connected to sref.sec as a float.

        .. note::

            To get the number of child sections as an int, use: ``num = len(sref.child)``

         

    .. tab:: HOC


        Syntax:
            ``integer = sref.nchild``
        
        
        Description:
            Return the number of child sections connected to sref.sec 
        
----



.. method:: SectionRef.is_cas

    .. tab:: Python
    
    
        Syntax:
            ``boolean = sref.is_cas()``


        Description:
            Returns True if this section reference is the currently accessed (default) section, False otherwise. 

        .. note::

            An equivalent expression is ``(sref.sec == n.cas())``.

         

    .. tab:: HOC


        Syntax:
            ``boolean = sref.is_cas()``
        
        
        Description:
            Returns 1 if this section reference is the currently accessed section, 0 otherwise. 
        
----



.. method:: SectionRef.exists

    .. tab:: Python
    
    
        Syntax:
            ``boolean = sref.exists()``


        Description:
            Returns True if the referenced section has not been deleted, False otherwise. 

        .. seealso::
            :func:`delete_section`, :func:`section_exists`

         
         

    .. tab:: HOC


        Syntax:
            ``boolean = sref.exists()``
        
        
        Description:
            Returns 1 if the section has not been deleted, 0 otherwise. 
        
        
        .. seealso::
            :func:`delete_section`, :func:`section_exists`
        
