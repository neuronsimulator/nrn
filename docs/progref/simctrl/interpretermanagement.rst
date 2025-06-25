Interpreter Management
----------------------

.. function:: saveaudit

    .. tab:: Python

        Not completely implemented at this time. Saves all commands executed 
        by the interpreter. 


    .. tab:: HOC


        Not completely implemented at this time. Saves all commands executed 
        by the interpreter. 

    .. tab:: MATLAB

        Not supported. Instead: use MATLAB's ``diary`` function to save the command history,
        or use MATLAB's ``history`` function to view it.

        
----

.. function:: retrieveaudit

    .. tab:: Python
    
        Not completely implemented at this time. See :func:`saveaudit` . 


    .. tab:: HOC


        Not completely implemented at this time. See :func:`saveaudit` .
        
    .. tab:: MATLAB

        Not completely implemented at this time. See :func:`saveaudit` .


----

.. function:: quit

    .. tab:: Python
    
        Exits the program. Can be used as the action of a button. If edit buffers 
        are open you will be asked if you wish to save them before the final exit.


    .. tab:: HOC


        Exits the program. Can be used as the action of a button. If edit buffers 
        are open you will be asked if you wish to save them before the final exit.

    .. tab:: MATLAB

        Syntax:
            ``n.quit()``
        
        Description:

                Exits the program and MATLAB.
        
