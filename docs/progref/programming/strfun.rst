.. _strfun:

StringFunctions (String Manipulation Class)
-------------------------------------------



.. class:: StringFunctions


    Syntax:
        ``sf = n.StringFunctions()``


    Description:
        The StringFunctions class contains functions which you can apply to a ``strdef``.  This class 
        exists purely for the utility of preventing pollution of name space with string operations. 

    Example:

        .. code-block::
            python

            from neuron import n
            sf = n.StringFunctions() 


         

----



.. method:: StringFunctions.len


    Syntax:
        ``length = sf.len(str)``


    Description:
        Return the length of a string. Works with both NEURON string references and
        regular Python strings.

    Example: 
        .. code-block::
            python
    
            from neuron import n
            sf = n.StringFunctions()
            length = sf.len("hello")
            print(length)
    
    .. note::

        This is approximately equivalent to ``len(s)`` for Python strings
        and to ``len(s[0])`` for references to NEURON strings, but it uses
        the same syntax for both types of strings.


----



.. method:: StringFunctions.substr


    Syntax:
        ``index = sf.substr(s1, s2)``


    Description:
        Return the index into ``s1`` of the first occurrence of ``s2``. 
        If ``s2`` isn't a substring then the return value is -1. 
        The arguments can be either NEURON string references or regular Python strings.


    Example:
        .. code-block::
            python

            from neuron import n
            s1 = n.ref("allowed")
            s2 = n.ref("low")
            sf = n.StringFunctions()
            index = sf.substr(s1, s2)
    
    .. note::

        When working with pure Python strings (not NEURON string references), the return value is the same as
        ``s1.find(s2)``. e.g., ``sf.substr("allowed", "low")`` is equivalent to ``"allowed".find("low")`` and
        both return ``2``.
         

----



.. method:: StringFunctions.head


    Syntax:
        ``sf.head(str, "regexp", result)``


    Description:
        The result contains the head of the string 
        up to but not including the *regexp*. returns index of 
        last char. 

    Example:
        .. code-block::
            python
        
            from neuron import n
            s1 = n.ref("hello world")
            s2 = n.ref("")
            sf = n.StringFunctions()
            index = sf.head(s1, "[e]", s2)
            print(s2[0])

    .. seealso::
        
        Python's regular expression module ``re``.
         

----



.. method:: StringFunctions.tail


    Syntax:
        ``sf.tail(str, "regexp", result)``


    Description:
        The result contains the tail of the string 
        from the char following *regexp* to the end of the string. 
        return index of first char. 
         
        Other functions can be added as needed, 
        e.g., ``index(s1, c1)``, ``char(s1, i)``, etc. 
        without polluting the global name space. In recent versions 
        functions can return strings. 

    Example:
        .. code-block::
            python
        
            from neuron import n
            s1 = n.ref("hello world")
            s2 = n.ref("")
            sf = n.StringFunctions()
            index = sf.tail(s1, "[e]", s2)
            print(s2[0])


----



.. method:: StringFunctions.right


    Syntax:
        ``sf.right(str, n)``


    Description:
        Removes first n characters from the NEURON string ``str`` and puts the result
        back in ``str``. This cannot be used with regular Python strings
        because they are immutable.

    Example:
        .. code-block::
            python
        
            from neuron import n
            s = n.ref("hello")
            sf = n.StringFunctions()
            sf.right(s, 3)
            print(s[0])  # prints: "lo"

    .. note::

        This is approximately equivalent to ``s = s[n:]`` for Python strings
        except that it modifies the NEURON string in place. That is, ``sf.right(s, 3)``
        always changes the value of ``s``, while ``s = s[n:]`` creates a new string
        and assigns it to ``s``, but it could be assigned to any other variable and
        leave the original string unchanged.
         

----



.. method:: StringFunctions.left


    Syntax:
        ``sf.left(str, n)``


    Description:

        Removes all but the first n characters from the NEURON string ``str`` and puts
        the result back in ``str``. This cannot be used with regular Python strings
        because they are immutable.

    Example:
        .. code-block::
            python
        
            from neuron import n
            s = n.ref("hello")
            sf = n.StringFunctions()
            sf.left(s, 3)
            print(s[0])  # prints "hel"
    
    .. note::

        This is approximately equivalent to ``s = s[:n]`` for Python strings
        except that it modifies the NEURON string in place. That is, ``sf.left(s, 3)``
        always changes the value of ``s``, while ``s = s[:n]`` creates a new string
        and assigns it to ``s``, but it could be assigned to any other variable and
        leave the original string unchanged.


