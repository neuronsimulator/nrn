Session Printing and Saving
---------------------------

.. function:: print_session

    Syntax:
        ``h.print_session(useprinter, "name")``

        ``h.print_session(useprinter, "name", useselected)``

        ``h.print_session()``

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
        ``h.save_session("filename")``

        ``h.save_session("filename", "header")``

    Description:
        Save all the (saveable) windows on the screen to filename. 
        This is equivalent to pressing the :ref:`Session_SaveAll` button 
        on the :ref:`pwm`.
        If the header argument exists, it is copied to the beginning of 
        the file. 

    .. seealso::
        :meth:`PWManager.save`


