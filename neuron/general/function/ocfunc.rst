.. _ocfunc:

ocfunc
------



.. function:: print_session


    Syntax:
        :code:`0 = print_session(useprinter, "name")`

        :code:`0 = print_session(useprinter, "name", useselected)`

        :code:`0 = print_session()`


    Description:
        Print a postscript file consisting of certain windows on the screen. 
         
        If useprinter==1 postscript is piped to the filter given by "name" 
        which should be able to deal with standard input (UNIX). If useprinter==0 
        the postscript is saved in the file specified by "name". 
         
        If there is a third arg equal to 1 then the printed windows are those 
        selected and arranged on the paper icon of the :ref:`PWM` and calling this function 
        is equivalent to pressing the :ref:`PWM_Print` button. Otherwise all 
        printable windows are printed in landscape mode with a size such that 
        the screen fits on the paper. 
         
        If there are no arguments then all the windows are printed in way that 
        works for mac, mswin, and unix. 


----



.. function:: save_session


    Syntax:
        :code:`0 = save_session("filename")`

        :code:`0 = save_session("filename", "header")`


    Description:
        Save all the (saveable) windows on the screen to filename. 
        This is equivalent to pressing the :ref:`Session_SaveAll` button 
        on the :ref:`pwm`.
        If the header argument exists, it is copied to the beginning of 
        the file. 

    .. seealso::
        :meth:`PWManager.save`


----



.. function:: prmat

        Prints the form of the matrix defined by :ref:`eqn <keyword_eqn>` statements. Each nonzero 
        element is printed as an "*". 

----



.. function:: solve

        Does one iteration of the non-linear system defined by :ref:`eqn <keyword_eqn>` statements. 
        Returns the linear norm of the difference between left and right hand sides 
        plus the change in the dependent variables. 

----



.. function:: eqinit

        Throws away previous dependent variable and equation specifications 
        from :ref:`eqn <keyword_eqn>` statements. 

----



.. function:: sprint


    Syntax:
        :code:`sprint(strdef, "format", args)`


    Description:
        Prints to a string. See :func:`printf` 


----



.. function:: strcmp


    Syntax:
        :code:`x = strcmp("string1", "string2")`


    Description:
        return negative, 0, or positive value 
        depending on how the strings compare lexicographically. 
        0 means they are identical. 


----



.. function:: startsw

        Initializes a stopwatch with a resolution of 1 second or 0.01 second if 
        gettimeofday system call is available. See :func:`stopsw` . 

----



.. function:: stopsw

        Returns the time since the stopwatch was last initialized with a :func:`startsw` . 

        .. code-block::
            none

            startsw() 
            for i=1,1000000 { x = sin(.2) ] 
            stopsw() 


    .. warning::
        Really the idiom 

        .. code-block::
            none

            x = startsw() 
            //... 
            startsw() - x 

        should be used since it allows nested timing intervals. 
         

----



.. function:: object_id


    Syntax:
        :code:`object_id(objref)`

        :code:`object_id(objref, 1)`


    Description:
        Returns 0 if the object reference does not point to an object instance. 
        (Otherwise returns the pointer cast to a double, not a very useful number) 
         
        If the second argument is 1, it returns the index of the object name. Returns 
        -1 if the object is the NULLObject. 


----



.. function:: allobjectvars


    Syntax:
        :code:`allobjectvars()`


    Description:
        Prints all the object references (objref variables) that have been 
        declared along with the class type of the object they reference and the 
        number of references. 

    .. warning::
        Instead of printing the address of the object in hex format, it ought 
        also to print the object_id and/or the internal instance name. 


----



.. function:: allobjects


    Syntax:
        :code:`allobjects()`

        :code:`allobjects("templatename")`

        :code:`nref = allobjects(objectref)`


    Description:
        Prints the internal names of all class instances (objects) available 
        from the interpreter along with the number of references to them. 
         
        With a templatename the list is restricted to objects of that class. 
         
        With an object variable, nothing is printed but the reference count 
        is returned. The count is too large by one if the argument was of the 
        form templatename[index] since a temporary reference is created while 
        the object is on the stack during the call. 


----