----



.. method:: StringFunctions.is_name


    Syntax:
        ``sf.is_name(item)``


    Description:
        Returns ``True`` if the ``item`` is the name of a symbol, ``False`` otherwise. 
        This is so useful that the same thing is available with the top level 
        :func:`name_declared` function (except that returns 1 or 0 instead of True
        or False). 

    Example:
        .. code-block::
            python
    
            from neuron import n
            s1 = n.ref("hello world")
            sf = n.StringFunctions()
            name = sf.is_name(s1)
            print(name)


    Here is an example with one string that works, 
    and another that does not:
        .. code-block::
            python
        
            from neuron import n
            sf = n.StringFunctions()
            # valid name
            print(sf.is_name("xvalue"))
            # invalid name
            print(sf.is_name("xsquiggle"))
    
    .. note::

        This is approximately equivalent to ``item in dir(h)`` but the Python module
        ``h`` contains additional names that are not NEURON symbols per se.
----



.. method:: StringFunctions.alias


    Syntax:
        ``sf.alias(obj, "name", _ref_var2)``

        ``sf.alias(obj, "name", obj2)``

        ``sf.alias(obj, "name")``

        ``sf.alias(obj)``


    Description:
        "name" becomes a public variable for obj and points to the 
        scalar pointed at by ``_ref_var2`` or object obj2. obj.name may be used anywhere the var2 or obj2 may 
        be used. With no third arg, the "name" is removed from the objects 
        alias list. With no second arg, the objects alias list is cleared. 

    Example:
        .. code-block::
            python

            from neuron import n
            sf = n.StringFunctions()
            v = n.Vector()
            sf.alias(v, 't', n._ref_t)
            print(f'v.t = {v.t}')
            n.t = 42
            print(f'v.t = {v.t}')

         

----



.. method:: StringFunctions.alias_list


    Syntax:
        ``listobj = sf.alias_list(obj)``


    Description:
        Return a new List object containing String objects which contain 
        the alias names. 

    .. warning::
        The String class is not a built-in class. It generally gets declared when 
        ``gui`` is imported or ``stdrun.hoc`` is loaded.
        Note that the String class must exist and its 
        constructor must allow a single strdef argument. Minimally: 

    
    Example:
        .. code-block::
            python
    
            from neuron import n
            n.load_file('stdrun.hoc')
            sf = n.StringFunctions()
            v = n.Vector()
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

            from neuron import n
            soma = n.Section('soma')
            sf = n.StringFunctions()
            sf.references(soma)


----



.. method:: StringFunctions.is_point_process


    Syntax:
        ``i = sf.is_point_process(object)``


    Description:
        Returns 0 if the object is not a POINT_PROCESS. Otherwise 
        returns the point type (which is always 1 greater than the index into the 
        :func:`MechanismType(1) <MechanismType>` list). In particular, the return
        value is an integer, not a boolean, because it indicates position in a list.

    Example:
        .. code-block::
            python

            from neuron import n
            n.load_file('stdrun.hoc')
            s1 = n.Section('soma')
            syn = n.ExpSyn(s1(0.5))
            sf = n.StringFunctions()
            # not point process
            print(sf.is_point_process(s1))
            # point process
            print(sf.is_point_process(syn))
            c = n.IntFire1()
            # point process
            print(ssf.is_point_process(c))

----



.. method:: StringFunctions.is_artificial


    Syntax:
        ``i = sf.is_artificial(object)``


    Description:
        Returns 0 if the object is not an ARTIFICIAL_CELL. Otherwise 
        returns the point type (which is always 1 greater than the index into the 
        :func:`MechanismType(1) <MechanismType>` list). In particular, the return
        value is an integer, not a boolean, because it indicates position in a list.

         

    Example:
        .. code-block::
            python

            from neuron import n
            n.load_file('stdrun.hoc')
            s1 = n.Section('soma')
            syn = n.ExpSyn(s1(0.5))
            # initiate string function
            sf = n.StringFunctions()
            c = n.IntFire1()
            # artificial 
            print(sf.is_artificial(c))
            # not artificial
            print(sf.is_artificial(syn))
