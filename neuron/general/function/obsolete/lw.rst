.. _lw:

obsolete
--------

        The above functions have been superseded by the graphical user interface 
        but are available for use on unix machines and in the DOS version. 
        See :class:`Graph` 

----



.. function:: fmenu


    Description:
        This is an old terminal based menu system that has been superseded by the 
        :func:`GUI` . 
         
        Fmenu creates, displays, and allows user to move within a menu to 
        select and change 
        a displayed variable value or to execute a command. 
         
        The user can create space for 
        a series of menus and execute individual menus with each menu consisting of 
        lists of 
        variables and commands. Menus can execute commands which call other 
        menus and in this way a hierarchical menu system can be constructed. 
        Menus can be navigated by using arrow keys or by typing the first character 
        of a menu item. To exit a menu, either press the Esc key, execute the 
        "Exit" item, or execute a command which has a "stop" statement. 
        A command item is executed by pressing the Return key. A variable item 
        is changed by typing the new number followed by a Return. 
         
        See the file $NEURONHOME/doc/man/oc/menu.tex for a complete description 
        of this function. 


----



.. function:: lw

         
         

    Name:
        lw - laser writer graphical output ( or HP pen plotter) 
         
         

    Syntax:
        :code:`lw(file)`

        :code:`lw(file, device)`

        :code:`lw()`




    Description:
        \ :code:`Lw(file, device)` opens a file to keep a copy of subsequent 
        plots (*file* is a string variable or a name enclosed in double 
        quotes).  All graphs which are generated on the screen are saved in 
        this file in a format given by the integer value of the *device* argument. 
         


        *device* =1 
            Hewlett Packard pen plotter style. 

        *device* =2 
            Fig style (Fig is a public domain graphics program available 
            on the SUN computer).  The filter \ :code:`f2ps` translates fig to postscript. 

        *device* =3 
            Codraw style. Files in this style can be read into the 
            PC program, \ :code:`CODRAW`.  The file should be opened with the extension, 
            \ :code:`.DRA`. 

         
        Lw keeps copying every plot to the screen until the file is closed with 
        the command, \ :code:`lw()`. Note that erasing the screen with \ :code:`plt(-3)` or 
        a (cntrl)E will throw away whatever is in the file and restart the file at the 
        beginning.  Therefore, \ :code:`lw` keeps an accurate representation of the 
        current graphic status of the screen. 
         
        After setting the device once, it remains the same unless changed again 
        by another call with two arguments.  The default device is 2. 
         
         

    Example:
        Suppose an HP plotter is connected to serial port, \ :code:`COM1:`.  Then 
        the following procedure will plot whatever graphics information 
        happens to be on the screen (not normal text). 
         

        .. code-block::
            none

            lw("temp", 1) 
            proc hp() { 
               plt(-1)  lw()  system("copy temp com1:")  lw("temp") 
            } 

         
        Notice that the above procedure closes a file, prints it, and then 
        re-opens \ :code:`temp`.  The initial direct command makes sure the 
        file is open the first time hp is called. 
         
         

    .. warning::
        It is often necessary to end all the plotting with a \ :code:`plt(-1)` 
        command before closing the file to ensure that the last line drawing 
        is properly terminated. 
         
        In our hands the the HP plotter works well at 9600 BAUD and 
        with the line ``\verb+MODE COM1:9600,,,,P+'' in the autoexec.bat file. 
         
         

    .. seealso::
        :func:`plot`, :func:`graph`, :func:`plt`
        
        


