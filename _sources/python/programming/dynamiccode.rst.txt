Dynamic Code Loading and Execution
==================================

These functions either execute HOC code contained in an arbitrary, possibly
generated-at-runtime string or load HOC code from a file potentially chosen at runtime.

.. note::
    
    To execute dynamically generated Python code, use Python's ``exec`` function; to evaluate a
    dynamically generated Python expression, use Python's ``eval`` function. Beware: using these functions
    (or any of the below) with input from the user potentially allows the user to execute arbitrary code.

code-executing
--------------

.. function:: execute

    Syntax:
        ``h.execute("statement")``

        ``h.execute("statement", objref)``

    Description:
        parse and execute the command in the context 
        of the object. If second arg not present then execute it at the 
        top level of the interpreter. 
        If command begins with a '~' then the tilda is removed and the rest 
        of the command is executed without enclosing it in {}. This allows 
        one to create a  func or proc dynamically. 

    .. seealso::
        :func:`execute1`


----

.. function:: execute1

    Syntax:
        ``err = h.execute1("statement")``

        ``err = h.execute1("statement", objref)``

        ``err = h.execute1("statement", show_err_mes)``

        ``err = h.execute1("statement", objref, show_err_mes)``

    Description:
        Same as :func:`execute` but returns 0 if there was an interpreter error 
        during execution of the statement and returns 1 if successful. 
        Does not surround the command with {}. 
         
        If the show_err_mes arg is present and equal to 0 then the normal 
        interpreter error message printing is turned off for the scope of the 
        statement. 
         
        Error messages can be turned on even inside the statement 
        with :func:`show_errmess_always`. 
         
        Parse and execute the command in the context 
        of the object. If second arg not present then execute it at the 
        top level of the interpreter. 
         
        When running under MPI, an error in the statement does NOT call
        MPI_Abort but returns 0. Note that it is not necessary to use
        this function to avoid calling MPI_Abort on error. That effect
        can also be obtained with :meth:`ParallelContext.mpiabort_on_error`.

    Example:
        Execute1 is heavily used in the construction of the fitter widgets. 
        It is also useful to objects in gaining information about the outside with 
        the idiom 

        .. code-block::
            python
            
            h.execute1(f"{obj_name}.var = outside_var")

        Here, outside_var is unavailable from within the object and so 
        a command is constructed which can be executed at the top level where that 
        variable is available and sets the public var in the object. 


code-loading
------------

.. function:: load_proc

    Syntax:
        ``h.load_proc("name1", ...)``

        ``h.load_func("name1", ...)``

        ``h.load_template("name1", ..._``

    Description:
        Load the file containing a definition. 
         
        For each name in the list 
        all the :file:`*.oc` and :file:`*.hoc` files will be searched and the first file that 
        contains the appropriate proc, func, or begintemplate will be loaded. 
        Loading only takes place if the name has not previously been defined. 
        The search path consists of the current working directory, followed by 
        the paths in the environment variable HOC_LIBRARY_PATH (space separated), 
        followed by :file:`$NEURONHOME/lib/hoc`. 
        Remember that only entire files are loaded-- not just the definition of 
        the name. And nothing is loaded if the name is already defined. 
        Inadvertent recursion will use up all the file descriptors. 
        For efficiency, on the first load, all the names are cached in a 
        temporary file and the file is scanned on subsequent loads for that session. 
         
    .. warning::
        This command is very slow under mswindows. Therefore it is often 
        useful to explicitly load the standard run library with the statement: 

        .. code-block::
            python

            h.xopen("$(NEURONHOME)/lib/hoc/noload.hoc") 


----

.. function:: load_file

    Syntax:
        ``h.load_file("filename")``

        ``h.load_file("filename", "name")``

        ``h.load_file(0or1, "filename")``

    Description:
        Similar to :func:`load_proc` but loads files and so does not have the 
        search overhead. Suitable for loading packages of files. 
         
        The functionality is identical to :func:`xopen` except that the xopen takes 
        place only if 
        if a file of that name has not already been loaded with the load_file, 
        :func:`load_proc`, :func:`load_template`, or :func:`load_func` functions. 
        The file is searched for in the current working 
        directory, $HOC_LIBRARY_PATH (a colon or space separated list of directories), 
        and :file:`$NEURONHOME/lib/hoc` directories (in that order) for 
        the file if there is no directory prefix. 
        Before doing the xopen on the file the current working directory is 
        temporarily changed to the directory containing the file so 
        that it can xopen files relative to its location. 
         
        If the second string arg exists, the file is xopen'ed only if the 
        name is not defined as a variable AND the file has not been loaded 
        with load_file. This is useful in those cases where the package was 
        first xopen'ed without going through the load_file function. 
         
        If the first arg is a number and is 1, then the file is loaded again even 
        if it has already been loaded. 


            


----

.. function:: load_func

    Syntax:
        ``h.load_func('name1', ...)``

        see :func:`load_proc` 

----

.. function:: load_template

    Syntax:
        ``h.load_template('name1', ...)``

        see :func:`load_proc` 

