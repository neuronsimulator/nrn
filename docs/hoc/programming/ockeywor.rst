.. _ockeywor:

HOC Keywords
------------



.. index::  help (keyword)

.. _keyword_help:

**help**

         
    invokes the help system 
         

    Syntax:
        ``help``

        ``help word``



    Description:
        ``Help word`` sends a word to the help system. 
        The word is looked up in the :file:`nrn/lib/helpdict` file and if found 
        Netscape is sent the appropriate URL to display 
        the help text. If the word is not found, the URL for the table 
        of contents is sent. Netscape must be running for the help system 
        to work. 

         

----



.. index::  return (keyword)

.. _keyword_return:

**return**


    Syntax:
        ``return``

        ``return expr``

        ``return objref``



    Description:
        The ``return`` command will immediately exit from a procedure 
        without returning a value. 
         
        The ``return expr`` command will immediately exit from a function 
        which must return a value.  This command must also be the last executable 
        statement of a function.  It is possible for a function to contain more 
        than one ``return`` command, for instance in a series of ``if else`` 
        statements; however, not more than one of the ``return`` commands may 
        return a value at any given time. 
         
        The ``return objref`` command must be used to return from an 
        :ref:`obfunc`. 
         

    Example:

        .. code-block::
            none

            func max(){ 
            	if ($1 > $2){ 
            		return $1 
            	} else { 
            		return $2 
            	} 
            } 

        returns the maximum of two arguments which are read into the function.  Eg. ``max(3,6)``, where $1 is the 
        first argument (3) and $2 is the second argument (6).  This use of ``max`` would return the value 6. 

    .. warning::
        See restriction of the :ref:`break <keyword_break>` statement. 

         

----



.. index::  break (keyword)

.. _keyword_break:

**break**


    Syntax:
        ``break``



    Description:
        Immediately exit from a loop. Control transfers to the next statement after 
        the loop statement. 
         

    .. warning::
        This statement, as well as "return", "continue", and "stop" 
        cannot occur within the scope of a statement that 
        modifies the section stack such as 
         
        section { statement } 
         
        or the stack will not be properly popped. Also it should not be placed on 
        a line that contains object syntax but should be placed on a line by 
        itself. eg. 

        .. code-block::
            none

            	x.p() break 

        should be written 

        .. code-block::
            none

            	x.p() 
            	break 


    Example:

        .. code-block::
            none

            while(1) { 
            	x = fscan() 
            	if (x < 0) { 
            		break; 
            	} 
            	print sqrt(x) 
            } 


         
         

----



.. index::  continue (keyword)

.. _keyword_continue:

**continue**


    Syntax:
        ``continue``



    Description:
        Inside a compound statement of a loop, transfers control to the next iteration of the 
        loop statement. 
         

    Example:

        .. code-block::
            none

            for i=1,10{ 
            	if(i==6){ 
            		continue 
            	} 
            	print i 
            } 

        prints the numbers: 1,2,3,4,5,7,8,9,10.  6 is left out because when i==6, the control is passed 
        beyond the print statement to the next iteration of the loop. 
         
        You can accomplish the same thing with the following syntax: 

        .. code-block::
            none

            for i=1,10{ 
            	if(i<6 || i>6){ 
            		print i 
            	} 
            } 

         

    .. warning::
        See restriction of the :ref:`break <keyword_break>` statement. 

         

----



.. index::  stop (keyword)

.. _keyword_stop:

**stop**


    Syntax:
        ``stop``



    Description:
        Return control to the command level of the interpreter.  This is a useful safety device 
        for stopping the current execution 
        of your program.  Eg. you may wish to stop the program and print out an error message 
        that lets you know if you have entered unacceptable arguments. 

    .. warning::
        See restriction of the :ref:`break <keyword_break>` statement. 

         

----



.. index::  if (keyword)

.. _keyword_if:

**if**


    Syntax:
        ``if (expr) stmt1``

        ``if (expr) stmt1 else stmt2``


    Description:
        Conditional statement.  When the *expr* evaluates to a nonzero number 
        (true) stmt1 is executed.  With the ``else`` form, if the expression 
        evaluates to zero (false) stm2 is executed. 
         

    Example:

        .. code-block::
            none

            i = 0	//initialize i 
            j = 0	//initialize j 
            if(vec.x[i] <= 10 && i < vec.size()){	//if the value of the ith element in vec 
            					//is less than or equal to 10, and 
            					//if i is an index within vec 
            	vec1.x[j] = vec.x[i]		//set the jth element of vec1 equal to that 
            					//ith element of vec 
            	i = i+1				//increment i by 1 
            	j = j+1				//increment j by 1 
            } else{					//otherwise (This must be on the same line as the closing brace of 
            					//the previous statement in order to indicate that the compound  
            					//statement has not ended.) 
            	i = i+1				//simply go to the next element of vec 
            } 

         

    .. seealso::
        :data:`float_epsilon`, :ref:`ifsec <keyword_ifsec>`

         

----



.. index::  else (keyword)

.. _keyword_else:

**else**


    .. seealso::
        :ref:`if <keyword_if>`


----



.. index::  while (keyword)

.. _keyword_while:

**while**


    Syntax:
        ``while (expr) stmt``



    Description:
        Iteration statement.  Repeatedly execute the statement as long as the 
        *expr* evaluates to true. 
         

    Example:

        .. code-block::
            none

            numelements = 20 
            i = 0 
            while (i < numelements){ 
            	print(cos(vec.x[i])) 
            	print(sin(vec.x[i])) 
            	i += 1 
            } 

        prints the cosines and the sines of the ``vec`` elements up to ``numelements``, which in this case = 20. 
         

    .. seealso::
        :ref:`for <keyword_for>`, :ref:`break <keyword_break>`, :ref:`continue <keyword_continue>`
        

         

----



.. index::  for (keyword)

.. _keyword_for:

**for**


    Syntax:
        ``for(stmt1; expr2; stmt3) stmt``

        ``for var=expr1, expr2  stmt``

        ``for (var) stmt``

        ``for (var, expr) stmt``

        ``for iterator (args) stmt``



    Description:
        Iteration statement.  The ``for`` statement is similar to ``while`` in that it iterates over 
        a statement.  However, the ``for`` statement is more compact and contains within its parentheses 
        the command to advance to the next iteration.  Statements 1 and 3 may be 
        empty. 
         
        This command also has a short form which always increments the iterations by one. 

        .. code-block::
            none

            for *var*=*expr1*, *expr2*  stmt 

        is equivalent to 

        .. code-block::
            none

            for(*var*=*expr1*; *var* <= *expr2*; *var*=*var*+1) stmt 

        However, *expr1* and *expr2* are evaluated only once at the 
        beginning of the ``for``. 
         
        ``for (var) stmt`` 
         
        Loops over all segments of the currently accessed section.  *var* begins 
        at 0 and ends at 1.  In between *var* is set to the center position of 
        each segment.  Ie.  stmt is executed nseg+2 times. 
         
        ``for (var, expr) stmt`` 
         
        If the expression evaluates to a non-zero value, it is exactly equivalent 
        to 
        ``for (var) stmt`` 
        If it evaluates to 0 (within :data:`float_epsilon` ) then the iteration does 
        not include the 0 or 1 points. Thus ``for(x, 0) { print x }`` 
        is exactly equivalent to ``for (x) if (x > 0 && x < 1) { print x }`` 
         
        The :ref:`keyword_iterator` form of the for loop executes the statement with a looping 
        construct defined by the user. 

    Example:

        .. code-block::
            none

            for(i=0; i<=9; i=i+1){ 
            	print i*2 
            } 

        is equivalent to 

        .. code-block::
            none

            for i=0, 9 { 
            	print i*2 
            } 


        .. code-block::
            none

            create axon 
            access axon 
            {nseg = 5  L=1000  diam=50  insert hh } 
            for (x) print x, L*x 
            for (x) if (x > 0 && x < 1) { print x, gnabar_hh(x) } 


    .. seealso::
        :ref:`keyword_iterator`,
        :ref:`break <keyword_break>`, :ref:`continue <keyword_continue>`, :ref:`while <keyword_while>`, :ref:`forall <keyword_forall>`, :ref:`forsec <keyword_forsec>`

         
         

----



.. index::  print (keyword)

.. _keyword_print:

