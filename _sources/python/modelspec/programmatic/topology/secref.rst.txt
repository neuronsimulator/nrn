.. _secref:

SectionRef
----------

.. note::

    Much of this functionality is available in Python through :class:`Section` methods in
    recent versions of NEURON. It is, however, sometimes necessary to use this class to
    interoperate with legacy code.


.. class:: SectionRef


    Syntax:
        ``sref = h.SectionRef(sec=section)``


    Description:
        SectionRef keeps a pointer/reference to section. If the ``sec=`` argument is
        omitted, the reference is to the currently accessed section.
         
        This class overcomes a HOC limitation where sections were not treated as objects.

----



.. data:: SectionRef.sec


    Syntax:
        ``sref.sec``


    Description:
        Returns the :class:`Section` the ``SectionRef`` references.

        .. code::

            from neuron import h

            s = h.Section(name="s")
            s2 = h.Section(name="s2")
            sref = h.SectionRef(sec=s2)

            print(sref.sec==s)  # False
            print(sref.sec==s2) # True



----



.. data:: SectionRef.parent


    Syntax:
        ``sref.parent``


    Description:

        Returns the parent of ``sref.sec``.

    .. warning::

        If there is a chance that a section does not have a parent then 
        :meth:`SectionRef.has_parent` should be called first to avoid an execution error. 
        Note that the parent is the current parent of sref.sec, not necessarily 
        the parent when the SectionRef was created. 


----



.. data:: SectionRef.trueparent


    Syntax:
        ``sref.trueparent``


    Description:
        Returns the trueparent of ``sref.sec``.

        This is normally identical to :meth:`SectionRef.parent` except when the 
        parent's :func:`parent_connection` is equal to the parent's 
        :func:`section_orientation`. 

        If there is a chance that a section does not have a trueparent then 
        :meth:`SectionRef.has_trueparent` should be called first to avoid an execution error. 


----



.. data:: SectionRef.child


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


----



.. data:: SectionRef.root


    Syntax:
        ``sref.root``


    Description:

        Returns the root of ``sref.sec``.


----



.. method:: SectionRef.has_parent


    Syntax:
        ``boolean = sref.has_parent()``


    Description:
        Returns ``True`` if sref.sec has a parent and ``False`` if sref.sec is a root section. 
        Invoking sref.parent when sref.sec is a root section will print an 
        error message and halt execution.

    Note:

        If ``sec`` is a Section, then ``sec.parentseg()`` is either the segment the section is
        attached to or ``None`` if ``sec`` does not have a parent.


----



.. method:: SectionRef.has_trueparent


    Syntax:
        ``boolean = sref.has_trueparent()``


    Description:
        returns ``True`` if the sref.sec parent node is not the root node and ``False`` otherwise. 
        Invoking sref.trueparent when it is the root node will print an 
        error message and halt execution. 


----



.. method:: SectionRef.nchild


    Syntax:
        ``num = sref.nchild()``


    Description:
        Return the number of child sections connected to sref.sec as a float.

    .. note::

        To get the number of child sections as an int, use: ``num = len(sref.child)``

         

----



.. method:: SectionRef.is_cas


    Syntax:
        ``boolean = sref.is_cas()``


    Description:
        Returns True if this section reference is the currently accessed (default) section, False otherwise. 

    .. note::

        An equivalent expression is ``(sref.sec == h.cas())``.

         

----



.. method:: SectionRef.exists


    Syntax:
        ``boolean = sref.exists()``


    Description:
        Returns True if the referenced section has not been deleted, False otherwise. 

    .. seealso::
        :func:`delete_section`, :func:`section_exists`

         
         

