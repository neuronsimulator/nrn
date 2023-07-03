
.. _hoc_lw_doc:

Obsolete Text Menus
-------------------

The functions above have been superseded by the graphical user interface 
but are available for use on unix machines and in the DOS version. 
See :hoc:class:`Graph`.

----



.. hoc:function:: fmenu


    Description:
        This is an old terminal based menu system that has been superseded by the 
        :ref:`hoc_GUI`.
         
        Fmenu creates, displays, and allows user to move within a menu to 
        select and change 
        a displayed variable value or to execute a command. 
         
        The user can create space for 
        a series of menus and execute individual menus with each menu consisting of 
        lists of 
        variables and commands. Menus can execute commands which call other 
        menus and in this way a hierarchical menu system can be constructed. 
        Menus can be navigated by using arrow keys or by typing the first character 
        of a menu item. To exit a menu, either press the :kbd:`Esc` key, execute the 
        "Exit" item, or execute a command which has a "stop" statement. 
        A command item is executed by pressing the Return key. A variable item 
        is changed by typing the new number followed by a Return. 
         
        See the file :file:`$NEURONHOME/doc/man/oc/menu.tex` for a complete description 
        of this function. 