**print**


    Syntax:
        ``print expr, string, ...``



    Description:
        Any number of expressions and/or strings may be printed.  A newline is 
        printed at the end. 
         

    Example:

        .. code-block::
            none

            x=2 
            y=3 
            print x, "hello", "good-bye", y, 7 

        prints 

        .. code-block::
            none

            x hello good-bye 3 7 

        and then moves to the next line. 
         

         

----



.. index::  delete (keyword)

.. _keyword_delete:

**delete**


    Syntax:
        ``delete varname``



    Description:
        Deletes the variable name from the global namespace.  Allows the 
        varname to be declared as another type.  It is up to the user to make 
        sure it is safe to execute this statement since the variable may be used 
        in an existing function. 
         

         

----



.. index::  read (keyword)

.. _keyword_read:

**read**


    Syntax:
        ``read(var)``



    Description:
        *var* is assigned the number input by the user, or the next number in the 
        standard input, or the file opened with ropen.  ``read(var)`` 
        returns 0 on 
        end of file and 1 otherwise. 
         

    Example:

        .. code-block::
            none

            for i=1, 5 { 
            	read(x) 
            	print x*x 
            } 

        will await input from the user or from a file, and will print the square of each value typed in 
        by the user, or read from the file, for the first five values. 
         

    .. seealso::
        :func:`xred`, :meth:`File.ropen`, :func:`fscan`, :func:`File`, :func:`getstr`
        

         

----



.. index::  debug (keyword)

.. _keyword_debug:

**debug**

        A toggle for parser debugging purposes. Prints the stack machine commands 
        resulting from parsing a statement.  Not useful to the user. 
         

----



.. index::  double (keyword)

.. _keyword_double:

**double**


    Syntax:
        ``double var1[expr]``

        ``double var2[expr1][expr2]``

        ``double varn[expr1][expr2]...[exprn]``



    Description:
        Declares a one-dimensional, a two-dimensional or an n-dimensional array of doubles. 
        This is reminiscent of the command which creates an array in C, however, HOC does not demand 
        that you specify whether or not numbers are integers.  All numbers in all arrays will be 
        doubles. 
         
        The index for each dimension ranges from 0 to expr-1.  Arrays may be 
        redeclared at any time, including within procedures.  Thus arrays may 
        have different lengths in different objects. 
         
        The :class:`Vector` class for the ivoc interpreter provides convenient and powerful methods for 
        manipulating arrays. 
         

    Example:

        .. code-block::
            none

            double vec[40] 

        declares an array with 40 elements, whereas 

        .. code-block::
            none

            objref vec 
            vec = new Vector(40) 

        creates a vector (which is an array by a different name) with 40 elements which you can 
        manipulate using the commands of the Vector class. 
         

         

----



.. index::  em (keyword)

.. _keyword_em:

**em**


    Syntax:
        ``em``



    Description:
        microemacs editor 
         
        This is a reasonably complete editor with many commands.  These commands 
        are listed in 
        `emacs <http://www.neuron.yale.edu/neuron/static/docs/help/emacs.txt>`_.
        A tutorial is also available at 
        `emacstut <http://www.neuron.yale.edu/neuron/static/docs/help/emacstut.txt>`_.
         
        When 
        called from the interpreter, the command :kbd:`^C` immediately returns to the 
        interpreter and the current buffer is interpreted. 
        Other commands follow: 


        :kbd:`^X^F` 
            reads a file into a new buffer. 

        :kbd:`^X^B` 
            changes buffers. 

        :kbd:`^X^W` *filename* 
            saves (writes) a file under a specific name. 

        :kbd:`^X^S`
            saves a file under the last specified name. 

         
         

----



.. index::  depvar (keyword)

.. _keyword_depvar:

**depvar**


    Syntax:
        ``depvar``



    Description:
        Declare a variable to be a dependent variable for the purpose of 
        solving simultaneous equations. 
         

    Example:

        .. code-block::
            none

            depvar x, y, z 
             proc equations() { 
               eqn x:: x + 2*y + z =  6 
               eqn y:: x - y + z   =  2 
               eqn z:: 2*x + y -z  = -3 
             } 
            equations() 
            solve() 
            print x,y,z 

        prints the values of x, y and z. 
         

    .. seealso::
        :ref:`eqn <keyword_eqn>`, :func:`eqinit`, :func:`solve`, :func:`Matrix`
        

         

