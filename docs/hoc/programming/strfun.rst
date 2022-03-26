
.. _hoc_strfun:

StringFunctions (String Manipulation Class)
-------------------------------------------



.. hoc:class:: StringFunctions


    Syntax:
        ``obj = new StringFunctions()``


    Description:
        The StringFunctions class contains functions which you can apply to a \ ``strdef``.  This class 
        exists purely for the utility of preventing pollution of name space with string operations. 

    Example:

        .. code-block::
            none

            objref strobj 
            strobj = new StringFunctions() 


         

----



.. hoc:method:: StringFunctions.len


    Syntax:
        ``length = strobj.len(str)``


    Description:
        Return the length of a string. 

         

----



.. hoc:method:: StringFunctions.substr


    Syntax:
        ``index = strobj.substr(s1, s2)``


    Description:
        Return the index into *s1* of the first occurrence of *s2*. 
        If *s2* isn't a substring then the return value is -1. 

         

----



.. hoc:method:: StringFunctions.head


    Syntax:
        ``strobj.head(str, "regexp", result)``


    Description:
        The result contains the head of the string 
        up to but not including the *regexp*. returns index of 
        last char. 

         

----



.. hoc:method:: StringFunctions.tail


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

         

----



.. hoc:method:: StringFunctions.right


    Syntax:
        ``strobj.right(str, n)``


    Description:
        Removes first n characters from *str* and puts the result in 
        *str*. 

         

----



.. hoc:method:: StringFunctions.left


    Syntax:
        ``.left(str, n)``


    Description:
        Removes all but first n characters from *str* and puts the 
        result in *str* 

         

----



.. hoc:method:: StringFunctions.is_name


    Syntax:
        ``.is_name(str)``


    Description:
        Returns 1 if the *str* is the name of a symbol, 0 otherwise. 
        This is so useful that the same thing is available with the top level 
        :hoc:func:`name_declared` function.

         

----



.. hoc:method:: StringFunctions.alias


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

         

----



.. hoc:method:: StringFunctions.alias_list


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

        .. code-block::
            none

            begintemplate String 
            public s 
            strdef s 
            proc init() { s = $s1 } 
            endtemplate String 


         

----



.. hoc:method:: StringFunctions.references


    Syntax:
        ``sf.references(object)``


    Description:
        Prints the number of references to the object and all objref names 
        that reference that object (including references via 
        :hoc:class:`HBox`, :hoc:class:`VBox`, and :hoc:class:`List`). It also prints the number of references found.

         

----



.. hoc:method:: StringFunctions.is_point_process


    Syntax:
        ``i = sf.is_point_process(object)``


    Description:
        Returns 0 if the object is not a POINT_PROCESS. Otherwise 
        returns the point type (which is always 1 greater than the index into the 
        :hoc:func:`MechanismType(1) <MechanismType>` list).

         

----



.. hoc:method:: StringFunctions.is_artificial


    Syntax:
        ``i = sf.is_artificial(object)``


    Description:
        Returns 0 if the object is not an ARTIFICIAL_CELL. Otherwise 
        returns the point type (which is always 1 greater than the index into the 
        :hoc:func:`MechanismType(1) <MechanismType>` list).

         

