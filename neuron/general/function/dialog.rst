.. _dialog_doc:

Dialog Windows
--------------
         
The following dialogs pop up at the center of the screen and block everything 
until dealt with. 

.. seealso::
    :class:`SymChooser`, :meth:`VBox.dialog`


----



.. function:: boolean_dialog


    Syntax:
        :code:`boolean_dialog("label", ["accept", "cancel"])`


    Description:
        return 1 or 0 

         

----



.. function:: continue_dialog


    Syntax:
        :code:`continue_dialog("label")`


    Description:
        info to the user 

         

----



.. function:: string_dialog


    Syntax:
        :code:`string_dialog("label", strdef)`


    Description:
        returns 0 if canceled and *strdef* remains unchanged 