----



.. index::  eqn (keyword)

.. _keyword_eqn:

**eqn**


    Syntax:
        ``eqn var:: expr = expr``

        ``eqn var: expr =``

        ``eqn var: = expr``


    Description:
        Introduce a simultaneous equation. 
        The single colon forms add the expressions to the indicated sides.  This is convenient for breaking 
        long equations down into more manageable parts which can be added together. 
         

    Example:

        .. code-block::
            none

            eqinit() 
            depvar x, y, z 
             proc equations() { 
               eqn x:: x + 2*y + z =  6 
               eqn y:: x - y + z   =  2 
               eqn z:: 2*x + y -z  = -3 
               eqn z: = 5 + 4y 
             } 
            equations() 
            solve() 
            print x,y,z 

        makes the right hand side of the z equation "2 + 4y" and solves for the values x, y, and z. 
         

         

----



.. index::  local (keyword)

.. _keyword_local:

**local**


    Syntax:
        ``local var``



    Description:
        Declare a list of local variables within a procedure or function 
        Must be the first statement on the same line as the function declaration. 
         

    Example:

        .. code-block::
            none

            func count() {local i, x 
            	x = 0 
            	for i=0,40 { 
            		if (vec.x[i] == 7) { 
            			 x = x+1 
            		} 
            	} 
            	return x 
            } 

        returns the number of elements which have the value of 7 in the first 40 elements of ``vec``. ``i`` 
        and ``x`` are local variables, and their usage here will not affect variables of the same name in 
        other functions and procedures of the same program. 
         

----



.. index::  localobj (keyword)

.. _keyword_localobj:

**localobj**


    Syntax:
        ``localobj var``


    Description:
        Declare a list, comma separated, of local objrefs within a proc, func, iterator, or obfunc. 
        Must be after the :ref:`local <keyword_local>` statement (if that exists) 
        on the same line as the function declaration 

    Example:

        .. code-block::
            none

            func sum() { local i, j  localobj tobj // sum from $1 to $2 
            	i = $1  j = $2 
            	tobj = new Vector() 
            	tobj.indgen(i, j ,1) 
            	return tobj.sum 
            } 
            sum(5, 10) == 45 


         

----



.. index::  strdef (keyword)

.. _keyword_strdef:

**strdef**


    Syntax:
        ``strdef stringname``



    Description:
        Declare a comma separated list of string variables.  String 
        variables cannot be arrays. 
         
        Strings can be passed as arguments to functions. 
         

    Example:

        .. code-block::
            none

            strdef a, b, c 
            a = "Hello, " 
            b = "how are you?" 
            c = "What is your name?" 
            print a, b 
            print c 

        will print to the screen: 

        .. code-block::
            none

            Hello, how are you? 
            What is your name? 

         

         

----



.. index::  setpointer (keyword)

.. _keyword_setpointer:

**setpointer**


    Syntax:
        ``setpointer pvar, var``



    Description:
        Connects pointer variables in membrane mechanisms to the address of var. 
        eg. If :file:`$NEURONHOME/examples/nmodl/synpre.mod` is linked into NEURON, then: 

        .. code-block::
            none

            soma1 syn1=new synp(.5) 
            setpointer syn1.vpre, axon2.v(1) 

        would enable the synapse in soma1 to observe the axon2 membrane potential. 

         

----



.. index::  insert (keyword)

.. _keyword_insert:

**insert**


    Syntax:
        ``insert mechanism``



    Description:
        Insert the density mechanism in the currently accessed section. 
        Not used for point processes--they are inserted with a different syntax. 
         

    .. seealso::
        :ref:`hh <mech_hh>`, :ref:`pas <mech_pas>`, :ref:`fastpas <mech_fastpas>`, :func:`psection`, :ref:`mech`
        

         

----



.. index::  uninsert (keyword)

.. _keyword_uninsert:

**uninsert**


    Syntax:
        ``uninsert mechanism``



    Description:
        Delete the indicated mechanism from the currently accessed section. Not for 
        point processes. 
         

         

