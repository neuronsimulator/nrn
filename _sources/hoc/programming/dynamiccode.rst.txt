Dynamic Code Loading and Execution
==================================

These functions either execute code contained in an arbitrary, possibly
generated-at-runtime string or load code from a file potentially chosen at runtime.


code-executing
--------------

.. hoc:function:: execute

    Syntax:
        ``execute("statement")``

        ``execute("statement", objref)``

    Description:
        parse and execute the command in the context 
        of the object. If second arg not present then execute it at the 
        top level of the interpreter. 
        If command begins with a '~' then the tilda is removed and the rest 
        of the command is executed without enclosing it in {}. This allows 
        one to create a  func or proc dynamically. 

    .. seealso::
        :hoc:func:`execute1`


----

.. hoc:function:: execute1

    Syntax:
        ``err = execute1("statement")``

        ``err = execute1("statement", objref)``

        ``err = execute1("statement", show_err_mes)``

        ``err = execute1("statement", objref, show_err_mes``

    Description:
        Same as :hoc:func:`execute` but returns 0 if there was an interpreter error
        during execution of the statement and returns 1 if successful. 
        Does not surround the command with {}. 
         
        If the show_err_mes arg is present and equal to 0 then the normal 
        interpreter error message printing is turned off for the scope of the 
        statement. 
         
        Error messages can be turned on even inside the statement 
        with :hoc:func:`show_errmess_always`.
         
        Parse and execute the command in the context 
        of the object. If second arg not present then execute it at the 
        top level of the interpreter. 
         
    Example:
        Execute1 is heavily used in the construction of the fitter widgets. 
        It is also useful to objects in gaining information about the outside with 
        the idiom 

        .. code-block::
            none

            sprint(cmd, "%s.var = outside_var", this) 
            execute1(cmd) 

        Here, outside_var is unavailable from within the object and so 
        a command is constructed which can be executed at the top level where that 
        variable is available and sets the public var in the object. 


code-loading
------------

.. hoc:function:: load_proc

    Syntax:
        ``load_proc("name1", ...)``

        ``load_func("name1", ...)``

        ``load_template("name1", ..._``

    Description:
        Load the file containing a definition. 
         
        For each name in the list 
        all the :file:`*.oc` and :file:`*.hoc` files will be searched and the first file that 
        contains the appropriate proc, func, or begintemplate will be loaded. 
        Loading only takes place if the name has not previously been defined. 
        The search path consists of the current working directory, followed by 
        the paths in the environment variable HOC_LIBRARY_PATH (space separated), 
        followed by `$NEURONHOME/lib/hoc <http://neuron.yale.edu/hg/neuron/nrn/file/tip/share/lib/hoc>`_. 
        Remember that only entire files are loaded-- not just the definition of 
        the name. And nothing is loaded if the name is already defined. 
        Inadvertent recursion will use up all the file descriptors. 
        For efficiency, on the first load, all the names are cached in a 
        temporary file and the file is scanned on subsequent loads for that session. 
         
    .. warning::
        This command is very slow under mswindows. Therefore it is often 
        useful to explicitly load the standard run library with the statement: 

        .. code-block::
            none

            xopen("$(NEURONHOME)/lib/hoc/noload.hoc") 


----

.. hoc:function:: load_file

    Syntax:
        ``load_file("filename")``

        ``load_file("filename", "name")``

        ``load_file(0or1, "filename")``

    Description:
        Similar to :hoc:func:`load_proc` but loads files and so does not have the
        search overhead. Suitable for loading packages of files. 
         
        The functionality is identical to :hoc:func:`xopen` except that the xopen takes
        place only if 
        if a file of that name has not already been loaded with the load_file, 
        :hoc:func:`load_proc`, :hoc:func:`load_template`, or :hoc:func:`load_func` functions.
        The file is searched for in the current working 
        directory, $HOC_LIBRARY_PATH (a colon or space separated list of directories), 
        and `$NEURONHOME/lib/hoc <http://neuron.yale.edu/hg/neuron/nrn/file/tip/share/lib/hoc>`_ directories (in that order) for 
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


    Description:


----

.. hoc:function:: load_func

        see :hoc:func:`load_proc`

----

.. hoc:function:: load_template

        see :hoc:func:`load_proc`

