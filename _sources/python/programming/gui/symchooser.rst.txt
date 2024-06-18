.. _symchoos:

SymChooser
----------



.. class:: SymChooser


    Syntax:
        ``h.SymChooser()``

        ``h.SymChooser(caption)``

        ``h.SymChooser("caption", "varname")``


    Description:
        This class allows you to display a list of all names and variables in the system.  From this 
        list, you can simply select the name which you want to implement. 
        \ ``Symchooser()`` becomes especially convenient when you must keep track of many variables, 
        or when the names of 
        your variables become too long to type over and over again. 
         
        This symbol chooser object appears the same as the PlotWhat chooser of Graphs. 
         
        When the second argument is the name of a variable, a single panel SymChooser 
        is generated that is restricted to all names of the same type as that variable. 
        Useful arguments might be "hh" to see all distributed mechanism names, 
        "Graph" to see all Templates, "soma" to see all section names, or "v" to see 
        all range variable names. 
         

    Example:

        .. code::

            scobj = h.SymChooser() 
            scobj.run() 

        puts the symbol chooser on the screen, giving you access to all existing variables, 
        sections, object references, and objects.  You can click on a specific symbol and then 
        click on the "accept" button to call the symbol in the interpreter.  If you click on 
        an object reference in the left-hand window, a list of all possible commands for that type 
        of object will appear in the middle window.  From this list you can choose a 
        command to execute. 

         

----



.. method:: SymChooser.run


    Syntax:
        ``scobj.run()``


    Description:
        Pops up the SymChooser dialog. See PlotWhat for usage. 
        Returns 0 if cancel chosen, 1 for accept. 

         

----



.. method:: SymChooser.text


    Syntax:
        ``scobj.text(strdef)``


    Description:
        Places the text of last choice in *strdef*. This must be a reference to a HOC-style
        string not a Python string; see the example.

    Example:

        .. code::

            from neuron import h, gui

            h('create soma')
            h.soma.insert(h.hh)

            scobj = h.SymChooser()
            scobj.run()

            # read the result
            resultPtr = h.ref('')
            scobj.text(resultPtr)
            print(f'You selected: {resultPtr[0]}')

        .. image:: ../../images/symchooser.png
            :align: center  