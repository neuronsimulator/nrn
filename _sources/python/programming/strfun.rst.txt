.. _strfun:

StringFunctions (String Manipulation Class)
-------------------------------------------



.. class:: StringFunctions


    Syntax:
        ``obj = h.StringFunctions()``


    Description:
        The StringFunctions class contains functions which you can apply to a \ ``strdef``.  This class 
        exists purely for the utility of preventing pollution of name space with string operations. 

    Example:

        .. code-block::
            python

            from neuron import h
            sf = h.StringFunctions() 


         

----



.. method:: StringFunctions.len


    Syntax:
        ``length = strobj.len(str)``


    Description:
        Return the length of a string. 

    Example: 
        .. code-block::
            python
    
            from neuron import h
            s = h.ref("hello")
            sf = h.StringFunctions()
            length = sf.len(s)
            print(length)
         

----



.. method:: StringFunctions.substr


    Syntax:
        ``index = strobj.substr(s1, s2)``


    Description:
        Return the index into *s1* of the first occurrence of *s2*. 
        If *s2* isn't a substring then the return value is -1. 

    Example:
        .. code-block::
            python

            from neuron import h
            s1 = h.ref("allowed")
            s2 = h.ref("low")
            sf = h.StringFunctions()
            index = sf.substr(s1, s2)
         

----



.. method:: StringFunctions.head


    Syntax:
        ``strobj.head(str, "regexp", result)``


    Description:
        The result contains the head of the string 
        up to but not including the *regexp*. returns index of 
        last char. 

    Example:
        .. code-block::
            python
        
            from neuron import h
            s1 = h.ref("hello world")
            s2 = h.ref("")
            sf = h.StringFunctions()
            index = sf.head(s1, "[e]", s2)
            print(s2[0])

         

----



.. method:: StringFunctions.tail


    Syntax:
        ``strobj.tail(str, "regexp", result)``


    Description:
        The result contains the tail of the string 
        from the char following *regexp* to the end of the string. 
        return index of first char. 
         
        Other functions can be added as needed, 
        eg., \ ``index(s1, c1)``, \ ``char(s1, i)``, etc. 
        without polluting the global name space. In recent versions 
        functions can return strings. 

    Example:
        .. code-block::
            python
        
            from neuron import h
            s1 = h.ref("hello world")
            s2 = h.ref("")
            sf = h.StringFunctions()
            index = sf.tail(s1, "[e]", s2)
            print(s2[0])


----



.. method:: StringFunctions.right


    Syntax:
        ``strobj.right(str, n)``


    Description:
        Removes first n characters from *str* and puts the result in 
        *str*.

    Example:
        .. code-block::
            python
        
            from neuron import h
            s = h.ref("hello")
            sf = h.StringFunctions()
            sf.right(s, 3)
            print(s[0])


         

----



.. method:: StringFunctions.left


    Syntax:
        ``.left(str, n)``


    Description:
        Removes all but first n characters from *str* and puts the 
        result in *str* 

    Example:
        .. code-block::
            python
        
            from neuron import h
            s = h.ref("hello")
            sf = h.StringFunctions()
            sf.left(s, 3)
            print(s[0])
             

----



.. method:: StringFunctions.is_name


    Syntax:
        ``.is_name(item)``


    Description:
        Returns True if the *item* is the name of a symbol, False otherwise. 
        This is so useful that the same thing is available with the top level 
        :func:`name_declared` function (except that returns 1 or 0 instead of True
        or False). 

    Example:
        .. code-block::
            python
    
            from neuron import h
            s1 = h.ref("hello world")
            sf = h.StringFunctions()
            name = sf.is_name(s1)
            print(name)


    Here is an example with one string that works, 
    and another that does not:
        .. code-block::
            python
        
            from neuron import h
            sf = h.StringFunctions()
            # valid name
            print(sf.is_name("xvalue"))
            # invalid name
            print(sf.is_name("xsquiggle"))
    
    .. note::

        This is approximately equivalent to ``item in dir(h)``.
----



.. method:: StringFunctions.alias


    Syntax:
        ``.alias(obj, "name", &var2)``

        ``.alias(obj, "name", obj2)``

        ``.alias(obj, "name")``

        ``.alias(obj)``


    Description:
        "name" becomes a public variable for obj and points to the 
        scalar var2 or object obj2. obj.name may be used anywhere the var2 or obj2 may 
        be used. With no third arg, the "name" is removed from the objects 
        alias list. With no second arg, the objects alias list is cleared. 

    Example:
        .. code-block::
            python

            from neuron import h
            sf = h.StringFunctions()
            v = h.Vector()
            sf.alias(v, 't', h._ref_t)
            print(f'v.t = {v.t}')
            h.t = 42
            print(f'v.t = {v.t}')

         

----



.. method:: StringFunctions.alias_list


    Syntax:
        ``list = sf.alias_list(obj)``


    Description:
        Return a new List object containing String objects which contain 
        the alias names. 

    .. warning::
        The String class is not a built-in class. It generally gets declared when 
        the nrngui.hoc file is loaded and lives in stdlib.hoc. 
        Note that the String class must exist and its 
        constructor must allow a single strdef argument. Minimally: 

    
    Example:
        .. code-block::
            python
    
            from neuron import h
            h.load_file('stdrun.hoc')
            sf = h.StringFunctions()
            v = h.Vector()
            al = sf.alias_list(v)
            print(al)

         

----



.. method:: StringFunctions.references


    Syntax:
        ``sf.references(object)``


    Description:
        Prints the number of references to the object and all objref names 
        that reference that object (including references via 
        :class:`HBox`, :class:`VBox`, and :class:`List`). It also prints the number of references found. 

    Example: 
        .. code-block::
            python

            from neuron import h
            s1 = h.Section(name='soma')
            strobj = h.StringFunctions()
            strobj.references(s1)


----



.. method:: StringFunctions.is_point_process


    Syntax:
        ``i = sf.is_point_process(object)``


    Description:
        Returns 0 if the object is not a POINT_PROCESS. Otherwise 
        returns the point type (which is always 1 greater than the index into the 
        :func:`MechanismType(1) <MechanismType>` list). 

    Example:
        .. code-block::
            python

            from neuron import h
            h.load_file('stdrun.hoc')
            s1 = h.Section(name='soma')
            syn = h.ExpSyn(s1(0.5))
            sf = h.StringFunctions()
            # not point process
            print(sf.is_point_process(s1))
            # point process
            print(sf.is_point_process(syn))
            c = h.IntFire1()
            # point process
            print(ssf.is_point_process(c))

----



.. method:: StringFunctions.is_artificial


    Syntax:
        ``i = sf.is_artificial(object)``


    Description:
        Returns 0 if the object is not an ARTIFICIAL_CELL. Otherwise 
        returns the point type (which is always 1 greater than the index into the 
        :func:`MechanismType(1) <MechanismType>` list). 

         

    Example:
        .. code-block::
            python

            from neuron import h
            h.load_file('stdrun.hoc')
            s1 = h.Section(name='soma')
            syn = h.ExpSyn(s1(0.5))
            # initiate string function
            sf = h.StringFunctions()
            c = h.IntFire1()
            # artificial 
            print(sf.is_artificial(c))
            # not artificial
            print(sf.is_artificial(syn))
