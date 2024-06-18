.. index::  syntax (HOC)


.. _hoc_ocsyntax:

HOC Syntax
----------

HOC was the original programming language supported by NEURON.
NEURON may also be programmed in Python.


.. index::  comments

comments
~~~~~~~~
Syntax:
    ``/*...*/``

    ``//...``



Description:
    Comments are similar in style to C++. Enclose any text in /*...*/ (but 
    do not nest them).	The rest of a line after // is a comment. 
         



.. index::  expression

Expressions
~~~~~~~~~~~

Description:
    An expression has a double precision value.  It usually appears as the right 
    hand side of an assignment statement.  An expression may be a number, 
    variable, function call, or combination of simpler expressions. 
     

Options:
    The ways in which expressions can be combined are listed below 
    in order of precedence. e stands for any expression, v stands for any variable 
    and operators are 
    left associative except for assignment operators which are right associative. 


    ``(e)`` 
        grouping 

    ``e^e`` 
        exponentiation 

    ``-e`` 
        negation 

    ``e*e  e/e  e%e`` 
        multiplication, division, modulus 

    ``e+e  e-e`` 
        addition, subtraction 

    ``e==e  e!=e  e<e  e<=e  e>e  e>=e`` 
        logical equal, unequal, less than, less than or equal, greater than, 
        greater than or equal. These expressions have the numerical 
        value 1 (true) or 0 (false). The expression is considered true if it is 
        within :hoc:data:`float_epsilon` of being mathematically exact.
         
        Special logical expressions of the form objref1 == objref2 (and obj != obj) 
        are also allowed and return 1 (0) if the object references label the same 
        object. This makes the former comparison idiom using 
        :hoc:func:`object_id` obsolete. Logical expressions of the strdef1 == strdef2
        cannot be directly compared because of parser consistency reasons. However 
        obj1.string1 == obj2.string2 will return true if the strings are identical 
        in the sense of :hoc:func:`strcmp` .
         
         

    ``e&&e`` 
        Logical and. Both expressions 
        are always evaluated. A subexpression is considered false if it is within 
        :hoc:data:`float_epsilon` of 0 and true otherwise. If the entire expression is true
        its value is 1. 

    ``e||e`` 
        Logical or. Both expressions are always evaluated. 
        A subexpression is considered false if it is within 
        :hoc:data:`float_epsilon` of 0 and true otherwise. If the entire expression is true
        its value is 1. 

    ``v=e  v+=e  v-=e  v*=e  v/=e`` 
        assignment. others are equivalent to ``v = (v + e)``, 
        ``v = (v - e)``, 
        ``v = (v * e)``, 
        ``v = (v / e)``, respectively. 

     

.. seealso::
    :hoc:data:`float_epsilon`
        

         
  .. index::  statement       

Statements
~~~~~~~~~~

Syntax:
    ``stmt``

    ``{stmt}``

    ``{stmt stmt ...stmt}``



Description:
    A statement is something executable that does not have a value, eg. 
    for loops, procedure calls, or a compound statement between braces. 
    An expression may be used anywhere a statement is required. 
     

Example:

    .. code-block::
        none

        i = 0	//initialize i 
        j = 0	//initialize j 
        if(vec.x[i] <= 10 && i < vec.size()){	//In the parentheses is an expression: 
        					//if the value of the ith element in vec 
        					//is less than or equal to 10, and 
        					//if i is an index within vec 
        					// 
        					//Between the braces is/are statement(s): 
        	vec1.x[j] = vec.x[i]		 
        	i = i+1				//increment i by 1 
        	j = j+1				//increment j by 1 
        } else{					 
        					//Here is also a statement 
        	i = i+1				//simply go to the next element of vec 
        } 

    Statements exist between the braces following the ``if`` and ``else`` commands. 
    The parentheses after the ``if`` command contain an expression. 
     

         
.. index::  proc
         

.. _hoc_proc:


proc
~~~~
Syntax:
    :samp:`proc {name}() stmt`



Description:
    Introduce the definition of a procedure. A procedure does not return a value. 
    You should always try to distill your programs into small, manageable 
    procedures and functions. 
     

Example:

    .. code-block::
        none

        proc printsquare() {local x 
           x = $1 
           print x*x 
         } 
        printsquare(5) 

    prints the square of 5. 
     
    Procedures can also be called within other procedures. 
    The code which produces the interactive examples for the :hoc:class:`Random` class contains procedures
    for both creating the buttons which allow you to select parameters as well as for creating 
    the histograms which appear on the screen. 
         

         
.. index::  func


.. _hoc_func:

func
~~~~

         

Syntax:
    :samp:`func {name}() {{stmt1, stmt2, stmt3...}}`



Description:
    Introduce the definition of a function. 
    A function returns a double precision value. 
     