.. function:: numarg


    Syntax:
        :code:`n = numarg()`


    Description:
        Number of arguments passed to a user written hoc function. 

    .. seealso::
        :ref:`arguments`, :func:`argtype`

         

----



.. function:: argtype


    Syntax:
        :code:`itype = argtype(iarg)`


    Description:
        The type of the ith arg. The return value is 0 for numbers, 1 for objref, 
        2 for strdef, 3 for pointers to numbers, and -1 if the arg does not exist. 

    .. seealso::
        :ref:`arguments`, :func:`numarg`

         

----



.. function:: hoc_pointer_


    Syntax:
        :code:`hoc_pointer_(&variable)`


    Description:
        A function used by c and c++ implementations to request a pointer to 
        the variable from its interpreter name. Not needed by the user. 


----



.. function:: execute


    Syntax:
        :code:`execute("statement")`

        :code:`execute("statement", objref)`


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
        :code:`err = execute1("statement")`

        :code:`err = execute1("statement", objref)`

        :code:`err = execute1("statement", show_err_mes)`

        :code:`err = execute1("statement", objref, show_err_mes`


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


----



.. function:: load_proc


    Syntax:
        :code:`load_proc("name1", ...)`

        :code:`load_func("name1", ...)`

        :code:`load_template("name1", ..._`


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
            none

            xopen("$(NEURONHOME)/lib/hoc/noload.hoc") 



----



.. function:: load_file


    Syntax:
        :code:`load_file("filename")`

        :code:`load_file("filename", "name")`

        :code:`load_file(0or1, "filename")`


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


    Description:

----



.. function:: load_func

        see :func:`load_proc` 

----



.. function:: load_template

        see :func:`load_proc` 

----



.. function:: machine_name


    Syntax:
        :code:`strdef name`

        :code:`machine_name(name)`


    Description:
        returns the hostname of the machine. 


----



.. function:: saveaudit

        Not completely implemented at this time. Saves all commands executed 
        by the interpreter. 

----



.. function:: retrieveaudit

        Not completely implemented at this time. See :func:`saveaudit` . 

----



.. function:: coredump_on_error


    Syntax:
        :code:`coredump_on_error(1 or 0)`


    Description:
        On unix machines, sets a flag which requests (1) a coredump in case 
        of memory or bus errors. 

         

----



.. function:: quit

        Exits the program. Can be used as the action of a button. If edit buffers 
        are open you will be asked if you wish to save them before the final exit. 

----



.. function:: object_push


    Syntax:
        :code:`object_push(objref)`


    Description:
        Enter the context of the object referenced by objref. In this context you 
        can directly access any variables or call any functions, even those not 
        declared as :ref:`public <keyword_public>`. Do not attempt to create any new symbol names! 
        This function is generally used by the object itself to save its state 
        in a session. 


----



.. function:: object_pop


    Syntax:
        :code:`object_pop()`


    Description:
        Pop the last object from an :func:`object_push` . 


----



.. function:: show_errmess_always


    Syntax:
        :code:`show_errmess_always(boolean)`


    Description:
        Sets or turns off a flag which, if on, always prints the error message even 
        if normally turned off by an :func:`execute1` statement or other call to the 
        interpreter. 


----



.. function:: name_declared


    Syntax:
        :code:`type = name_declared("name")`

        :code:`type = name_declared("name", 1)`


    Description:
        Return 0 if the name is not in the symbol table. The first form looks 
        for names in the top level symbol table. The second form looks in the 
        current object context. 
         
        If the name exists return 
         
        2 if an :func:`objref` . 
         
        3 if a Section 
         
        4 if a :ref:`strdef <keyword_strdef>` 
         
        5 if a scalar or :ref:`double <keyword_double>` variable. 
         
        1 otherwise 
         
        Note that names can be (re)declared only if they do not already 
        exist or are already of the same type. 
        This is too useful to require the user to waste an objref in creating a 
        :class:`StringFunctions` class to use :meth:`~StringFunctions.is_name`. 

        .. code-block::
            none

            name_declared("nrnmainmenu_") 
            {object_push(nrnmainmenu_) print name_declared("ldfile", 0) object_pop()} 
            {object_push(nrnmainmenu_) print name_declared("ldfile", 1) object_pop()} 


         

