.. _oop:

oop
---



.. function:: ObjectOrientedProgramming

        See <a href="../../../refman/obj.html">Object Oriented Programming</a> in the 
        reference manual. 

----



.. function:: begintemplate


    Syntax:
        :code:`begintemplate`



    Description:
        Declare a new class or data structure. Any HOC code may appear between the 
        \ :code:`begintemplate` and \ :code:`endtemplate` declarations. Classes are instantiated with 
        the new statement. 
         

    Example:

        .. code-block::
            none

            begintemplate String 
            public s 
            strdef s 
             proc init() { 
               if (numarg()) { 
                 s = $s1 
               } 
             } 
            endtemplate String 
            objref s 
            s = new String("Hello") 
            print s.s 

        will print "Hello" to the screen. 

         

----



.. function:: endtemplate


    Syntax:
        :code:`endtemplate`


    Description:
        Closes the class declaration 

    .. seealso::
        :func:`begintemplate`

         

----



.. function:: objectvar


    Syntax:
        :code:`objectvar`


    Description:
        Synonym for :func:`objref` . 

         

----



.. function:: objref


    Syntax:
        :code:`objref`



    Description:
        A comma separated list declarations of object variables.  Object 
        variables are labels (pointers, references) to the actual objects.  Thus \ :code:`o1 = o2` 
        merely states that o1 and o2 are labels for the same object.  Objects are 
        created with the \ :code:`new` statement.  When there are no labels for an object 
        the object is deleted. The keywords \ :code:`objectvar` and \ :code:`objref` are synonyms. 
         
        An object has a unique name that can be determined with the \ :code:`print obj` statement 
        and consists of the template name followed by an index number in brackets. 
        This name can be used in place of an objref. 
         

    Example:

        .. code-block::
            none

            objref vec, g 
            vec = new Vector(20) 
            g = new Graph() 

        creates a vector object and a graph object with pointers named vec and g, respectively. 
         

    .. seealso::
        :func:`new`, :func:`begintemplate`, :func:`List`, :func:`pointprocesses`, :func:`SectionList`
        

         

----



.. function:: public


    Syntax:
        :code:`public`



    Description:
        A comma separated list of all the names in a class that are available 
        outside the class. 
         

    .. seealso::
        :func:`begintemplate`

         

----



.. function:: external


    Syntax:
        :code:`external`



    Description:
        A comma separated list of functions, procedures, iterators, objects, 
        strings, or variables defined at the top 
        level that can be executed within this class.  This statement is 
        optional but if it exists must follow the begintemplate or public line. 
        This allows an object to get information from the outside and can 
        be used as information shared by all instances. External iterators 
        can only use local variables and arguments. 

    Example:

        .. code-block::
            none

            global_ra = 100 
             func ra_value() {return global_ra} 
            begintemplate Cell 
             external ra_value 
             create axon 
             proc init() { 
            	forall Ra = ra_value()	/* just the axon */ 
             } 
            endtemplate Cell 

         
        :func:`execute1` can be used to obtain external information as well. 
         

         

----



.. function:: new


    Syntax:
        :code:`objectvariable = new Object(args)`



    Description:
        Creates a new object/instance of type/class Object and makes 
        objectvariable label/point to it. 
        When the object no longer is pointed to, it no longer exists. 
         

    Example:

        .. code-block::
            none

            objref vec 
            vec = new Vector(30) 

        creates a vector of size 30 with its pointer named \ :code:`vec`. 
         

         

----



.. function:: init


    Syntax:
        :code:`proc init() { ... }`


    Description:
        If an init procedure is defined in a template, then it is called whenever 
        an instance of the template is created. 

    .. seealso::
        :func:`new`

         

----



.. function:: unref


    Syntax:
        :code:`proc unref() { print this, " refcount=", $1 }`


    Description:
        If an unref procedure is defined in a template, then it is called whenever 
        the reference count of an object of that type is decremented. The reference 
        count is passed as the argument. When the count is 0, the object will be 
        destroyed on return from unref. This is useful in properly managing 
        objects which mutually reference each other. Note that unref may be 
        called recursively. 

         
         

----



.. function:: NULLobject


    Syntax:
        :code:`objref nil`


    Description:
        When an object variable is first declared, it refers to NULLobject 
        until it has been associated with an instance of some object class 
        by a :meth:`ObjectOrientedProgramming.new` statement. 
        A NULLobject object variable can 
        be useful as an argument to certain class methods. 

    Example:

        .. code-block::
            none

            objref nil 
            print nil  // prints NULLobject 


         

----



.. function:: this


    Syntax:
        :code:`objref this`


    Description:
        Declared inside a template 
        (see :meth:`ObjectOrientedProgramming.begintemplate` ). 
        Allows the object to call a procedure 
        with itself as one of the arguments. 

    Example:

        .. code-block::
            none

            begintemplate Demothis 
               public printname 
               objref this 
             
               proc init() { 
                 printname() 
               } 
             
               proc printname() { 
                 print "I am ", this 
               } 
            endtemplate Demothis 
             
            objref foo[3] 
            print "at creation" 
            for i=0,2 foo[i]=new Demothis() 
            print "check existing" 
            for i=0,2 foo[i].printname() 


         
         
         
         
         
         
         
         
         
         
         
         
         
         
         
         
         
         
         
         