Example:

    .. code-block::
        none

         func tan() {  
        	return sin($1)/cos($1)  
         } 
         tan(PI/8) 

    creates a function ``tan()`` which takes one argument (floating point 
    or whole number), and contains one 
    statement. 
     

         

.. index::  obfunc


.. _hoc_obfunc:

obfunc
~~~~~~

Syntax:
    :samp:`obfunc {name}() {{ statements }}`


Description:
    Introduce the definition of a function returning an objref 

Example:

    .. code-block::
        none

        obfunc last() { // arg is List 
        	return $o1.object($o1.count - 1) 
        } 


.. seealso::
    :ref:`localobj <hoc_keyword_localobj>`, :ref:`return <hoc_keyword_return>`

     

.. index::  iterator


.. _hoc_keyword_iterator:

iterator
~~~~~~~~

     

Syntax:
    ``iterator name() stmt``



Description:
    Define a looping construct to be used subsequently in looping 
    over a statement. 
     

Example:

    .. code-block::
        none

        iterator case() {local i 
        	for i = 2, numarg() {		//must begin at 2 because the first argument is 
        					//in reference to the address 
        		$&1 = $i		//what is at the address will be changed 
        		*iterator_statement*	//This is where the iterator statement will 
        					//be executed. 
        	} 
        } 

    In this case 

    .. code-block::
        none

        x=0 
        for case (&x, 1,2,4,7,-25) { 
        	print x			//the iterator statement 
        } 

    will print the values 1, 2, 4, 7, -25 
     
    The body of the ``for name(..) statement`` is executed in the same 
    context as a normal for statement. The name is executed in the same 
    context as a normal procedure but should use only variables local to the 
    iterator. 
     

         
         

.. index::  arguments


.. _hoc_arguments:

Arguments
~~~~~~~~~

Arguments to functions and procedures are retrieved positionally. 
``$1, $2, $3`` refer to the first, second, and third scalar arguments 
respectively. 
 
If "``i``" is declared as a local variable, ``$i`` refers 
to the scalar argument in the position given by the value of ``i``. 
The value of ``i`` must be in the 
range {1...numarg()}. 
 
The normal idiom is

    ``for i=1, numarg()  {print $i}`` 

Scalar arguments use call by value so the variable in the calling 
statement cannot be changed. 
 
If the calling statement has a '&' 
prepended to the variable then it is passed by reference and must 
be retrieved with the 
syntax ``$&1, $&2, ..., $&i``. If the variable passed by reference 
is a one dimensional array then ``$&1`` refers to the first (0th) element 
and index i is denoted ``$&1[i]``. Warning, NO array bounds checking is 
done and the array is treated as being one-dimensional. A scalar or 
array reference may be passed to another procedure with 
``&$&1``. To save a scalar reference use the :hoc:class:`Pointer` class.
 
Retrieval of strdef arguments uses the syntax: ``$s1, $s2, ..., $si``. 
Retrieval of objref arguments uses the syntax: ``$o1, $o2, ..., $oi``. 
Arguments of type :ref:`strdef <hoc_keyword_strdef>` and ``objref`` use call by reference so the calling
value may be changed. 

Example:

    .. code-block::
        none

        func mult(){ 
        	return $1*$2 
        } 

    defines a function which multiplies two arguments. 
    Therefore ``mult(4,5)`` will return the value 20. 

    .. code-block::
        none

        proc pr(){ 
        	print $s3 
        	print $1*$2 
        	print $o4 
        } 

    defines a procedure which first prints the string defined in 
    position 3, then prints the product of the two numbers in 
    positions 1 and 2, and finally prints the pointer reference to an 
    object in position 4. 
     
    For a string '``s``' which is defined as ``s = "hello"``, and an 
    objref '``r``', ``pr(3,5,s,r)`` will return 

    .. code-block::
        none

        hello 
        15 
        Graph[0]   

    assuming ``r`` refers to the first graph. 

.. seealso::
    :ref:`hoc_func`, :ref:`hoc_proc`, :ref:`hoc_objref`, :ref:`strdef <hoc_keyword_strdef>`, :hoc:class:`Pointer`, :hoc:func:`numarg`, :hoc:func:`argtype`

----

.. hoc:function:: numarg

    Syntax:
        ``n = numarg()``

    Description:
        Number of arguments passed to a user written hoc function. 

    .. seealso::
        :ref:`hoc_arguments`, :hoc:func:`argtype`
         

----

.. hoc:function:: argtype

    Syntax:
        ``itype = argtype(iarg)``

    Description:
        The type of the ith arg. The return value is 0 for numbers, 1 for objref, 
        2 for strdef, 3 for pointers to numbers, and -1 if the arg does not exist. 

    .. seealso::
        :ref:`hoc_arguments`, :hoc:func:`numarg`

     
     
     

