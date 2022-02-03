Plotter Control (obsolete)
--------------------------


.. hoc:function:: lw

         
         

    Name:
        lw - laser writer graphical output (or HP pen plotter) 
         
         

    Syntax:
        ``lw(file)``

        ``lw(file, device)``

        ``lw()``




    Description:
        ``Lw(file, device)`` opens a file to keep a copy of subsequent 
        plots (*file* is a string variable or a name enclosed in double 
        quotes).  All graphs which are generated on the screen are saved in 
        this file in a format given by the integer value of the *device* argument. 
         


        *device* =1 
            Hewlett Packard pen plotter style. 

        *device* =2 
            Fig style (Fig is a public domain graphics program available 
            on the SUN computer).  The filter ``f2ps`` translates fig to postscript. 

        *device* =3 
            Codraw style. Files in this style can be read into the 
            PC program, ``CODRAW``.  The file should be opened with the extension, 
            ``.DRA``. 

         
        Lw keeps copying every plot to the screen until the file is closed with 
        the command, ``lw()``. Note that erasing the screen with ``plt(-3)`` or 
        a :kbd:`Control-e` will throw away whatever is in the file and restart the file at the 
        beginning.  Therefore, ``lw`` keeps an accurate representation of the 
        current graphic status of the screen. 
         
        After setting the device once, it remains the same unless changed again 
        by another call with two arguments.  The default device is 2. 
         
         

    Example:
        Suppose an HP plotter is connected to serial port, ``COM1:``.  Then 
        the following procedure will plot whatever graphics information 
        happens to be on the screen (not normal text). 
         

        .. code-block::
            none

            lw("temp", 1) 
            proc hp() { 
               plt(-1)  lw()  system("copy temp com1:")  lw("temp") 
            } 

         
        Notice that the above procedure closes a file, prints it, and then 
        re-opens :file:`temp`.  The initial direct command makes sure the 
        file is open the first time hp is called. 
         
         

    .. warning::
        It is often necessary to end all the plotting with a ``plt(-1)`` 
        command before closing the file to ensure that the last line drawing 
        is properly terminated. 
         
        In our hands the the HP plotter works well at 9600 BAUD and 
        with the line ``\verb+MODE COM1:9600,,,,P+`` in the autoexec.bat file. 
         
         

    .. seealso::
        :hoc:func:`plot`, :hoc:func:`graph`, :hoc:func:`plt`
        
        


